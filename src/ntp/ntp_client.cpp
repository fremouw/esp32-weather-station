#include "ntp_client.h"
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFi.h>
#include <esp_log.h>

extern "C"{
  #include "lwip/opt.h"
  #include "lwip/tcp.h"
  #include "lwip/inet.h"
  #include "lwip/dns.h"
  #include "lwip/init.h"
}

static const char LogTag[] PROGMEM = "NtpClient";

static xQueueHandle NtpQueue;
static volatile TaskHandle_t NtpTaskHandle = NULL;

namespace Ntp {
  Client::Client() {}
  Client::~Client() {}

  bool Client::getFormattedTime(String& _time) {
    if(!this->didSynchronizeTime) {
      return false;
    }

    int _hour   = hour();
    int _minute = minute();
    int _second = second();

    String formattedTime;

    if (_hour < 10) {
      formattedTime += F("0");
    }
    formattedTime += String(_hour);
    formattedTime += ":";

    if (_minute < 10) {
      formattedTime += F("0");
    }
    formattedTime += String(_minute);
    formattedTime += F(":");

    if (_second < 10) {
      formattedTime += F("0");
    }
    formattedTime += String(_second);

    _time = formattedTime;

    return true;
  }

  bool Client::getFormattedDate(String& _date) {
    if(!this->didSynchronizeTime) {
      return false;
    }

    String formattedDate;

    formattedDate = dayShortStr(weekday());
    formattedDate += F(", ");
    if(day() < 10) {
        formattedDate += F("0");
    }
    formattedDate += String(day());
    formattedDate += F(" ");
    formattedDate += monthShortStr(month());
    formattedDate += F(" ");
    formattedDate += year();

    _date = formattedDate;

    return true;
  }

  bool Client::getUnixTime(unsigned long& unixTime) {
    if(!this->didSynchronizeTime) {
      return false;
    }

    unixTime = now();

    return true;
  }

  void Client::setup(const String& ntpServer, const uint16_t port) {
    this->ntpServer = ntpServer;
    this->ntpServerPort = port;

    //
    if(!NtpQueue){
      NtpQueue = xQueueCreate(16, sizeof(QueueData));
      if(!NtpQueue){
        ESP_LOGE(LogTag, "error creating NTP queue.");
      }
    }

    //
    if(!NtpTaskHandle) {
      static const char kNtpTaskName[] PROGMEM = "NtpClient::TaskHandler";

      xTaskCreatePinnedToCore(&Client::UdpTask,
                              kNtpTaskName,
                              4096,
                              NULL,
                              3,
                              (TaskHandle_t*)&NtpTaskHandle,
                              1);
    } else {
      ESP_LOGE(LogTag, "error creating NTP task.");
    }

    this->udp.onPacket([this](AsyncUDPPacket packet) {
      ESP_LOGI(LogTag, "got NTP packet from server.");

      uint8_t *packetBuffer = packet.data();

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;

      // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;

      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;

      this->unixTime = epoch;

      time_t localTime;

      // @ToDo make configurable.
      TimeChangeRule euCET  = { "CET", Last, Sun, Oct, 2, 60 };
      TimeChangeRule euCEST = { "CEST", Last, Sun, Mar, 2, 120 };

      Timezone euCentral(euCET, euCEST);

      localTime = euCentral.toLocal(this->unixTime);

      setTime(localTime);

      this->didSynchronizeTime = true;

      this->doCallback(true);
    });
  }

  bool Client::update(std::function<void(bool)> _callback) {
    bool success = false;

    if(this->isUpdatingTime) {
      ESP_LOGE(LogTask, "already updating time.");
      _callback(false);

      return false;
    }

    this->isUpdatingTime = true;
    this->callback = _callback;

    ESP_LOGI(LogTag, "look up IP for hostname %s.", this->ntpServer.c_str());

    ip_addr_t ipAddress;
    err_t error = dns_gethostbyname(this->ntpServer.c_str(), &ipAddress, (dns_found_callback)&DnsFoundCallback, this);
    if(error == ERR_OK) {
      ESP_LOGI(LogTag, "nice! directly returned ip.");
      success = this->connect(&ipAddress);
    } else if(error == ERR_INPROGRESS) {
      success = true;
    } else {
      // Throw error?
    }

    return success;
  }

  bool Client::connect(const ip_addr_t* ipAddress) {
    static const uint8_t timePacket[Client::kNTPPacketSize] = {
      0b11100011,                         // LI, Version, Mode
      0,                                  // Stratum, or type of clock
      6,                                  // Polling Interval
      0xec,                               // Peer Clock Precision
      0,             0, 0, 0, 0, 0, 0, 0, // 8 bytes of zero for Root Delay & Root Dispersion
      49,
      0x4e,
      49,
      52
    };
    bool success = false;

    ESP_LOGI(LogTag, "connecting to %s (%s) %d.", IPAddress(ipAddress->u_addr.ip4.addr).toString().c_str(), this->ntpServer.c_str(), xPortGetCoreID());

    if(this->udp.connect(ipAddress, this->ntpServerPort)) {
      ESP_LOGI(LogTag, "connected.");
      udp.write(timePacket, Client::kNTPPacketSize);

      success = true;
    } else {
      ESP_LOGE(LogTag, "failed connecting.");

      this->doCallback(false);
    }

    return success;
  }

  void Client::doCallback(bool success) {
    if(this->callback) {
      this->callback(success);
    } else {
      ESP_LOGE(LogTag, "no callback set!");
    }

    this->isUpdatingTime = false;
  }

  void Client::dnsFoundCallback(const char *name, ip_addr_t* ipAddress) {
    if(ipAddress) {
      QueueData* data = (QueueData *)malloc(sizeof(QueueData));

      data->ipAddress = *ipAddress;
      data->context = this;

      ESP_LOGE(LogTag, "adding to queue (%s).", name);

      if (xQueueSend(NtpQueue, &data, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(LogTag, "error adding to queue (%s)!", name);

        free((void*)data);

        this->doCallback(false);
      }
    } else {
      ESP_LOGE(LogTag, "could not resolve IP for %s.", name);

      this->doCallback(false);
    }
  }

  void Client::DnsFoundCallback(const char* name, ip_addr_t* ipAddress, void* arg) {
    if(arg != NULL) {
      Client *self = reinterpret_cast<Client *>(arg);

      self->dnsFoundCallback(name, ipAddress);
    } else {
      ESP_LOGE(LogTag, "no callback set!");
    }
  }

  // static void _udp_task(void *pvParameters){
  void Client::UdpTask(void* params) {
    QueueData* data = NULL;

    for (;;) {
      if(xQueueReceive(NtpQueue, &data, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(LogTag, "got item from queue.");

        data->context->connect(&data->ipAddress);

        free((void*)data);
      }

      // Needed?
      delay(10);
    }

    ESP_LOGI(LogTag, "exit NTP task.");

    NtpTaskHandle = NULL;
    vTaskDelete( NULL );
  }
}
