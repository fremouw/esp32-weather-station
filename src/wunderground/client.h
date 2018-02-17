#include <Arduino.h>
#include <AsyncTCP.h>
#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include "Conditions.h"

#ifndef WUNDERGROUND_CLIENT_H_
#define WUNDERGROUND_CLIENT_H_

namespace wunderground {
  static const char kApiUrl[] PROGMEM = "api.wunderground.com";
  static unsigned int kApiUrlPort = 443;

  class Client {
    public:
      Client(const String& apiKey, const String& language, const String& latLon);
      Client(const String& apiKey, const String& language, const String& country, const String& city);

      void update(const std::function<void(bool,Conditions&)> callback);
    private:
      static const int kHTTPTimeout = 5000;
      static const size_t kDefaultJsonBufferSize = 8192;

      String apiKey;
      String language;

      String query;

      Conditions conditions;
    };
}

#endif // WUNDERGROUND_CLIENT_H_
