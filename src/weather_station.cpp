#include "weather_station.h"
#include "tools/timer.h"
#include "config.h"
#include <esp_log.h>

volatile SemaphoreHandle_t WeatherStation::SuperShortIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::ShortIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::MediumIntervalTimerSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WeatherStation::LongIntervalTimerSemaphore = xSemaphoreCreateBinary();

const char WeatherStation::kTVOCKey[] PROGMEM = "ws.aq.tvoc";
const char WeatherStation::kECO2Key[] PROGMEM = "ws.aq.eco2";

static const char LogTag[] PROGMEM = "WeatherStation";

WeatherStation::WeatherStation(WiFiManager& wifiManager)
  : w0(0)
    , spi(VSPI)
    , mqttPublisher(wifiClient)
    , wifiManager(wifiManager)
    , display(WSConfig::kSpiResetPin, WSConfig::kSpiDcPin, WSConfig::kSpiCsPin, spi)
    , weatherDisplay(display, ntpClient, conditions, environmentMeasurement, airQualityMeasurement)
    , wunderground(WSConfig::kWundergroundApiKey, WSConfig::kWundergroundLanguage, WSConfig::kWundergroundLocation)
    , environmentSensor(w0)
    , airQualitySensor(w0) {}

WeatherStation::~WeatherStation() {}

void WeatherStation::setup() {
  this->w0.begin(WSConfig::kI2cSdaPin, WSConfig::kI2cSclPin, 400000);

  this->mqttPublisher.setup(WSConfig::kMqttBroker,
    WSConfig::kMqttBrokerPort,
    WSConfig::kMqttBrokerUsername,
    WSConfig::kMqttBrokerPassword);

  this->environmentSensor.setup();
  this->airQualitySensor.setup();

  this->wifiManager.onConnected(std::bind(&WeatherStation::onConnectedToWireless, this));

  this->weatherDisplay.setup();

  this->ntpClient.setup(WSConfig::kNtpServerName);

  this->store.loadConfig();

  Sensors::AirQuality::Measurement aqBaselineMeasurement = { 0, 0 };
  this->store.get(kTVOCKey, aqBaselineMeasurement.tVoc);
  this->store.get(kECO2Key, aqBaselineMeasurement.eCo2);

  if(aqBaselineMeasurement.eCo2 > 0 && aqBaselineMeasurement.tVoc > 0) {
    ESP_LOGI(LogTask, "air quality baseline from EEPROM: eCO2=0x%02X ppm, TVOC=0x%02X ppb", aqBaselineMeasurement.eCo2, aqBaselineMeasurement.tVoc);

    this->airQualitySensor.setBaseline(aqBaselineMeasurement);
  }

  static const char kBackgroundTaskName[] PROGMEM = "WeatherStation::StaticBackgroundTask";

  xTaskCreatePinnedToCore(
    WeatherStation::StaticDisplayTask,   /* Task function. */
    kBackgroundTaskName,                    /* name of task. */
    16384,                                  /* Stack size of task */
    this,                                   /* parameter of the task */
    1,                                      /* priority of the task */
    NULL,                                   /* Task handle to keep track of created task */
    0);                                     /* pin task to core 0 */

  this->superShortUpdateTimer = Timer::CreateTimer(&WeatherStation::OnSuperShortIntervalTimer, 3, 10000000, true, 80);
  this->shortUpdateTimer = Timer::CreateTimer(&WeatherStation::OnShortIntervalTimer, 0, 60000000, true, 80);
  this->mediumUpdateTimer = Timer::CreateTimer(&WeatherStation::OnMediumIntervalTimer, 1, 300000000, true, 80);
  this->longUpdateTimer = Timer::CreateTimer(&WeatherStation::OnLongIntervalTimer, 2, 3600000000, true, 80);

  this->tasks = WeatherStationTasks::eIdle;

  //
  spi.setFrequency(10000000);
}

