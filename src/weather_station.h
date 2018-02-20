#include <SSD1306Wire.h>
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

  TwoWire w0;
  TwoWire w1;

  WiFiManager& wifiManager;
  wunderground::Client weatherClient;
  WiFiClientSecure wifiClient;
  PubSubClient mqttClient;
  SSD1306Wire display;
  OLEDDisplayUi ui;
  TimeClient timeClient;
  environmental::Measurement measurement;
  wunderground::Conditions conditions;
  WeatherDisplay weatherDisplay;
  environmental::Sensor sensor;
  bool didSetTime = false;
  bool didUpdateWeather = false;
  bool isUpdatingFirmware = false;
  unsigned long lastSensorMeasurement = millis() + kSensorMeasurementInterval + 1;
  hw_timer_t * shortUpdateTimer;
  hw_timer_t * mediumUpdateTimer;
  hw_timer_t * longUpdateTimer;
  static volatile SemaphoreHandle_t ShortIntervalTimerSemaphore;
  static volatile SemaphoreHandle_t MediumIntervalTimerSemaphore;
  static volatile SemaphoreHandle_t LongIntervalTimerSemaphore;

  bool wantsToPushTemperature = false;
  bool wantsToUpdateWeather = false;
  bool wantsToUpdateTime = false;

  void onConnected();
  uint8_t backgroundTaskLoop();
  static void StaticBackgroundTask(void* parameter);
  static void IRAM_ATTR OnShortIntervalTimer();
  static void IRAM_ATTR OnMediumIntervalTimer();
  static void IRAM_ATTR OnLongIntervalTimer();
};

#endif // WEATHER_STATION_H_
