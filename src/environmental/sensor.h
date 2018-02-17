#include <Arduino.h>

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
      bool isEnabled = false;

    public:
      void setup(const int sda = 21, const int scl = 22);
      void measure(environmental::Measurement &measurement);
  };
}
#endif // SENSOR_H_
