#include "Conditions.h"

namespace wunderground {
  void Conditions::parse(JsonObject& root) {
    JsonObject& response = root["response"];
    const char* response_version = response["version"];

    JsonObject& currentObservation = root["current_observation"];
    parseCurrentObservation(currentObservation);

    JsonObject& forecast = root["forecast"];
    parseForecast(forecast);
  }

  void Conditions::getCurrentObservation(Observation& observation) {
    observation = this->observation;
  }

  void Conditions::getForecastForPeriod(const int period, Forecast& forecast) {
    if (period < kMaxForecasts) {
      forecast = this->forecasts[period];
    }
  }

  void Conditions::printConditions() {

  }

  void Conditions::parseCurrentObservation(JsonObject& root) {
    // Current weather
    Observation observation;

    observation.city = root["display_location"]["city"].as<String>();

    observation.temperature = root["temp_c"];
    observation.title = root["weather"].as<String>();
    observation.icon = root["icon"].as<String>();

    // Only the url contains the prefix to either show the night (nt_),
    // or regular day icon.
    String iconUrl = root["icon_url"].as<String>();
    const int iconnameIndex = iconUrl.lastIndexOf('/');
    if(iconnameIndex > -1 && iconnameIndex < iconUrl.length()) {
      const int extensionIndex = iconUrl.indexOf('.', iconnameIndex);
      if(extensionIndex > -1) {
        observation.icon = iconUrl.substring(iconnameIndex + 1, extensionIndex);
        Serial.print("IconUrl=");
        Serial.println(observation.icon);
      }
    }

    this->observation = observation;
  }

  void Conditions::parseForecast(JsonObject& forecast) {
    JsonArray& forecastDay = forecast["simpleforecast"]["forecastday"];

    uint8_t index = 0;
    for(JsonArray::iterator it = forecastDay.begin(); it != forecastDay.end(); ++it)
    {
      JsonObject& forecastObject = *it;

      Forecast forecast;

      forecast.title = forecastObject["date"]["weekday"].as<String>();
      forecast.highTemperature = forecastObject["high"]["celsius"].as<float>();
      forecast.lowTemperature = forecastObject["low"]["celsius"].as<float>();
      forecast.icon = static_cast<const char *>(forecastObject["icon"]);

      forecasts[index++] = forecast;

      if(index > kMaxForecasts - 1) {
        break;
      }
    }
  }
}
