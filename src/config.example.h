#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <pgmspace.h>

namespace WSConfig {
  // Wireless configuration.
  const char kHostname[] PROGMEM = "mfr-weatherstation-";
  const char kSsid[] PROGMEM = "<ssid>";
  const char kSsidPassword[] PROGMEM = "<password>";

  // I2C SDA/SCL pinout for display.
  const uint8_t kI2cDisplayAddress = 0x3c;
  const uint8_t kI2cDisplaySdaPin = 21;
  const uint8_t kI2cDisplaySclPin = 21;

  // I2C SDA/SCL pinout for BME280 sensor.
  const uint8_t kI2cTemperatureSdaPin = 5;
  const uint8_t kI2cTemperatureSclPin = 4;

  // NTP Timeserver to use.
  const char kNtpServerName[] PROGMEM = "<ntpserver>";

  // Weatherunderground API key, language and location.
  // The location is used to find closest weather station.
  const char kWundergroundApiKey[] PROGMEM  = "<apikey>";
  const char kWundergroundLanguage[] PROGMEM = "EN";
  const char kWundergroundLocation[] PROGMEM    = "<lat>,<long>";

  // MQTT configuration. Leave empty if not used.
  const char kMqttBroker[] PROGMEM = "<mqttbroker>";
  const uint16_t kMqttBrokerPort = 8883;
  const char kMqttBrokerUsername[] PROGMEM = "<username>";
  const char kMqttBrokerPassword[] PROGMEM = "<password>";
  const char kMqttTopicName[] PROGMEM = "Sensor/Temperature/1";
};

#endif /* _CONFIG_H_ */
