#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <BME280_MOD-1022.h>

#ifndef SENSOR_H_
#define SENSOR_H_

namespace environmental {
  struct Measurement {
    float temperature;
    float pressure;
    float humidity;
  };

  class Sensor {
    private:
      static const int kMeasurementTimeout = 3000;
      bool isEnabled = false;
      BME280Class bme280;

    public:
      Sensor(TwoWire& wire): bme280(wire) {

      };

      void setup();
      bool enabled();
      bool measure(environmental::Measurement &measurement);
  };
}
#endif // SENSOR_H_
