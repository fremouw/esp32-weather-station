#include <Arduino.h>
#include <WiFi.h>

#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_

typedef std::shared_ptr<std::function<void()>> WiFiEventHandler;

class WiFiManager {
  public:
    WiFiManager();

    void setup(const char* ssid, const char* password);
    void loop();
    bool isConnected();

    void onConnected(std::function<void()>);

    static void GetWifiQuality(int8_t& quality);

  private:
    String ssid;
    String password;
    uint8_t timer;

    unsigned long connectTimeout;
    unsigned long sleepTimeout;
    bool isConnecting;
    bool isSleeping;

    hw_timer_t* wifiRetryTimer;

    static volatile SemaphoreHandle_t WiFiRetrySemaphore;

  private:
    void connect();

    static void OnWiFiEvent(WiFiEvent_t event);
};

#endif // WIFI_MANAGER_H_
