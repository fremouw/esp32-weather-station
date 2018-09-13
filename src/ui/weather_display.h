#include <OLEDDisplayUi.h>
#include <SSD1306Wire.h>
#include "ntp/ntp_client.h"
#include "wunderground/Conditions.h"
#include "sensors/environment.h"
#include "sensors/air_quality.h"

#ifndef UI_WEATHER_DISPLAY_H_
#define UI_WEATHER_DISPLAY_H_

namespace Frames {
  enum {
    eBootScreen = 1 << 0,
    eTimeAndDate = 1 << 1,
    eWeatherReport = 1 << 2,
    eIndoorSensors = 1 << 3,
  };

  typedef uint16_t Flags;
};

class WeatherDisplay {
  public:
    WeatherDisplay(OLEDDisplay& display
      , Ntp::Client& ntpClient
      , wunderground::Conditions& conditions
      , Sensors::Environment::Measurement& environmentMeasurement
      , Sensors::AirQuality::Measurement& airQualityMeasurement);

    void setup();
    int update();

    void addFrame(const Frames::Flags frame);
    void removeFrame(const Frames::Flags frame);

    void setWifiQuality(const int8_t quality);
    bool isInTransition();
  private:
    Frames::Flags framesEnabled = Frames::eBootScreen;

    static const size_t kMaxFrameSize = 5;
    FrameCallback frames[kMaxFrameSize];

    OLEDDisplay& display;
    OLEDDisplayUi ui;
    Ntp::Client& ntpClient;
    wunderground::Conditions& conditions;
    Sensors::Environment::Measurement& environmentMeasurement;
    Sensors::AirQuality::Measurement& airQualityMeasurement;

    int numberOfFrames;
    int8_t wifiQuality;
    int progress;
    int progressDirection;

    void updateFrames();
    void drawBootScreen();

    static void DrawDateTime(OLEDDisplay *display,
                             OLEDDisplayUiState *state,
                             int16_t x,
                             int16_t y);
    static void DrawCurrentWeather(OLEDDisplay        *display,
                                   OLEDDisplayUiState *state,
                                   int16_t x,
                                   int16_t y);
    static void DrawForecastDetails(const WeatherDisplay* self,
                                    OLEDDisplay *display,
                                    int x,
                                    int y,
                                    int dayIndex);
    static void DrawIndoorTemperature(OLEDDisplay        *display,
                                      OLEDDisplayUiState *state,
                                      int16_t x,
                                      int16_t y);
    static void DrawForecast(OLEDDisplay        *display,
                             OLEDDisplayUiState *state,
                             int16_t x,
                             int16_t y);
    static void DrawHeaderOverlay(OLEDDisplay *display,
                                  OLEDDisplayUiState *state);
    static void ConvertIconTextToMeteoconIcon(const String& iconText,
                                              String& icon);
};

#endif // _UI_WEATHER_DISPLAY_H_
