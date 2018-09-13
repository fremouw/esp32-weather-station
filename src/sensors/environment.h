#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <BME280_MOD-1022.h>

#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

namespace Sensors {
  class Environment {
    private:
      static const int kMeasurementTimeout = 3000;
      bool isEnabled = false;
      BME280Class bme280;

    public:
      struct Measurement {
        float temperature;
        float pressure;
        float humidity;
      };

      Environment(TwoWire& wire = Wire): bme280(wire) {};
      ~Environment() {};

      void setup();
      bool enabled();
      bool measure(Measurement &measurement);
  };
}
#endif // _ENVIRONMENT_H_
