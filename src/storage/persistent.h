#include <ArduinoJson.h>
#include <EEPROM.h>

#ifndef PERSISTENT_CONFIGURATION_H_
#define PERSISTENT_CONFIGURATION_H_

#define _STORAGE_IDENTIFIER "MF1"

namespace Storage {
  class Persistent {
    public:
      Persistent() : root(jsonBuffer.createObject()) {
      };

      void clear();
      void loadConfig();
      void saveConfig();

      void get(const String& key, String& value);
      void set(const String& key, const String& value);

      void get(const String& key, uint16_t& value);
      void set(const String& key, const uint16_t value);

    private:
      static constexpr size_t kEEPROMSize = 512;

      struct eepromLayout {
              char id[4];
              char jsonString[kEEPROMSize - 4];
      } config = {
              _STORAGE_IDENTIFIER,
              ""
      };

      StaticJsonBuffer<kEEPROMSize> jsonBuffer;
      JsonObject& root;
  };
}

#endif // PERSISTENT_CONFIGURATION_H_
