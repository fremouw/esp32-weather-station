#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include "config.h"
#include "weather_station.h"
#include "wireless/wifi_manager.h"

WiFiManager wifiManager;

WeatherStation weatherStation(wifiManager);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.print(F("\r\nBooting Weather Station v1.0-"));
  Serial.print(APP_VERSION);
  Serial.print(F(" ("));
  Serial.print(__TIMESTAMP__);
  Serial.println(F(")."));
  Serial.print(F("ESP32 SDK version is: "));
  Serial.print(ESP.getSdkVersion());
  Serial.println(F("."));
  Serial.print(F("CPU revision is: "));
  Serial.print(ESP.getChipRevision());
  Serial.println(F(""));
  Serial.print(F("Clocks: CPU="));
  Serial.print(ESP.getCpuFreqMHz());
  Serial.print(F(" MHz, Flash="));
  Serial.print(ESP.getFlashChipSpeed() / 1000000);
  Serial.println(F(" MHz"));
  Serial.print(F("Flash: Size="));
  Serial.print(ESP.getFlashChipSize());
  Serial.println(F(" bytes"));
  // Serial.print(F("Memory: Total="));
  // Serial.print(ESP.getHeapSize());
  Serial.println(F(" bytes"));
  Serial.print(F("Memory: Free="));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));

  weatherStation.setup();

  wifiManager.setup(WSConfig::kSsid, WSConfig::kSsidPassword);
}

void loop() {
  static bool printRunningCore = false;
  if(!printRunningCore) {
    printRunningCore = true;
    Serial.print(F("Arduino running core: "));
    Serial.println(xPortGetCoreID());
  }

  weatherStation.loop();
}
