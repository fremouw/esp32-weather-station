#include "client.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "verisign_ca.h"

namespace wunderground {
  Client::Client(const String& apiKey,
                 const String& language,
                 const String& country,
                 const String& city) : apiKey(apiKey), language(language) {
    query = "/q/" + country + "/" + city;
  }

  Client::Client(const String& apiKey,
                 const String& language,
                 const String& latLon) : apiKey(apiKey), language(language) {
    query = "/q/" + latLon;
  }

  void Client::update(const std::function<void(bool, Conditions&)>callback) {
    String path = "/api/" + apiKey + "/conditions/forecast/lang:" + language + query + ".json";
    bool success = false;

    // WiFiClientSecure client;
    WiFiClient client;

    //client.setCACert(kVeriSignRootCA);

    if (client.connect(kApiUrl, kApiUrlPort)) {
      String request = \
        "GET " + path + " HTTP/1.1\r\n" \
        "Host: " + kApiUrl + "\r\n" \
        "Connection: close\r\n\r\n";

      client.write(request.c_str(), request.length());

      size_t jsonBufferSize = kDefaultJsonBufferSize;
      unsigned long timeout = millis();
      while(client.connected()) {
        if(client.available()) {
          static const char kContentLength[] PROGMEM = "Content-Length: ";

          // Find and get content length from header.
          bool foundContentLength = client.find(kContentLength);
          if(!foundContentLength) {
            Serial.println("error: could not find content-length in HTTP response header.");
            break;
          }

          // Read content length and multiply with a factor, this should cover
          // the ArduinoJson overhead.
          String contentLength = client.readStringUntil('\r');
          const long length = contentLength.toInt();
          if(length > 0) {
            jsonBufferSize = length * 1.5;
          }

          // Skip all the headers.
          static const char kEndOfHeaders[] PROGMEM = "\r\n\r\n";

          bool foundBody = client.find(kEndOfHeaders);
          if(!foundBody) {
            Serial.println("error: could not find response body.");
            break;
          }

          DynamicJsonBuffer jsonBuffer(jsonBufferSize);

          JsonObject& root = jsonBuffer.parse(client);

          if(!root.success()) {
            Serial.println("error: could not parse JSON response.");
            break;
          }

          this->conditions.parse(root);

          success = true;

          break;
        }

        if (millis() - timeout > kHTTPTimeout) {
          Serial.println("error: unable to connect to Weather Underground, timeout.");
          break;
        }

        delay(50);
      }

      client.stop();
    }

    callback(success, this->conditions);
  }
}
