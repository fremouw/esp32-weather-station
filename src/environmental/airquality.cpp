#include "airquality.h"

namespace environmental {
  void AirQuality::setup() {
    this->isEnabled = this->sgp.begin(&wire);

    if(this->isEnabled) {
      this->sgp.IAQinit();

      Serial.print(F("Found SGP30 serial #"));
      Serial.print(sgp.serialnumber[0], HEX);
      Serial.print(sgp.serialnumber[1], HEX);
      Serial.println(sgp.serialnumber[2], HEX);

      this->humidity = 0;
    }
  }

  bool AirQuality::enabled() {
    return this->isEnabled;
  }

  void AirQuality::setHumidity(uint32_t humidity) {
    this->humidity = humidity;
  }

  bool AirQuality::getBaseline(environmental::AirQualityMeasurement &measurement) {
    if(!this->isEnabled) {
      return false;
    }

    uint16_t tvoc;
    uint16_t eco2;

    if(sgp.getIAQBaseline(&eco2, &tvoc)) {
      Serial.print(F("info: air quality baseline values: eCO2: 0x"));
      Serial.print(eco2, HEX);
      Serial.print(F(", TVOC: 0x"));
      Serial.println(tvoc, HEX);

      measurement.tVoc = tvoc;
      measurement.eCo2 = eco2;

      return true;
    }

    return false;
  }

  bool AirQuality::measure(environmental::AirQualityMeasurement &measurement) {
    if(!this->isEnabled) {
      // Try to find BME280.
      setup();

      // If still not found, return.
      if(!this->isEnabled) {
        Serial.println(F("error: could not find SGP30 sensor."));
        return false;
      }

      this->sgp.IAQmeasure();
    }

    Serial.print(F("sensor SGP30: measuring "));

    if(this->isEnabled) {
      if(this->humidity > 0) {
        this->sgp.setHumidity(this->humidity);
      }

      unsigned long timeout = millis();
      while (!this->sgp.IAQmeasure()) {
        if(millis() - timeout > kMeasurementTimeout) {
          Serial.println(F("error: timeout measuring air quality."));
          this->isEnabled = false;
          return false;
        }

        Serial.print(F("."));
        delay(50);
      }
      Serial.println(F(" done."));

      measurement.eCo2 = sgp.eCO2;
      measurement.tVoc = sgp.TVOC;
    }

    return true;
  }
}
