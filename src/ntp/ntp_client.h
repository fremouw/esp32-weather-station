#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

extern "C" {
    #include "lwip/init.h"
    #include "lwip/err.h"
    #include "lwip/pbuf.h"
};

#ifndef _NTP_CLIENT_H_
#define _NTP_CLIENT_H_

class Client;

namespace Ntp {
  class Client {
    public:
      Client();
      ~Client();

      void setup(const String& ntpServer, const uint16_t port = 123);
      bool update(std::function<void(bool)> _callback);
      bool getFormattedTime(String& time);
      bool getFormattedDate(String& _date);
      bool getUnixTime(unsigned long& unixTime);
    private:
      struct QueueData {
        ip_addr_t ipAddress;
        Client* context;
      };

      static const int kNTPPacketSize = 48;
      bool didSynchronizeTime = false;
      bool isUpdatingTime = false;
      AsyncUDP udp;
      unsigned long unixTime;
      String ntpServer;
      uint16_t ntpServerPort;
      std::function<void(bool)> callback;

      bool connect(const ip_addr_t* ipAddress);
      void doCallback(bool success);
      void dnsFoundCallback(const char* name, ip_addr_t* ipAddress);
      static void DnsFoundCallback(const char* name, ip_addr_t* ipAddress, void* arg);
      static void UdpTask(void* params);
  };
}

#endif // _NTP_CLIENT_H_
