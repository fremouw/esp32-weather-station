#include "sensor.h"
#include <Wire.h>
#include <SPI.h>
#include <BME280_MOD-1022.h>

namespace environmental {
  void Sensor::setup(const int sda, const int scl) {
    // Wire.begin(sda, scl);

    BME280.readCompensationParams();

    // BME280.writeFilterCoefficient(fc_off);
    BME280.writeFilterCoefficient(fc_16);      // IIR Filter coefficient 16

    BME280.writeOversamplingPressure(os2x);
    BME280.writeOversamplingTemperature(os2x);
    BME280.writeOversamplingHumidity(os1x);

    // BME280.writeMode(smForced);

    BME280.writeStandbyTime(tsb_1000ms);        // tsb = 0.5ms
    // BME280.writeOversamplingPressure(os16x);    // pressure x16
    // BME280.writeOversamplingTemperature(os2x);  // temperature x2
    // BME280.writeOversamplingHumidity(os1x);     // humidity x1

    BME280.writeMode(smNormal);

    uint8_t chipId = BME280.readChipId();
    if(chipId == 0xff) {
      Serial.println("sensor BME280: sensor not found.");
    } else {
      // find the chip ID out just for fun
      Serial.print("sensor BME280: chip identifier=0x");
      Serial.println(chipId, HEX);
      isEnabled = true;
    }
  }

  void Sensor::measure(environmental::Measurement &measurement) {
    if(!this->isEnabled) {
      return;
    }

    Serial.print("sensor BME280: measuring ");
    while (BME280.isMeasuring()) {
      Serial.print(".");
      delay(50);
    }
    Serial.println(" done.");

    BME280.readMeasurements();

    measurement.temperature = BME280.getTemperature();
    measurement.pressure    = BME280.getPressure();
    measurement.humidity    = BME280.getHumidity();
  }
}
