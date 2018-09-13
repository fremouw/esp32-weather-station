#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"

#ifndef AIRQUALITY_H_
#define AIRQUALITY_H_

namespace Sensors {
  class AirQuality {
    public:
      struct Measurement {
        uint16_t eCo2;
        uint16_t tVoc;
      };

      AirQuality(TwoWire& wire = Wire): wire(wire) {};
      ~AirQuality() {};

      void setHumidity(uint32_t humidity);

      void setup();
      bool enabled();
      bool getBaseline(Measurement& measurement);
      void setBaseline(const Measurement& measurement);
      bool measure(Measurement& measurement);

    private:
      static const int kMeasurementTimeout = 3000;
      bool isEnabled = false;
      uint32_t humidity = 0;
      Adafruit_SGP30 sgp;
      TwoWire& wire;
      Measurement baseline = { 0, 0 };
  };
}

#endif // AIRQUALITY_H_
