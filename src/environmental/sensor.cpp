#include "sensor.h"
#include <Wire.h>

namespace environmental {
  void Sensor::setup() {
    uint8_t chipId = bme280.readChipId();
    if(chipId == 0xff) {
      Serial.println(F("sensor BME280: sensor not found."));

      return;
    } else {
      // find the chip ID out just for fun
      Serial.print(F("sensor BME280: chip identifier=0x"));
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

  bool Sensor::enabled() {
    return this->isEnabled;
  }

  bool Sensor::measure(environmental::Measurement &measurement) {
    if(!this->isEnabled) {
      // Try to find BME280.
      setup();

      // If still not found, return.
      if(!this->isEnabled) {
        Serial.println(F("error: could not find BME280 sensor."));
        return false;
      }
    }

    Serial.print(F("sensor BME280: measuring "));

    unsigned long timeout = millis();
    while (bme280.isMeasuring()) {
      if(millis() - timeout > kMeasurementTimeout) {
        Serial.println(F("error: timeout measuring temperature."));
        this->isEnabled = false;
        return false;
      }
      Serial.print(F("."));
      delay(50);
    }
    Serial.println(F(" done."));

    bme280.readMeasurements();

    measurement.temperature = bme280.getTemperatureMostAccurate();
    measurement.pressure    = bme280.getPressureMostAccurate();
    measurement.humidity    = bme280.getHumidityMostAccurate();

    return true;
  }
}
