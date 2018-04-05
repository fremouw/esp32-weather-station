#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "weather_station.h"
#include "wireless/wifi_manager.h"

WiFiManager wifiManager;

WeatherStation weatherStation(wifiManager);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print(F("\r\nBooting... v1.0-"));
  Serial.print(APP_VERSION);
  Serial.print(" (");
  Serial.print(__TIMESTAMP__);
  Serial.println(F(")."));
  Serial.print("ESP32 SDK version is: ");
  Serial.print(ESP.getSdkVersion());
  Serial.println(".");
  Serial.print("CPU revision is: ");
  Serial.print(ESP.getChipRevision());
  Serial.println("");
  Serial.print("Clocks: CPU=");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.print(" MHz, Flash=");
  Serial.print(ESP.getFlashChipSpeed() / 1000000);
  Serial.println(" MHz");
  Serial.print("Flash: Size=");
  Serial.print(ESP.getFlashChipSize());
  Serial.println(" bytes");

  weatherStation.setup();

  wifiManager.setup(WSConfig::kSsid, WSConfig::kSsidPassword);
}

void loop() {
  weatherStation.loop();
}
