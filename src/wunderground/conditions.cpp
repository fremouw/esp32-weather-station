#include "Conditions.h"

namespace wunderground {
  void Conditions::parse(JsonObject& root) {
    JsonObject& currentObservation = root[F("current_observation")];
    parseCurrentObservation(currentObservation);

    JsonObject& forecast = root[F("forecast")];
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

    observation.city = root[F("display_location")][F("city")].as<String>();

    observation.temperature = root[F("temp_c")];
    observation.title = root[F("weather")].as<String>();
    observation.icon = root[F("icon")].as<String>();

    // Only the url contains the prefix to either show the night (nt_),
    // or regular day icon.
    String iconUrl = root[F("icon_url")].as<String>();
    const int iconnameIndex = iconUrl.lastIndexOf('/');
    if(iconnameIndex > -1 && iconnameIndex < iconUrl.length()) {
      const int extensionIndex = iconUrl.indexOf('.', iconnameIndex);
      if(extensionIndex > -1) {
        observation.icon = iconUrl.substring(iconnameIndex + 1, extensionIndex);
      }
    }

    this->observation = observation;
  }

  void Conditions::parseForecast(JsonObject& forecast) {
    JsonArray& forecastDay = forecast[F("simpleforecast")][F("forecastday")];

    uint8_t index = 0;
    for(JsonArray::iterator it = forecastDay.begin(); it != forecastDay.end(); ++it)
    {
      JsonObject& forecastObject = *it;

      Forecast forecast;

      forecast.title = forecastObject[F("date")][F("weekday")].as<String>();
      forecast.highTemperature = forecastObject[F("high")][F("celsius")].as<float>();
      forecast.lowTemperature = forecastObject[F("low")][F("celsius")].as<float>();
      forecast.icon = static_cast<const char *>(forecastObject[F("icon")]);

      forecasts[index++] = forecast;

      if(index > kMaxForecasts - 1) {
        break;
      }
    }
  }
}
