#include "persistent.h"
#include <Arduino.h>
#include <esp_log.h>

static const char LogTag[] PROGMEM = "Persistent";

namespace Storage {
  #define _STORAGE_CONFIGURATION_START 3

  void Persistent::clear() {
    ESP_LOGI(LogTag, "clearing EEPROM.");

    for (int i = 0; i < kEEPROMSize; ++i) {
      EEPROM.write(i, 0x0);
    }

    EEPROM.commit();

    EEPROM.end();
  }

  void Persistent::loadConfig() {
    ESP_LOGI(LogTag, "read data from EEPROM.");

    EEPROM.begin(kEEPROMSize);

    if ((EEPROM.read(0) == _STORAGE_IDENTIFIER[0]) &&
        (EEPROM.read(1) == _STORAGE_IDENTIFIER[1]) &&
        (EEPROM.read(2) == _STORAGE_IDENTIFIER[2])) {
      for (size_t t = _STORAGE_CONFIGURATION_START; t < sizeof(config); t++) {
        *((char *)&config + t) = EEPROM.read(t);
      }
    } else {
      ESP_LOGI(LogTag, "no data stored in EEPROM.");
    }

    JsonObject& _root = jsonBuffer.parseObject(config.jsonString);

    for (auto & it : _root) {
      // We create a copy of the key/value pair.
      root.set(String(it.key), String(it.value.as<char*>()));
    }
  }

  void Persistent::saveConfig() {
    //
    memset(config.jsonString, 0x0, sizeof(config.jsonString));

    //
    root.printTo(config.jsonString, sizeof(config.jsonString));

    //
    size_t size = (sizeof(config) - sizeof(config.jsonString)) + strlen(
        config.jsonString);

    for (size_t j = 0; j < size; j++) {
      EEPROM.write(j, *((char *)&config + j));
    }

    EEPROM.commit();

    EEPROM.end();
  }

  void Persistent::get(const String& key, String& value) {
    const String v = root[key];

    value = v;
  }

  void Persistent::set(const String& key, const String& value) {
    root[key] = String(value);
  }

  void Persistent::get(const String& key, uint16_t& value) {
    const uint16_t v = root[key];

    value = v;
  }

  void Persistent::set(const String& key, const uint16_t value) {
    root[key] = value;
  }
}
