#include <Arduino.h>
#include <AsyncTCP.h>
#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include "http/http_client.h"
#include "conditions.h"

#ifndef WUNDERGROUND_CLIENT_H_
#define WUNDERGROUND_CLIENT_H_

namespace wunderground {
  static const char kApiUri[] PROGMEM = "http://api.wunderground.com";

  class Client {
    public:
      Client(const String& apiKey, const String& language, const String& latLon);
      Client(const String& apiKey, const String& language, const String& country, const String& city);
      ~Client();

      void update(const std::function<void(bool,Conditions&)> callback);
    private:
      static const int kHTTPTimeout = 5000;
      static const size_t kDefaultJsonBufferSize = 32768;//16384; //8192;

      Http::Client http;
      String apiKey;
      String language;

      String query;
      Conditions conditions;
    };
}

#endif // WUNDERGROUND_CLIENT_H_
