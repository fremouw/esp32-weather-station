#ifndef PUBLISHER_H_
#define PUBLISHER_H_

#include <functional>
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

namespace Mqtt {
  class Publisher {
    private:
      static const int kPublishTimeout = 5000;
      static const int kConnectTimeout = 5000;
      static const int kMaxReconnects = 4;

      struct Settings {
        String broker;
        uint16_t port;
        String username;
        String password;
      } settings;

      struct Context {
        String topic;
        String payload;
        std::function<void(bool)> callback;
      } context;

      PubSubClient mqttClient;
      bool isPublishing;
      unsigned long lastReconnect;
      uint8_t reconnects;
      unsigned long publishTimer;

      bool reconnect();
      void fireCallback(bool success);
    public:
      Publisher(const WiFiClient& client) : mqttClient((WiFiClient&)client) {};
      ~Publisher() {};

      void setup(const String& broker, const uint16_t port, const String& username, const String& password);
      bool publish(const String& topic, const String& payload, std::function<void(bool)> callback);
      void loop();
  };
}

#endif // PUBLISHER_H_
