#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <Arduino.h>
#include <AsyncTCP.h>
#include <map>
#include "uri_parser.h"

namespace Http {
  struct Response {
    std::map<String, String> headers;
    String body;
    uint16_t statusCode;
    String reasonMessage;
  };

  class Client {
    public:
      Client();
      ~Client();
      bool get(const String& uri, const std::function<void(bool, Response)> callback);

    private:
      AsyncClient tcp;
      Response response;
      size_t contentReceivedLength = 0;
      size_t contentLength = 0;
      std::function<void(bool, Response)> callback;
      // @ToDo: find better solution.
      char buffer[4096];

      void onData(AsyncClient* c, char *data, size_t len);
      static void OnData(void* v, AsyncClient* c, void *data, size_t len);
      static bool SendRequest(const Uri::Uri& u, AsyncClient* c);
      static bool ParseResponse(char* rawData, const size_t len, Response& res);
      static bool ParseResponseStatusLine(char* line, const size_t lineLength, Response& response);
      static bool ParseResponseHeaderField(char* line, const size_t lineLength, Response& response);
  };
}

#endif // _HTTP_CLIENT_H_
