#include <Arduino.h>

#include <Wire.h>
#include <SPI.h>
#include <BME280_MOD-1022.h>

#include "weather_station.h"
#include "wireless/wifi_manager.h"

#define I2C_DISPLAY_ADDRESS 0x3c
#define I2C_SDA 4
#define I2C_SCL 5

#define SDA_PIN 21
#define SCL_PIN 22

const String ssid = "SSID";
const String password = "PASSWORD";

WiFiManager wifiManager;

WeatherStation weatherStation(wifiManager);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("\r\nBooting...\r\n");
  Serial.print(__DATE__);
  Serial.print(", ");
  Serial.print(__TIME__);
  Serial.println("");
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

  wifiManager.setup(ssid, password);

  // Wire.begin(SDA_PIN, SCL_PIN);

  // BME280.readCompensationParams();
  //
  // BME280.writeFilterCoefficient(fc_off);
  //
  // BME280.writeOversamplingPressure(os1x);
  // BME280.writeOversamplingTemperature(os1x);
  //
  // BME280.writeOversamplingHumidity(os1x);
  //
  // BME280.writeMode(smForced);
  //
  // uint8_t chipID = BME280.readChipId();
  //
  // // find the chip ID out just for fun
  // Serial.print("Info: chip identifier = 0x");
  // Serial.println(chipID, HEX);
  //
  // BME280.readMeasurements();
  //
  // float temperature = BME280.getTemperature();
  // float pressure    = BME280.getPressure();
  // float humidity    = BME280.getHumidity();
  //
  // Serial.print("Temperature: ");
  // Serial.println(temperature);
  //
  // Serial.print("Pressure: ");
  // Serial.println(pressure);
  //
  // Serial.print("Humidity: ");
  // Serial.println(humidity);
}

void loop() {
  weatherStation.loop();
}
