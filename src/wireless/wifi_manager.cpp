#include "wifi_manager.h"
#include <ESPmDNS.h>
#include <list>
#include <functional>

static std::list<WiFiEventHandler> sEventList;

volatile SemaphoreHandle_t WiFiManager::WiFiRetrySemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t WiFiManager::WiFiForceRetrySemaphore = xSemaphoreCreateBinary();;

WiFiManager::WiFiManager() {
}

void WiFiManager::setup(const char* ssid, const char* password) {
  this->ssid = String(ssid);
  this->password = String(password);

  WiFi.onEvent(WiFiManager::OnWiFiEvent);

  this->connect();
}

void WiFiManager::loop() {
  bool shouldRetry = xSemaphoreTake(WiFiManager::WiFiRetrySemaphore, 0) == pdTRUE;
  bool forceRetry = xSemaphoreTake(WiFiManager::WiFiForceRetrySemaphore, 0) == pdTRUE;

  if(this->isConnecting && WiFi.status() == WL_CONNECTED) {
    Serial.println("wireless: connected!");

    this->isConnecting = false;
    this->isSleeping = false;

    delay(100);

    for(auto it = std::begin(sEventList); it != std::end(sEventList);) {
      WiFiEventHandler &handler = *it;

      (*handler)();
      ++it;
    }
  } else if(forceRetry) {
    Serial.println("wireless: forced reconnect...");

    this->connect();
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
  Serial.print(F("wireless: trying to connect to: "));
  Serial.println(this->ssid);

  this->isSleeping = false;
  this->isConnecting = true;
  this->connectTimeout = millis();

  WiFi.mode(WIFI_STA);
  WiFi.begin(this->ssid.c_str(), this->password.c_str());
}

void WiFiManager::OnWiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print(F("wireless: event: connected, IP address: "));
      Serial.println(WiFi.localIP());

      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println(F("wireless: event: lost connection."));

      xSemaphoreGiveFromISR(WiFiManager::WiFiRetrySemaphore, NULL);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println(F("wireless: event: timeout."));

      xSemaphoreGiveFromISR(WiFiManager::WiFiForceRetrySemaphore, NULL);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println(F("wireless: event: lost IP."));

      xSemaphoreGiveFromISR(WiFiManager::WiFiForceRetrySemaphore, NULL);
    break;
    default:
      Serial.print(F("wireless: event: unhandled event: "));
      Serial.println(event);
  }
}
