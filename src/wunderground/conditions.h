#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef WUNDERGROUND_Conditions_H_
#define WUNDERGROUND_Conditions_H_

namespace wunderground {
  class Conditions {
    public:
      struct Forecast {
        String title;
        String icon;
        float lowTemperature;
        float highTemperature;
      };

      struct Observation {
        float temperature;
        String city;
        String icon;
        String title;
      };

      void parse(JsonObject& root);

      void getCurrentObservation(Observation& observation);
      void getForecastForPeriod(const int period, Forecast& forecast);
      void printConditions();

    private:
      static const int kMaxForecasts = 4;

      Observation observation;
      Forecast forecasts[kMaxForecasts];

      void parseCurrentObservation(JsonObject& root);
      void parseForecast(JsonObject& forecast);
  };
}

#endif // WUNDERGROUND_Conditions_H_
