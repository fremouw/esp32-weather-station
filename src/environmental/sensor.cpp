#include "sensor.h"
#include <Wire.h>

namespace environmental {
  void Sensor::setup() {
    uint8_t chipId = bme280.readChipId();
    if(chipId == 0xff) {
      Serial.println("sensor BME280: sensor not found.");

      return;
    } else {
      // find the chip ID out just for fun
      Serial.print("sensor BME280: chip identifier=0x");
      Serial.println(chipId, HEX);
      isEnabled = true;
    }

    bme280.readCompensationParams();

    // BME280.writeFilterCoefficient(fc_off);
    bme280.writeFilterCoefficient(fc_16);      // IIR Filter coefficient 16

    bme280.writeOversamplingPressure(os2x);
    bme280.writeOversamplingTemperature(os2x);
    bme280.writeOversamplingHumidity(os1x);

    // BME280.writeMode(smForced);

    bme280.writeStandbyTime(tsb_1000ms);        // tsb = 0.5ms
    // BME280.writeOversamplingPressure(os16x);    // pressure x16
    // BME280.writeOversamplingTemperature(os2x);  // temperature x2
    // BME280.writeOversamplingHumidity(os1x);     // humidity x1

    bme280.writeMode(smNormal);
  }

  void Sensor::measure(environmental::Measurement &measurement) {
    if(!this->isEnabled) {
      setup();
      return;
    }

    Serial.print("sensor BME280: measuring ");
    while (bme280.isMeasuring()) {
      Serial.print(".");
      delay(50);
    }
    Serial.println(" done.");

    bme280.readMeasurements();

    measurement.temperature = bme280.getTemperature();
    measurement.pressure    = bme280.getPressure();
    measurement.humidity    = bme280.getHumidity();
  }
}
