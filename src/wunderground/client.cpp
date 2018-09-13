#include "client.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <esp_log.h>

static const char LogTag[] PROGMEM = "WeatherUnderground";

namespace wunderground {
  // Client::Client() {}
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

  Client::~Client() {}

  void Client::update(const std::function<void(bool, Conditions&)> callback) {
    String uri = String(kApiUri) + "/api/" + apiKey + "/conditions/forecast/lang:" + language + query + ".json";

    http.get(uri, [this, callback](bool success, Http::Response response) {
      ESP_LOGI(LogTag, "get callback.");

      // auto it = response.headers.begin();
      // while(it != response.headers.end()) {
      //   Serial.print(it->first);
      //   Serial.print("=");
      //   Serial.println(it->second);
      //   it++;
      // }

      DynamicJsonBuffer jsonBuffer(kDefaultJsonBufferSize);
      JsonVariant variant = jsonBuffer.parse(response.body.c_str());

      if (variant.is<JsonObject>()) {
        ESP_LOGI(LogTag, "response is JSON object.");

        JsonObject& root = variant;

        ESP_LOGI(LogTag, "response version: %s.", root["response"]["version"].as<String>().c_str());

        this->conditions.parse(root);
      } else {
        ESP_LOGE(LogTag, "invalid JSON response.");
        success = false;
      }

      callback(success, this->conditions);
    });
  }
}
