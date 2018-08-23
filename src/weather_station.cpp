#include "weather_station.h"
#include "tools/timer.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <cmath>
#include "config.h"

const char WeatherStation::kTVOCKey[] PROGMEM = "ws.aq.tvoc";
const char WeatherStation::kECO2Key[] PROGMEM = "ws.aq.eco2";

volatile SemaphoreHandle_t WeatherStation::ShortIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::MediumIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::LongIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::ButtonPressSemaphore = xSemaphoreCreateBinary();

WeatherStation::WeatherStation(WiFiManager& _wifiManager): w0(0), sensor(w0), wifiManager(_wifiManager), display(WSConfig::kSpiResetPin, WSConfig::kSpiDcPin, WSConfig::kSpiCsPin), ui(&display), weatherClient(WSConfig::kWundergroundApiKey, WSConfig::kWundergroundLanguage, WSConfig::kWundergroundLocation), weatherDisplay(display, ui, timeClient, conditions, measurement, airQualityMeasurement), mqttClient(wifiClient), airQuality(w0) {

  this->measurement.temperature = -1.0;
  this->measurement.humidity    = -1.0;
  this->measurement.pressure    = -1.0;
}

void WeatherStation::setup() {
  pinMode(WSConfig::kButtonPin, INPUT_PULLDOWN);

  attachInterrupt(digitalPinToInterrupt(WSConfig::kButtonPin), WeatherStation::OnButtonPressInterrupt, FALLING);

  this->w0.begin(WSConfig::kI2cSdaPin, WSConfig::kI2cSclPin, 100000);

  this->weatherDisplay.setup();

  this->weatherDisplay.setShowBootScreen(true);

  this->timeClient.setup(WSConfig::kNtpServerName);

  this->sensor.setup();

  this->airQuality.setup();

  this->store.loadConfig();

  environmental::AirQualityMeasurement baselineMeasurement = { 0, 0 };
  this->store.get(kTVOCKey, baselineMeasurement.tVoc);
  this->store.get(kECO2Key, baselineMeasurement.eCo2);

  if(baselineMeasurement.eCo2 > 0 && baselineMeasurement.tVoc > 0) {
    Serial.print(F("info: get air quality baseline from EEPROM: eCO2=0x"));
    Serial.print(baselineMeasurement.eCo2, HEX);
    Serial.print(F(", TVOC=0x"));
    Serial.print(baselineMeasurement.tVoc, HEX);
    Serial.println(F(" ppb"));

    this->airQuality.setBaseline(baselineMeasurement);
  }

  xTaskCreatePinnedToCore(
    WeatherStation::StaticBackgroundTask,   /* Task function. */
    "WeatherStation::StaticBackgroundTask", /* name of task. */
    8192,                                   /* Stack size of task */
    this,                                   /* parameter of the task */
    1,                                      /* priority of the task */
    NULL,                                   /* Task handle to keep track of created task */
    0);                                     /* pin task to core 1 */

  if(strlen(WSConfig::kMqttBroker) > 0) {
    this->mqttIsEnabled = true;
  }

  this->shortUpdateTimer = Timer::CreateTimer(&WeatherStation::OnShortIntervalTimer, 0, 60000000, true, 80);
  this->mediumUpdateTimer = Timer::CreateTimer(&WeatherStation::OnMediumIntervalTimer, 1, 300000000, true, 80);
  // @ToDo: ok, maybe we don't need to synchronize the time every 5 minutes.
  // Check skew after 24 hours.
  this->longUpdateTimer = Timer::CreateTimer(&WeatherStation::OnLongIntervalTimer, 2, 3600000000, true, 80);

  this->wifiManager.onConnected(std::bind(&WeatherStation::onConnected, this));

  // ArduinoOTA.onStart([this]() {
  //   Serial.print(F("OTA update: "));
  //   this->isUpdatingFirmware = true;
  //   this->weatherDisplay.setShowUpdateScreen(true);
  // });
  //
  // ArduinoOTA.onError([this](ota_error_t error) {
  //   Serial.println(F(" error."));
  //   this->weatherDisplay.setShowUpdateScreen(false);
  //   this->isUpdatingFirmware = false;
  // });
  //
  // ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
  //   Serial.print(F("."));
  //   this->weatherDisplay.drawOtaProgress(progress, total);
  // });
  //
  // ArduinoOTA.onEnd([this]() {
  //   Serial.println(F(" done."));
  //   this->weatherDisplay.setShowUpdateScreen(false);
  //   this->isUpdatingFirmware = false;
  // });
  //
  // ArduinoOTA.begin();
}

