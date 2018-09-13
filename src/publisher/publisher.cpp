#include "publisher.h"

namespace Mqtt {
  static const char LogTag[] PROGMEM = "Publisher";

  void Publisher::setup(const String& broker, const uint16_t port, const String& username, const String& password) {
    this->settings.broker = broker;
    this->settings.port = port;
    this->settings.username = username;
    this->settings.password = password;

    this->mqttClient.setServer(this->settings.broker.c_str(), this->settings.port);
    this->isPublishing = false;
    this->lastReconnect = 0;
    this->reconnects = 0;
  }

  bool Publisher::publish(const String& topic, const String& payload, std::function<void(bool)> callback) {
    bool success = false;

    if(this->isPublishing) {
      ESP_LOGE(LogTask, "already publishing.");

      if(callback) {
        callback(false);
      }
    } else {

      this->isPublishing = true;
      this->publishTimer = millis();
      this->reconnects = 0;

      this->context.callback = callback;
      this->context.topic = topic;
      this->context.payload = payload;

      success = true;
    }

    return success;
  }

  void Publisher::loop() {
    if(!this->isPublishing) {
      return;
    }

    if(this->mqttClient.connected()) {
      ESP_LOGI(LogTag, "connected to MQTT broker!!1");

      bool success = this->mqttClient.publish(this->context.topic.c_str(), this->context.payload.c_str(), true);
      ESP_LOGI(LogTag, "publishing topic %s, payload: %s (%d)", this->context.topic.c_str(), this->context.payload.c_str(), success);
      
      this->fireCallback(success);

      // Not needed, we only publish.
      // this->mqttClient.loop();

      this->mqttClient.disconnect();
      return;
    } else {
      this->lastReconnect = 0;
    }

    //
    unsigned long now = millis();
    if(now - this->publishTimer > kPublishTimeout) {
      ESP_LOGE(LogTag, "timeout publishing to MQTT broker (%s, %s).", this->settings.broker.c_str(), this->context.topic.c_str());

      this->fireCallback(false);

      return;
    }

    if (!this->mqttClient.connected() && now - this->lastReconnect > kConnectTimeout) {
      this->lastReconnect = now;

      if(this->reconnects++ > kMaxReconnects) {
        ESP_LOGE(LogTag, "unable to connect to MQTT broker after %d retries.", this->reconnects);

        this->fireCallback(false);
      } else {
        this->reconnect();
      }
    }
  }

  bool Publisher::reconnect() {
    Serial.println("(re)connecting..");
    if (this->mqttClient.connect(this->settings.broker.c_str(), this->settings.username.c_str(), this->settings.password.c_str())) {
      ESP_LOGI(LogTag, "connecting to MQTT broker.");
    }

    return this->mqttClient.connected();
  }

  void Publisher::fireCallback(bool success) {
    if(this->context.callback) {
      this->context.callback(success);
    }

    this->isPublishing = false;
  }
}
