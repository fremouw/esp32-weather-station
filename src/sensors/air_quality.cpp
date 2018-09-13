#include "air_quality.h"
#include <esp_log.h>

static const char LogTag[] PROGMEM = "AirQuality";

namespace Sensors {
  void AirQuality::setup() {
    ESP_LOGI(LogTag, "trying to find SGP30...");

    this->isEnabled = this->sgp.begin(&wire);

    if(this->isEnabled) {
      this->isEnabled = this->sgp.IAQinit();
      if(this->isEnabled) {
        ESP_LOGI(LogTag, "found SGP30 serial #%02X%02X%02X.", this->sgp.serialnumber[0], this->sgp.serialnumber[1], this->sgp.serialnumber[2]);

        this->humidity = 0;

        if(this->baseline.eCo2 > 0 && this->baseline.tVoc > 0) {
          this->sgp.setIAQBaseline(this->baseline.eCo2, this->baseline.tVoc);
        }
      } else {
        ESP_LOGE(LogTag, "SGP30 not found.");
      }
    } else {
      ESP_LOGE(LogTag, "SGP30 not found.");
    }
  }

  bool AirQuality::enabled() {
    return this->isEnabled;
  }

  void AirQuality::setHumidity(uint32_t humidity) {
    this->humidity = humidity;

    this->sgp.setHumidity(this->humidity);
  }

  bool AirQuality::getBaseline(Measurement& measurement) {
    if(!this->isEnabled) {
      return false;
    }

    uint16_t tvoc;
    uint16_t eco2;

    if(sgp.getIAQBaseline(&eco2, &tvoc)) {
      ESP_LOGI(LogTag, "baseline values: eCO2: 0x%02X ppm, TVOC: 0x%02X ppb.", eco2, tvoc);

      measurement.tVoc = tvoc;
      measurement.eCo2 = eco2;

      return true;
    }

    return false;
  }

  void AirQuality::setBaseline(const Measurement& measurement) {
    this->baseline = baseline;

    if(this->isEnabled) {
      this->sgp.setIAQBaseline(measurement.eCo2, measurement.tVoc);
    }
  }

  bool AirQuality::measure(Measurement& measurement) {
    if(!this->isEnabled) {
      // Try to find BME280.
      setup();

      // If still not found, return.
      if(!this->isEnabled) {
        return false;
      }

      this->sgp.IAQmeasure();
    }

    ESP_LOGI(LogTag, "SGP30 start measuring.");

    if(this->isEnabled) {
      // if(this->humidity > 0) {
      //   this->sgp.setHumidity(this->humidity);
      // }

      unsigned long timeout = millis();
      while (!this->sgp.IAQmeasure()) {
        if(millis() - timeout > kMeasurementTimeout) {
          ESP_LOGE(LogTag, "timeout measuring temperature.");
          this->isEnabled = false;
          return false;
        }

        Serial.print(F("."));
        yield();
      }
      ESP_LOGI(LogTag, "done measuring.");

      measurement.eCo2 = sgp.eCO2;
      measurement.tVoc = sgp.TVOC;
    }

    return true;
  }
}