void WeatherStation::onConnectedToWireless() {
  Serial.println(F("info: weather station: got interwebz!"));

  // Force updating just everything :-)
  this->tasks = WeatherStationTasks::eUpdateDateAndTime
    | WeatherStationTasks::eUpdateWeatherReport
    | WeatherStationTasks::eUpdateEnvironmentSensor
    | WeatherStationTasks::eUpdateAirQualitySensor
    | WeatherStationTasks::ePushTemperature;
}

void WeatherStation::loop() {
  this->wifiManager.loop();

  int8_t wiFiQuality = 0;
  WiFiManager::GetWifiQuality(wiFiQuality);

  this->weatherDisplay.setWifiQuality(wiFiQuality);

  if (xSemaphoreTake(WeatherStation::SuperShortIntervalTimerSemaphore, 0) == pdTRUE) {
    // ESP_LOGI(LogTag, "super short timer fired, running on core %d.", xPortGetCoreID());

    this->tasks |= WeatherStationTasks::eUpdateEnvironmentSensor;
    this->tasks |= WeatherStationTasks::eUpdateAirQualitySensor;
  }

  if (xSemaphoreTake(WeatherStation::ShortIntervalTimerSemaphore, 0) == pdTRUE) {
    // ESP_LOGI(LogTag, "short timer fired, running on core %d.", xPortGetCoreID());

    ESP_LOGI(LogTag, "free heap: %d bytes.", ESP.getFreeHeap());

    this->tasks |= WeatherStationTasks::ePushTemperature;
  }

  if (xSemaphoreTake(WeatherStation::MediumIntervalTimerSemaphore, 0) == pdTRUE) {
    // ESP_LOGI(LogTag, "timer fired, running on core %d,", xPortGetCoreID());

    this->tasks |= WeatherStationTasks::eUpdateWeatherReport;
  }

  if (xSemaphoreTake(WeatherStation::LongIntervalTimerSemaphore, 0) == pdTRUE) {
    ESP_LOGI(LogTag, "slow timer fired, running on core %d.", xPortGetCoreID());

    this->tasks |= WeatherStationTasks::eUpdateDateAndTime;
  }

  if(this->tasks & WeatherStationTasks::eUpdateEnvironmentSensor) {
    this->tasks &= ~WeatherStationTasks::eUpdateEnvironmentSensor;

    Sensors::Environment::Measurement measurement;

    // If it didn't succeed, show last temperature.
    bool success = this->environmentSensor.measure(measurement);
    if(success) {
      this->environmentMeasurement = measurement;

      Serial.print(F("weather station: read environmental sensor: temperature="));
      Serial.print(this->environmentMeasurement.temperature);
      Serial.print(F(" pressure="));
      Serial.print(this->environmentMeasurement.pressure);
      Serial.print(F(" humidity="));
      Serial.println(this->environmentMeasurement.humidity);

      // Improves quality of CO2 measurement.
      this->airQualitySensor.setHumidity(this->environmentMeasurement.humidity);
      this->weatherDisplay.addFrame(Frames::eIndoorSensors);
    }
  }

  if(this->tasks & WeatherStationTasks::eUpdateAirQualitySensor) {
    this->tasks &= ~WeatherStationTasks::eUpdateAirQualitySensor;

    Sensors::AirQuality::Measurement airQualityMeasurement;
    bool success = this->airQualitySensor.measure(airQualityMeasurement);
    if(success) {
      this->airQualityMeasurement = airQualityMeasurement;

      Serial.print(F("weather station: read airquality sensor: eCO2="));
      Serial.print(this->airQualityMeasurement.eCo2);
      Serial.print(F(" ppm TVOC="));
      Serial.print(this->airQualityMeasurement.tVoc);
      Serial.println(F(" ppb"));
    }
  }

  if(!this->wifiManager.isConnected()) {
    delay(1000);

    return;
  }

  this->mqttPublisher.loop();

  if(this->tasks & WeatherStationTasks::eUpdateDateAndTime) {
    this->tasks &= ~WeatherStationTasks::eUpdateDateAndTime;

    ESP_LOGI(LogTag, "updating date and time (%d) on core %d...", this->tasks, xPortGetCoreID());

    this->ntpClient.update([this](bool success) {
      if(success) {
        String currentTime;
        this->ntpClient.getFormattedTime(currentTime);
        Serial.print(F("info: station: current time: "));
        Serial.println(currentTime);

        this->weatherDisplay.addFrame(Frames::eTimeAndDate);
      } else {
        ESP_LOGE(LogTag, "could not update time and date.");
        this->tasks |= WeatherStationTasks::eUpdateDateAndTime;
      }
    });
  }

  if(this->tasks & WeatherStationTasks::eUpdateWeatherReport) {
    this->tasks &= ~WeatherStationTasks::eUpdateWeatherReport;

    ESP_LOGI(LogTag, "updating weather report...");

    this->wunderground.update([this](bool success, wunderground::Conditions& conditions) {
      ESP_LOGI(LogTag, "updating weather report callback (%d).", success);

      // Retry
      if(success) {
        this->conditions = conditions;
        this->weatherDisplay.addFrame(Frames::eWeatherReport);
      } else {
        ESP_LOGE(LogTag, "failed updating weather report, retry.");
        this->tasks |= WeatherStationTasks::eUpdateWeatherReport;
      }
    });
  }

  if(this->tasks & WeatherStationTasks::ePushTemperature
    && (this->environmentSensor.enabled() || this->airQualitySensor.enabled())) {
    this->tasks &= ~WeatherStationTasks::ePushTemperature;

    ESP_LOGI(LogTag, "environment sensor enabled: %d, air quality sensor enabled: %d.", this->environmentSensor.enabled(), this->airQualitySensor.enabled());
    ESP_LOGI(LogTag, "pushing temperature to MQTT broker.");

    StaticJsonBuffer<512> jsonBuffer;  
    unsigned long unixTime = 0;
    bool success = this->ntpClient.getUnixTime(unixTime);
    if(success){      
      JsonObject& root = jsonBuffer.createObject();
      root[F("time")] = unixTime;
    
      if(this->environmentSensor.enabled()) {
        root[F("temperature")] = this->environmentMeasurement.temperature;
        root[F("humidity")] = this->environmentMeasurement.humidity;
        root[F("pressure")] = this->environmentMeasurement.pressure;
      }
    
      if(this->airQualitySensor.enabled()) {
        root[F("eco2")] = this->airQualityMeasurement.eCo2;
        root[F("tvoc")] = this->airQualityMeasurement.tVoc;
      }
    
      String msg;
      root.printTo(msg);

      this->mqttPublisher.publish(WSConfig::kMqttTopicName, msg, [this](bool success) {
        ESP_LOGI(LogTag, "pushed data to MQTT broker (%d).", success);
      });
    } else {
      // Time is not yet set, so retry pushing temperature.
      this->tasks |= WeatherStationTasks::ePushTemperature;
    }
  }

  // @ToDo: needed?
  delay(10);
}

bool WeatherStation::displayTaskLoop() {
  int remainingTimeBudget = this->weatherDisplay.update();
  if(remainingTimeBudget > 0) {
    delay(remainingTimeBudget);
  }

  return true;
}

void WeatherStation::OnSuperShortIntervalTimer() {
  xSemaphoreGiveFromISR(WeatherStation::SuperShortIntervalTimerSemaphore, NULL);
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

void WeatherStation::StaticDisplayTask(void* parameter) {
  if (parameter != NULL) {
    WeatherStation *self = reinterpret_cast<WeatherStation *>(parameter);

    if(self != NULL) {
      for(;;) {
        micros();
        if(!self->displayTaskLoop()) {
          ESP_LOGI(LogTag, "stopping display task.");
          break;
        }
      }
    }
  }

  vTaskDelete( NULL );
}
