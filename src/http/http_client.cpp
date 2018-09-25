#include "http_client.h"
#include <string.h>
#include <cstdio>
#include <stdlib.h>
#include <esp_log.h>

static const char LogTag[] PROGMEM = "HttpClient";

namespace Http {
  Client::Client(const uint32_t rxTimeout) {
    this->tcp.setRxTimeout(rxTimeout);
  }

  Client::~Client() {}

  bool Client::get(const String& uri, const std::function<void(bool, Response)> callback) {
    ESP_LOGI(LogTag, "uri: %s", uri.c_str());

    Uri::Uri u;

    bool success = Uri::UriParser::parse(uri, u);
    if(!success) {
      ESP_LOGE(LogTag, "invalid uri.");
      callback(false, this->response);
      return false;
    }

    this->response.statusCode = 0;
    this->callback = callback;

    this->tcp.onConnect([this, u](void* v, AsyncClient* c) {
      ESP_LOGI(LogTag, "connected %s:%d.", c->remoteIP().toString().c_str(), c->remotePort());

      if(!SendRequest(u, c)) {
        ESP_LOGI(LogTag, "connected call.");
        this->callback(false, this->response);
      }
    });

    this->tcp.onData(OnData, this);

    // @ToDo: do we need a callback here?
    this->tcp.onDisconnect([this](void* v, AsyncClient* c) {
      ESP_LOGI(LogTag, "disconnected.");

      if(this->callback) {
        this->callback(false, this->response);
      }
    });

    this->tcp.onTimeout([this](void* v, AsyncClient* c, uint32_t time) {
      ESP_LOGE(LogTag, "timeout %d.", time);

      this->callback(false, this->response);
    });

    this->tcp.onError([](void* v, AsyncClient* c, int8_t error) {
      ESP_LOGE(LogTag, "error %d.", error);

      // callback(false);
    });

    return this->tcp.connect(u.domain.c_str(), u.port);
  }

  //
  // - private
  //
  void Client::onData(AsyncClient* c, char *data, size_t len) {

    // Headers are always in the first response.
    if(this->response.statusCode == 0) {
      bool success = ParseResponse(data, len, response);
      if(!success) {
        c->stop();
        this->callback(false, response);
        this->callback = nullptr;
        return;
      }

      this->contentReceivedLength = response.body.length();
      this->contentLength = atol(response.headers["Content-Length"].c_str());
    } else {
      this->contentReceivedLength += len;

      memcpy(this->buffer, data, len);
      this->buffer[len] = '\0';

      this->response.body.concat(buffer);
    }

    if(this->contentReceivedLength >= this->contentLength && this->callback) {
      c->stop();
      this->callback(true, response);
      this->callback = nullptr;
    }
  }

  void Client::OnData(void* v, AsyncClient* c, void *data, size_t len) {
    Client *self = static_cast<Client*>(v);

    if(self != NULL) {
      char* rawData = reinterpret_cast<char*>(data);
      self->onData(c, rawData, len);
    }
  }

  //
  // - Static
  //
  bool Client::ParseResponse(char* rawData, const size_t len, Response& res) {
    static const size_t kEndOfResponseHeaderLength = 4;
    size_t i = 0;
    char* line = rawData;
    char* position = line;
    bool isFirstline = true;
    bool success = true;

    for(; i < len - kEndOfResponseHeaderLength; i++) {
      if(rawData[i] == '\r' && rawData[i + 1] == '\n') {
        rawData[i] = '\0';

        // The first line in the response contains the status line. Parse that
        // first then continue with parsing normal header fields.
        if(isFirstline) {
          isFirstline = false;
          if(!ParseResponseStatusLine(line, i, res)) {
            success = false;
            break;
          }
        } else if(!ParseResponseHeaderField(line, i, res)) {
          success = false;
          break;
        }

        // The end of the header is marked by a double cr lf. Check if there's
        // space in the buffer and for (another) cr lf combo.
        // Otherwise check if there's space and move to the next header field.
        if(i + kEndOfResponseHeaderLength < len && rawData[i + 2] == '\r' && rawData[i + 3] == '\n') {
          line = &rawData[i + kEndOfResponseHeaderLength];
          break;
        } else if(i + 2 < len) {
          line = &rawData[i + 2];
        } else {
          ESP_LOGE(LogTag, "could not parse response header fields.");
          success = false;
          break;
        }
      } else {
        (++position) = &rawData[i];
      }
    }

    if(success && (i + kEndOfResponseHeaderLength) < len) {
      res.body = String(line);
    } else {
      res.body = String("");
    }

    return true;
  }

  bool Client::ParseResponseStatusLine(char* line, const size_t lineLength, Response& response) {
    static const char kStatusLine[] PROGMEM = "HTTP/1.1 ";
    static const size_t kStatusLineLength = strlen(kStatusLine);

    if(lineLength < kStatusLineLength) {
      return false;
    }

    // Check if status line starts with "HTTP/1.1 ".
    if(strncmp(line, kStatusLine, kStatusLineLength) == 0) {
      // Pointer to where the status code starts, it starts right after HTTP/1.1
      // and up to the first space.
      const char* statusCode = &line[kStatusLineLength];

      // Find the position of the first space and replace it with a
      // null terminating char (\0).
      const size_t endOfStatusCodePosition = strcspn(statusCode, " ");

      if((kStatusLineLength + endOfStatusCodePosition) >= lineLength) {
        ESP_LOGE(LogTag, "could not find status code in status line.");
        return false;
      }

      char* endOfStatusCode = (char*)&statusCode[endOfStatusCodePosition];
      *endOfStatusCode = '\0';

      response.statusCode = atoi(statusCode);

      // The reason message starts right after the status code, the String
      // is already null terminated.
      if((endOfStatusCodePosition + kStatusLineLength) >= lineLength) {
        ESP_LOGE(LogTag, "could not find reason message in status line.");
        return false;
      }

      //
      response.reasonMessage = String(++endOfStatusCode);
    } else {
      ESP_LOGE(LogTag, "invalid status line.");
      return false;
    }

    return true;
  }

  bool Client::ParseResponseHeaderField(char* line, const size_t lineLength, Response& response) {
    // Find position of : char in line and replace with terminating char.
    const size_t keyPosition = strcspn(line, ":");
    line[keyPosition] = '\0';

    if(keyPosition + 2 < lineLength) {
      response.headers.insert(std::make_pair(line, &line[keyPosition + 2]));

      return true;
    } else {
      ESP_LOGE(LogTag, "could not parse header field.");
      return false;
    }
  }

  // @ToDo: optimize.
  bool Client::SendRequest(const Uri::Uri& u, AsyncClient* c) {
    String requestLine = "GET " + u.path + " HTTP/1.1\r\nHost: " + u.domain + "\r\n\r\n";
    if(c->space() > requestLine.length() && c->canSend()) {
      c->add(requestLine.c_str(), requestLine.length());
      c->send();

      return true;
    }

    return false;
  }
}
