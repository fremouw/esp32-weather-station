#include "weather_station.h"
#include "tools/timer.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#define I2C_DISPLAY_ADDRESS 0x3c
#define I2C_SDA 4
#define I2C_SCL 5
// #define I2C_SDA 21
// #define I2C_SCL 22


#define HOSTNAME "MFR-WEATHERSTATION-"

// const char ntpServerName[] PROGMEM = "europe.pool.ntp.org";
// const char ntpServerName[] PROGMEM = "192.168.243.153";
const char ntpServerName[] PROGMEM = "capybara.lan";
const char WUNDERGRROUND_API_KEY[] PROGMEM  = "key";
const char WUNDERGRROUND_LANGUAGE[] PROGMEM = "EN";
const char WUNDERGROUND_LATLON[] PROGMEM    = "52.3680491,4.861764";

const char MQTT_BROKER[] PROGMEM = "mqtt.mrtn.io";
const uint16_t MQTT_BROKER_PORT = 8883;

volatile SemaphoreHandle_t WeatherStation::ShortIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::MediumIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::LongIntervalTimerSemaphore = xSemaphoreCreateBinary();

WeatherStation::WeatherStation(WiFiManager& _wifiManager): wifiManager(_wifiManager), display(I2C_DISPLAY_ADDRESS, I2C_SDA, I2C_SCL), ui(&display), weatherClient(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_LATLON), weatherDisplay(display, ui, timeClient, conditions, measurement), mqttClient(wifiClient) {
}

void WeatherStation::setup() {
  weatherDisplay.setup();

  weatherDisplay.setShowBootScreen(true);

  timeClient.setup(ntpServerName);

  sensor.setup();

  xTaskCreatePinnedToCore(
    WeatherStation::StaticBackgroundTask,   /* Task function. */
    "WeatherStation::StaticBackgroundTask", /* name of task. */
    8192,                                   /* Stack size of task */
    this,                                   /* parameter of the task */
    1,                                      /* priority of the task */
    NULL,                                   /* Task handle to keep track of created task */
    0);                                     /* pin task to core 1 */

  shortUpdateTimer = Timer::CreateTimer(&WeatherStation::OnShortIntervalTimer, 0, 60000000, true, 80);
  mediumUpdateTimer = Timer::CreateTimer(&WeatherStation::OnMediumIntervalTimer, 1, 300000000, true, 80);
  longUpdateTimer = Timer::CreateTimer(&WeatherStation::OnLongIntervalTimer, 2, 3600000000, true, 80);

  wifiManager.onConnected(std::bind(&WeatherStation::onConnected, this));

  ArduinoOTA.onStart([this]() {
    Serial.print(F("OTA update: "));
    this->isUpdatingFirmware = true;
    this->weatherDisplay.setShowUpdateScreen(true);
  });

  ArduinoOTA.onError([this](ota_error_t error) {
    Serial.println(F(" error."));
    this->weatherDisplay.setShowUpdateScreen(false);
    this->isUpdatingFirmware = false;
  });

  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
    Serial.print(F("."));
    this->weatherDisplay.drawOtaProgress(progress, total);
  });

  ArduinoOTA.onEnd([this]() {
    Serial.println(F(" done."));
    this->weatherDisplay.setShowUpdateScreen(false);
    this->isUpdatingFirmware = false;
  });
}

void WeatherStation::loop() {
  // Needs to be here?
  ArduinoOTA.handle();

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

      this->sensor.measure(measurement);

      Serial.print("weather station: read environmental sensor: temperature=");
      Serial.print(measurement.temperature);
      Serial.print(" pressure=");
      Serial.print(measurement.pressure);
      Serial.print(" humidity=");
      Serial.println(measurement.humidity);

      this->lastSensorMeasurement = millis();
    }

    int8_t wiFiQuality = 0;
    WiFiManager::GetWifiQuality(wiFiQuality);

    weatherDisplay.setWifiQuality(wiFiQuality);

    delay(remainingTimeBudget);
  }
}

void WeatherStation::onConnected() {
  Serial.println("weatherstation: got interwebz!");
  // Force update.
  wantsToPushTemperature = true;
  wantsToUpdateTime = true;
  wantsToUpdateWeather = true;
}

uint8_t WeatherStation::backgroundTaskLoop() {
  if(this->isUpdatingFirmware) {
    return -1;
  }

  wifiManager.loop();

  mqttClient.loop();

  if (xSemaphoreTake(WeatherStation::ShortIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print("Short fired. WeatherStation::backgroundTaskLoop running on core ");
    Serial.println(xPortGetCoreID());

    wantsToPushTemperature = true;
  }

  if (xSemaphoreTake(WeatherStation::MediumIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print("Timer fired. WeatherStation::backgroundTaskLoop running on core ");
    Serial.println(xPortGetCoreID());

    wantsToUpdateWeather = true;
  }

  if (xSemaphoreTake(WeatherStation::LongIntervalTimerSemaphore, 0) == pdTRUE) {
    Serial.print("Slow Timer fired. WeatherStation::backgroundTaskLoop running on core ");
    Serial.println(xPortGetCoreID());

    wantsToUpdateTime = true;
  }

  if(wantsToUpdateTime) {
    this->didSetTime = timeClient.update();

    if(this->didSetTime) {
      wantsToUpdateTime = false;
    }
  } else if(wantsToUpdateWeather) {
    //
    weatherClient.update([this](bool success,
                                wunderground::Conditions& _conditions) mutable {
        Serial.println("weatherClient.update");

        if (success) {
          _conditions.printConditions();
          this->conditions = _conditions;
          this->didUpdateWeather = true;
        }
    });

    wantsToUpdateWeather = false;
  } else if(wantsToPushTemperature) {
    // Set only once?
    mqttClient.setServer(MQTT_BROKER, MQTT_BROKER_PORT);

    if (!mqttClient.connected()) {
        Serial.println(F("debug: not connected to MQTT broker."));

        if (mqttClient.connect(MQTT_BROKER, "username", "pass")) {
          Serial.println("debug: connected to MQTT broker.");
        }
    }

    if (mqttClient.connected()) {
      StaticJsonBuffer<200> jsonBuffer;

      unsigned long unixTime = 0;
      this->timeClient.getUnixTime(unixTime);

      JsonObject& root = jsonBuffer.createObject();
      root["temperature"] = measurement.temperature;
      root["humidity"] = measurement.humidity;
      root["pressure"] = measurement.pressure;
      root["time"] = unixTime;

      String msg;
      root.printTo(msg);

      Serial.println(msg);
      mqttClient.publish("Sensor/Temperature/1", msg.c_str(), true);
      mqttClient.disconnect();
      wantsToPushTemperature = false;
    }
  }

  if(this->didSetTime && this->didUpdateWeather) {
    weatherDisplay.setShowBootScreen(false);
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
