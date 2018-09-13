#include "environment.h"
#include <Wire.h>
#include <esp_log.h>

static const char LogTag[] PROGMEM = "EnvironmentSensor";

namespace Sensors {
  void Environment::setup() {
    uint8_t chipId = this->bme280.readChipId();
    if(chipId == 0xff || chipId == 0x0) {
      ESP_LOGE(LogTag, "BME280 not found.");
      this->isEnabled = false;
      return;
    } else {
      // find the chip ID out just for fun
      ESP_LOGI(LogTag, "BME280 chip identifier=0x%02X.", chipId);
      this->isEnabled = true;
    }

    this->bme280.readCompensationParams();

    // BME280.writeFilterCoefficient(fc_off);
    this->bme280.writeFilterCoefficient(fc_16);      // IIR Filter coefficient 16

    this->bme280.writeOversamplingPressure(os2x);
    this->bme280.writeOversamplingTemperature(os2x);
    this->bme280.writeOversamplingHumidity(os1x);

    // BME280.writeMode(smForced);

    this->bme280.writeStandbyTime(tsb_1000ms);        // tsb = 0.5ms
    // BME280.writeOversamplingPressure(os16x);    // pressure x16
    // BME280.writeOversamplingTemperature(os2x);  // temperature x2
    // BME280.writeOversamplingHumidity(os1x);     // humidity x1

    this->bme280.writeMode(smNormal);
  }

  bool Environment::enabled() {
    return this->isEnabled;
  }

  bool Environment::measure(Measurement &measurement) {
    if(!this->isEnabled) {
      // Try to find BME280.
      setup();

      // If still not found, return.
      if(!this->isEnabled) {
        return false;
      }
    }

    ESP_LOGI(LogTag, "BME280 start measuring.");

    unsigned long timeout = millis();
    while (this->bme280.isMeasuring()) {
      if(millis() - timeout > kMeasurementTimeout) {
        ESP_LOGE(LogTag, "timeout measuring temperature.");
        this->isEnabled = false;
        return false;
      }
      yield();
    }
    ESP_LOGI(LogTag, "done measuring.");

    this->bme280.readMeasurements();

    measurement.temperature = this->bme280.getTemperatureMostAccurate();
    measurement.pressure    = this->bme280.getPressureMostAccurate();
    measurement.humidity    = this->bme280.getHumidityMostAccurate();

    return true;
  }
}
