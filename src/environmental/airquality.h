#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"

#ifndef AIRQUALITY_H_
#define AIRQUALITY_H_

namespace environmental {
  struct AirQualityMeasurement {
    uint16_t eCo2;
    uint16_t tVoc;
  };

  class AirQuality {
    private:
      static const int kMeasurementTimeout = 3000;
      bool isEnabled = false;
      uint32_t humidity = 0;
      Adafruit_SGP30 sgp;
      TwoWire& wire;

    public:
      AirQuality(TwoWire& wire): wire(wire) {
      };

      void setHumidity(uint32_t humidity);

      void setup();
      bool enabled();
      bool getBaseline(environmental::AirQualityMeasurement &measurement);
      bool measure(environmental::AirQualityMeasurement &measurement);
  };
}
#endif // AIRQUALITY_H_
