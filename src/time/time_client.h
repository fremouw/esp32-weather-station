#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

#ifndef TIME_CLIENT_H_
#define TIME_CLIENT_H_

class TimeClient {
public:
        TimeClient();

        void setup(const String& ntpServer, const uint16_t port = 123, const int timeout = 2000);
        bool update(std::function<void(bool)> _callback);
        void getFormattedTime(String& time);
        void getFormattedDate(String& _date);
        void getUnixTime(unsigned long& unixTime);
private:
    static const int kNTPPacketSize = 48;

    AsyncUDP udp;
    unsigned long unixTime;
    String ntpServer;
    uint16_t ntpServerPort;
    int ntpServerTimeout;
    std::function<void(bool)> callback;
};

#endif // TIME_CLIENT_H_
