#include "weather_station.h"
#include "tools/timer.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "config.h"

volatile SemaphoreHandle_t WeatherStation::ShortIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::MediumIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::LongIntervalTimerSemaphore = xSemaphoreCreateBinary();

WeatherStation::WeatherStation(WiFiManager& _wifiManager): w0(0), w1(1), sensor(w1), wifiManager(_wifiManager), display(WSConfig::kI2cDisplayAddress, w0), ui(&display), weatherClient(WSConfig::kWundergroundApiKey, WSConfig::kWundergroundLanguage, WSConfig::kWundergroundLocation), weatherDisplay(display, ui, timeClient, conditions, measurement), mqttClient(wifiClient) {

  this->measurement.temperature = -1.0;
  this->measurement.humidity    = -1.0;
  this->measurement.pressure    = -1.0;
}

void WeatherStation::setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  this->w0.begin(WSConfig::kI2cDisplaySdaPin, WSConfig::kI2cDisplaySclPin, 700000);
  this->w1.begin(WSConfig::kI2cTemperatureSdaPin, WSConfig::kI2cTemperatureSclPin, 100000);

  this->weatherDisplay.setup();

  this->weatherDisplay.setShowBootScreen(true);

  this->timeClient.setup(WSConfig::kNtpServerName);

  this->sensor.setup();

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

  // ArduinoOTA.begin();
  //
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
}

void WeatherStation::loop() {
  if(this->isUpdatingFirmware) {
    return;
  }

  int remainingTimeBudget = weatherDisplay.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.

    // Read in UI loop, as the sensor shares the I2C bus with the display.
    if(millis() - this->lastSensorMeasurement > kSensorMeasurementInterval) {
      // Serial.print("weather station::loop running on core ");
      // Serial.println(xPortGetCoreID());

      // If it didn't succeed, show last temperature.
      this->didMeasureTemperature = this->sensor.measure(this->measurement);
      if(this->didMeasureTemperature) {
        Serial.print("weather station: read environmental sensor: temperature=");
        Serial.print(this->measurement.temperature);
        Serial.print(" pressure=");
        Serial.print(this->measurement.pressure);
        Serial.print(" humidity=");
        Serial.println(this->measurement.humidity);
      }

      this->lastSensorMeasurement = millis();
    }

    int8_t wiFiQuality = 0;
    WiFiManager::GetWifiQuality(wiFiQuality);

    this->weatherDisplay.setWifiQuality(wiFiQuality);

    delay(remainingTimeBudget);
  }
}

void WeatherStation::onConnected() {
  Serial.println("weatherstation: got interwebz!");

  // Force update.
  this->wantsToPushTemperature = this->mqttIsEnabled;
  this->wantsToUpdateTime = true;
  this->wantsToUpdateWeather = true;
}

uint8_t WeatherStation::backgroundTaskLoop() {
  // Needs to be here?
  //ArduinoOTA.handle();

  this->wifiManager.loop();

  if(this->isUpdatingFirmware) {
    return -1;
  }

  if(!this->wifiManager.isConnected()) {
    return 0;
  }

  this->mqttClient.loop();

  if (xSemaphoreTake(WeatherStation::ShortIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print("Short fired. WeatherStation::backgroundTaskLoop running on core ");
    Serial.println(xPortGetCoreID());

    if(this->mqttIsEnabled) {
      this->wantsToPushTemperature = true;
    }
  }

  if (xSemaphoreTake(WeatherStation::MediumIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print("Timer fired. WeatherStation::backgroundTaskLoop running on core ");
    Serial.println(xPortGetCoreID());

    this->wantsToUpdateWeather = true;
  }

  if (xSemaphoreTake(WeatherStation::LongIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print("Slow Timer fired. WeatherStation::backgroundTaskLoop running on core ");
    Serial.println(xPortGetCoreID());

    this->wantsToUpdateTime = true;
  }

  if(this->wantsToUpdateTime) {
    this->didSetTime = timeClient.update();

    if(this->didSetTime) {
      this->wantsToUpdateTime = false;
    }
  } else if(this->wantsToUpdateWeather) {
    this->wantsToUpdateWeather = false;

    //
    this->weatherClient.update([this](bool success,
                                wunderground::Conditions& _conditions) mutable {
        Serial.println("weatherClient.update");

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
          Serial.println("debug: connected to MQTT broker.");
        }
    }

    if (this->mqttClient.connected()) {
      StaticJsonBuffer<200> jsonBuffer;

      unsigned long unixTime = 0;
      this->timeClient.getUnixTime(unixTime);

      JsonObject& root = jsonBuffer.createObject();
      root["temperature"] = this->measurement.temperature;
      root["humidity"] = this->measurement.humidity;
      root["pressure"] = this->measurement.pressure;
      root["time"] = unixTime;

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
          Serial.println("weather station: stopping background task.");
          break;
        }
      }
    }
  }

  vTaskDelete( NULL );
}