void WeatherStation::loop() {
  if(this->isUpdatingFirmware) {
    return;
  }

  int remainingTimeBudget = weatherDisplay.update();

  if (remainingTimeBudget > 0) {
    if(millis() - this->lastSensorMeasurement > kSensorMeasurementInterval) {
      environmental::Measurement measurement;

      // If it didn't succeed, show last temperature.
      this->didMeasureTemperature = this->sensor.measure(measurement);
      if(this->didMeasureTemperature) {
        // if the temperature is changed by more then 100 degrees celcius,
        // it's probably an invalid reading, so skip.
        if(std::abs(this->measurement.temperature - measurement.temperature) < 100) {
          this->measurement = measurement;

          Serial.print(F("weather station: read environmental sensor: temperature="));
          Serial.print(this->measurement.temperature);
          Serial.print(F(" pressure="));
          Serial.print(this->measurement.pressure);
          Serial.print(F(" humidity="));
          Serial.println(this->measurement.humidity);

          // Improves quality of CO2 measurement.
          this->airQuality.setHumidity(this->measurement.humidity);
        } else {
          Serial.print(F("weather station: read invalid data: temperature="));
          Serial.print(measurement.temperature);
          Serial.print(F(" pressure="));
          Serial.print(measurement.pressure);
          Serial.print(F(" humidity="));
          Serial.println(measurement.humidity);
        }
      }

      environmental::AirQualityMeasurement airQualityMeasurement;
      this->didMeasureAirQuality = this->airQuality.measure(airQualityMeasurement);
      if(this->didMeasureAirQuality) {
        this->airQualityMeasurement = airQualityMeasurement;

        Serial.print(F("weather station: read airquality sensor: eCO2="));
        Serial.print(this->airQualityMeasurement.eCo2);
        Serial.print(F(" ppm TVOC="));
        Serial.print(this->airQualityMeasurement.tVoc);
        Serial.println(F(" ppb"));
      }

      this->lastSensorMeasurement = millis();
    }

    int8_t wiFiQuality = 0;
    WiFiManager::GetWifiQuality(wiFiQuality);

    this->weatherDisplay.setWifiQuality(wiFiQuality);

    delay(remainingTimeBudget);
  }
}

void WeatherStation::storeAirQualityBaseine() {
  Serial.println(F("WeatherStation::storeAirQualityBaseine"));

  environmental::AirQualityMeasurement baselineMeasurement;
  this->airQuality.getBaseline(baselineMeasurement);

  Serial.print(F("info: storing air quality baseline in EEPROM: eCO2=0x"));
  Serial.print(baselineMeasurement.eCo2, HEX);
  Serial.print(F(" ppm, TVOC=0x"));
  Serial.print(baselineMeasurement.tVoc, HEX);
  Serial.println(F(" ppb"));

  this->store.set(kTVOCKey, baselineMeasurement.tVoc);
  this->store.set(kECO2Key, baselineMeasurement.eCo2);

  //
  this->store.saveConfig();
}

void WeatherStation::onConnected() {
  Serial.println(F("weatherstation: got interwebz!"));

  // Force update.
  this->wantsToPushTemperature = this->mqttIsEnabled;
  this->wantsToUpdateTime = true;
  this->wantsToUpdateWeather = true;
}

