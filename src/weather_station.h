#include <SSD1306Spi.h>
#include <OLEDDisplayUi.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "ui/weather_display.h"
#include "time/time_client.h"
#include "wunderground/client.h"
#include "wunderground/Conditions.h"
#include "wireless/wifi_manager.h"
#include "environmental/sensor.h"
#include "environmental/airquality.h"
#include "storage/persistent.h"

#ifndef WEATHER_STATION_H_
#define WEATHER_STATION_H_

class WeatherStation {
public:
  WeatherStation(WiFiManager& wifiManager);

  void setup();
  void loop();

private:
  static const int kSensorMeasurementInterval = 5000;
  static const int kMaxNTPTimeRetry = 3;
  static const char kTVOCKey[] PROGMEM;
  static const char kECO2Key[] PROGMEM;

  Storage::Persistent store;
  TwoWire w0;
  WiFiManager& wifiManager;
  wunderground::Client weatherClient;
  WiFiClientSecure wifiClient;
  PubSubClient mqttClient;
  SSD1306Spi display;
  OLEDDisplayUi ui;
  TimeClient timeClient;
  environmental::Measurement measurement;
  environmental::AirQualityMeasurement airQualityMeasurement;
  wunderground::Conditions conditions;
  WeatherDisplay weatherDisplay;
  environmental::Sensor sensor;
  environmental::AirQuality airQuality;
  bool didSetTime = false;
  bool didUpdateWeather = false;
  bool isUpdatingFirmware = false;
  bool didMeasureTemperature = false;
  bool didMeasureAirQuality = false;
  unsigned long lastSensorMeasurement = millis() + kSensorMeasurementInterval + 1;
  hw_timer_t * shortUpdateTimer;
  hw_timer_t * mediumUpdateTimer;
  hw_timer_t * longUpdateTimer;
  static volatile SemaphoreHandle_t ShortIntervalTimerSemaphore;
  static volatile SemaphoreHandle_t MediumIntervalTimerSemaphore;
  static volatile SemaphoreHandle_t LongIntervalTimerSemaphore;
  static volatile SemaphoreHandle_t ButtonPressSemaphore;

  bool mqttIsEnabled = false;
  bool wantsToPushTemperature = false;
  bool wantsToUpdateWeather = false;
  bool wantsToUpdateTime = false;

  void storeAirQualityBaseine();
  void onConnected();
  uint8_t backgroundTaskLoop();
  static void StaticBackgroundTask(void* parameter);
  static void IRAM_ATTR OnShortIntervalTimer();
  static void IRAM_ATTR OnMediumIntervalTimer();
  static void IRAM_ATTR OnLongIntervalTimer();
  static void OnButtonPressInterrupt();
};

#endif // WEATHER_STATION_H_
