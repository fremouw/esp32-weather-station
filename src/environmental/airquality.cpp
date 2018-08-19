#include "airquality.h"

namespace environmental {
  void AirQuality::setup() {
    this->isEnabled = this->sgp.begin(&wire);

    if(this->isEnabled) {
      this->sgp.IAQinit();

      Serial.print("Found SGP30 serial #");
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

  bool AirQuality::measure(environmental::AirQualityMeasurement &measurement) {
    if(!this->isEnabled) {
      // Try to find BME280.
      setup();

      // If still not found, return.
      if(!this->isEnabled) {
        Serial.println("error: could not find SGP30 sensor.");
        return false;
      }

      this->sgp.IAQmeasure();
    }

    Serial.print("sensor SGP30: measuring ");

    if(this->isEnabled) {
      if(this->humidity > 0) {
        this->sgp.setHumidity(this->humidity);
      }

      unsigned long timeout = millis();
      while (!this->sgp.IAQmeasure()) {
        if(millis() - timeout > kMeasurementTimeout) {
          Serial.println("error: timeout measuring air quality.");
          this->isEnabled = false;
          return false;
        }

        Serial.print(".");
        delay(50);
      }
      Serial.println(" done.");

      measurement.eCo2 = sgp.eCO2;
      measurement.tVoc = sgp.TVOC;

      uint16_t TVOC_base, eCO2_base;
      if(sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
        Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
        Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
      }
    }

    return true;
  }
}
