#ifndef _CONFIG_H_
#define _CONFIG_H_

#define I2C_DISPLAY_ADDRESS 0x3c
#define I2C_DISPLAY_SDA 21
#define I2C_DISPLAY_SCL 22

#define I2C_TEMPERATURE_SDA 5
#define I2C_TEMPERATURE_SCL 4

#define HOSTNAME "MF-WEATHERSTATION-"

const char kNtpServerName[] PROGMEM = "<ntpserver>";
const char kWundergroundApiKey[] PROGMEM  = "<apikey>";
const char kWundergroundLanguage[] PROGMEM = "EN";
const char kWundergroundLocation[] PROGMEM    = "lat,long";
const char kMqttBroker[] PROGMEM = "mqtt.mrtn.io";
const uint16_t kMqttBrokerPort = 8883;
const char kMqttBrokerUsername[] PROGMEM = "<username>";
const char kMqttBrokerPassword[] PROGMEM = "<password>";
const char kMqttTopicName[] PROGMEM = "Sensor/Temperature/1";

#endif /* _CONFIG_H_ */
