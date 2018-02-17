#include "wifi_manager.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <list>
#include <functional>

static std::list<WiFiEventHandler> sEventList;

volatile SemaphoreHandle_t WiFiManager::WiFiRetrySemaphore = xSemaphoreCreateBinary();

WiFiManager::WiFiManager() {
}

void WiFiManager::setup(const String& ssid, const String& password) {
  this->ssid = ssid;
  this->password = password;

  WiFi.onEvent(WiFiManager::OnWiFiEvent);

  this->connect();

  ArduinoOTA.begin();
}

void WiFiManager::loop() {
  bool shouldRetry = xSemaphoreTake(WiFiManager::WiFiRetrySemaphore, 0) == pdTRUE;

  // if(WiFi.status() == WL_CONNECTED) {
  //   // ArduinoOTA.handle();
  // }

  if(this->isConnecting && WiFi.status() == WL_CONNECTED) {
    Serial.println("wireless: connected!");

    this->isConnecting = false;
    this->isSleeping = false;

    for(auto it = std::begin(sEventList); it != std::end(sEventList);) {
      WiFiEventHandler &handler = *it;

      (*handler)();
      ++it;
    }
  } else if(this->isConnecting && millis() - this->connectTimeout > 15000) {
    Serial.println("wireless: timeout: could not connect...");

    this->isConnecting = false;
    this->isSleeping = true;
    this->sleepTimeout = millis();
  } else if(this->isSleeping && millis() - this->sleepTimeout > 60000) {
    Serial.println("wireless: wake-up, retry connecting.");

    this->connect();
  } else if(!this->isConnecting && !this->isSleeping && shouldRetry) {
    Serial.println("wireless: reconnect...");

    this->connect();
  }
}

void WiFiManager::onConnected(std::function<void()> callback) {
  WiFiEventHandler handler = std::make_shared<std::function<void()>>(callback);

  sEventList.push_back(handler);
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::GetWifiQuality(int8_t& quality) {
  if(WiFi.status() != WL_CONNECTED) {
    quality = 0;
    return;
  }

  int32_t dbm = WiFi.RSSI();

  if (dbm <= -100) {
    quality = 0;
  } else if (dbm >= -50) {
    quality = 100;
  } else {
    quality = 2 * (dbm + 100);
  }
}

void WiFiManager::connect() {
  this->isSleeping = false;
  this->isConnecting = true;
  this->connectTimeout = millis();

  WiFi.mode(WIFI_STA);
  WiFi.begin(this->ssid.c_str(), this->password.c_str());
}

void WiFiManager::OnWiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("wireless: connected, IP address: ");
      Serial.println(WiFi.localIP());

      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("wireless: lost connection.");

      xSemaphoreGiveFromISR(WiFiManager::WiFiRetrySemaphore, NULL);
      break;
  }
}
