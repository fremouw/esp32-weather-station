#include "time_client.h"
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFi.h>

TimeClient::TimeClient() {}

void TimeClient::getFormattedTime(String& _time) {
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
}

void TimeClient::getFormattedDate(String& _date) {
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
}

void TimeClient::getUnixTime(unsigned long& unixTime) {
  unixTime = now();
}

void TimeClient::setup(const String& ntpServer, const uint16_t port, const int timeout) {
  this->ntpServer = ntpServer;
  this->ntpServerPort = port;
  this->ntpServerTimeout = timeout;

  this->udp.onPacket([this](AsyncUDPPacket packet) {
    Serial.println(F("info: got NTP packet from server."));
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

    TimeChangeRule euCET  = { "CET", Last, Sun, Oct, 2, 60 };
    TimeChangeRule euCEST = { "CEST", Last, Sun, Mar, 2, 120 };

    Timezone euCentral(euCET, euCEST);

    localTime = euCentral.toLocal(this->unixTime);

    setTime(localTime);

    if(this->callback) {
      this->callback(true);
    }
  });
}

bool TimeClient::update(std::function<void(bool)> _callback) {
  Serial.println(F("info: getting time from NTP server."));

  static const uint8_t timePacket[TimeClient::kNTPPacketSize] = {
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

  this->callback = _callback;

  IPAddress ipAddress;
  WiFi.hostByName(ntpServer.c_str(), ipAddress);

  Serial.print(F("info: ntp server="));
  Serial.print(ipAddress);
  Serial.println(F("."));

  if(this->udp.connect(ipAddress, this->ntpServerPort)) {
    udp.write(timePacket, TimeClient::kNTPPacketSize);
  } else if(this->callback) {
    this->callback(false);
  }

  return true;
}
