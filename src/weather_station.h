#ifndef _STATION_H_
#define _STATION_H_

#include <SSD1306Spi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include "ui/weather_display.h"
#include "wunderground/client.h"
#include "wunderground/conditions.h"
#include "ntp/ntp_client.h"
#include "wireless/wifi_manager.h"
#include "sensors/environment.h"
#include "sensors/air_quality.h"
#include "storage/persistent.h"
#include "publisher/publisher.h"

namespace WeatherStationTasks {
  enum {
    eIdle = 0,
    eUpdateDateAndTime = 1 << 0,
    eUpdateWeatherReport = 1 << 1,
    eUpdateEnvironmentSensor = 1 << 2,
    eUpdateAirQualitySensor = 1 << 3,
    ePushTemperature = 1 << 4
  };

  typedef uint16_t Tasks;
};

class WeatherStation {
  private:
    static const char kTVOCKey[] PROGMEM;
    static const char kECO2Key[] PROGMEM;

    WeatherStationTasks::Tasks tasks;

    Storage::Persistent store;

    TwoWire w0;
    SPIClass spi;
    Mqtt::Publisher mqttPublisher;
    WiFiManager& wifiManager;
    SSD1306Spi display;
    WeatherDisplay weatherDisplay;
    wunderground::Client wunderground;
    Sensors::Environment environmentSensor;
    Sensors::AirQuality airQualitySensor;

    WiFiClientSecure wifiClient;
    wunderground::Conditions conditions;
    Ntp::Client ntpClient;
    Sensors::Environment::Measurement environmentMeasurement;
    Sensors::AirQuality::Measurement airQualityMeasurement;
    hw_timer_t* superShortUpdateTimer;
    hw_timer_t* shortUpdateTimer;
    hw_timer_t* mediumUpdateTimer;
    hw_timer_t* longUpdateTimer;

    bool displayTaskLoop();
    void onConnectedToWireless();

    static volatile SemaphoreHandle_t SuperShortIntervalTimerSemaphore;
    static volatile SemaphoreHandle_t ShortIntervalTimerSemaphore;
    static volatile SemaphoreHandle_t MediumIntervalTimerSemaphore;
    static volatile SemaphoreHandle_t LongIntervalTimerSemaphore;

    static void StaticDisplayTask(void* parameter);
    static void IRAM_ATTR OnSuperShortIntervalTimer();
    static void IRAM_ATTR OnShortIntervalTimer();
    static void IRAM_ATTR OnMediumIntervalTimer();
    static void IRAM_ATTR OnLongIntervalTimer();

  public:
    WeatherStation(WiFiManager& wifiManager);
    ~WeatherStation();
    void setup();
    void loop();
};

#endif // _STATION_H_