uint8_t WeatherStation::backgroundTaskLoop() {
  // Needs to be here?
  // ArduinoOTA.handle();

  this->wifiManager.loop();

  if(this->isUpdatingFirmware) {
    return -1;
  }

  if(!this->wifiManager.isConnected()) {
    return 0;
  }

  this->mqttClient.loop();

  if (xSemaphoreTake(WeatherStation::ShortIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print(F("Short fired. WeatherStation::backgroundTaskLoop running on core "));
    Serial.println(xPortGetCoreID());

    if(this->mqttIsEnabled) {
      this->wantsToPushTemperature = true;
    }
  }

  if (xSemaphoreTake(WeatherStation::MediumIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print(F("Timer fired. WeatherStation::backgroundTaskLoop running on core "));
    Serial.println(xPortGetCoreID());

    this->wantsToUpdateWeather = true;
  }

  if (xSemaphoreTake(WeatherStation::LongIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print(F("Slow Timer fired. WeatherStation::backgroundTaskLoop running on core "));
    Serial.println(xPortGetCoreID());

    this->wantsToUpdateTime = true;
  }

  if (xSemaphoreTake(WeatherStation::ButtonPressSemaphore, 0) == pdTRUE) {
    Serial.print(F("OnButtonPressSemaphore. WeatherStation::backgroundTaskLoop running on core "));
    Serial.println(xPortGetCoreID());

    this->storeAirQualityBaseine();
  }

  if(this->wantsToUpdateTime) {
    this->timeClient.update([this](bool success) {
      this->didSetTime = success;

      if (success) {
          Serial.println(F("did update time."));

          this->wantsToUpdateTime = false;
      }
    });
  } else if(this->wantsToUpdateWeather) {
    this->wantsToUpdateWeather = false;

    //
    this->weatherClient.update([this](bool success,
                                wunderground::Conditions& _conditions) mutable {
        environmental::AirQualityMeasurement measurement;
        this->airQuality.getBaseline(measurement);

        Serial.println(F("weatherClient.update"));

        if (success) {
          _conditions.printConditions();
          this->conditions = _conditions;
          this->didUpdateWeather = true;
        } else {
          this->wantsToUpdateWeather = true;
        }
    });
  } else if(this->wantsToPushTemperature && this->didMeasureTemperature) {
    // Set only once?
    this->mqttClient.setServer(WSConfig::kMqttBroker, WSConfig::kMqttBrokerPort);

    if (!this->mqttClient.connected()) {
        Serial.println(F("debug: not connected to MQTT broker."));

        if (this->mqttClient.connect(WSConfig::kMqttBroker, WSConfig::kMqttBrokerUsername, WSConfig::kMqttBrokerPassword)) {
          Serial.println(F("debug: connected to MQTT broker."));
        }
    }

    if (this->mqttClient.connected()) {
      StaticJsonBuffer<200> jsonBuffer;

      unsigned long unixTime = 0;
      this->timeClient.getUnixTime(unixTime);

      JsonObject& root = jsonBuffer.createObject();
      root[F("temperature")] = this->measurement.temperature;
      root[F("humidity")] = this->measurement.humidity;
      root[F("pressure")] = this->measurement.pressure;
      root[F("eco2")] = this->airQualityMeasurement.eCo2;
      root[F("tvoc")] = this->airQualityMeasurement.tVoc;
      root[F("time")] = unixTime;

      String msg;
      root.printTo(msg);

      Serial.println(msg);
      this->mqttClient.publish(WSConfig::kMqttTopicName, msg.c_str(), true);
      this->mqttClient.disconnect();
      this->wantsToPushTemperature = false;
    }
  }

  if(this->didSetTime && this->didUpdateWeather) {
    this->weatherDisplay.setShowBootScreen(false);
  }

  delay(100);

  return 0;
}

void WeatherStation::OnShortIntervalTimer() {
  xSemaphoreGiveFromISR(WeatherStation::ShortIntervalTimerSemaphore, NULL);
}

void WeatherStation::OnMediumIntervalTimer() {
  xSemaphoreGiveFromISR(WeatherStation::MediumIntervalTimerSemaphore, NULL);
}

void WeatherStation::OnLongIntervalTimer() {
  xSemaphoreGiveFromISR(WeatherStation::LongIntervalTimerSemaphore, NULL);
}

void WeatherStation::StaticBackgroundTask(void* parameter) {
  if (parameter != NULL) {
    WeatherStation *self = static_cast<WeatherStation *>(parameter);

    if(self != NULL) {
      for(;;) {
        micros();
        if(self->backgroundTaskLoop() < 0) {
          Serial.println(F("weather station: stopping background task."));
          break;
        }
      }
    }
  }

  vTaskDelete( NULL );
}

void WeatherStation::OnButtonPressInterrupt() {
  Serial.println(F("debug: button pressed!"));

  static unsigned long timeout = millis();
  if(millis() - timeout < 3000) {
    return;
  }

  timeout = millis();
  xSemaphoreGiveFromISR(WeatherStation::ButtonPressSemaphore, NULL);
}
