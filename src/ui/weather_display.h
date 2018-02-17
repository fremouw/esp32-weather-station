#include <OLEDDisplayUi.h>
#include <SSD1306Wire.h>
#include "wunderground/Conditions.h"
#include "environmental/sensor.h"
#include "time/time_client.h"

#ifndef UI_WEATHER_DISPLAY_H_
#define UI_WEATHER_DISPLAY_H_

class WeatherDisplay {
  public:
    WeatherDisplay(SSD1306Wire& display, OLEDDisplayUi& ui, TimeClient& timeClient, wunderground::Conditions& conditions, environmental::Measurement& measurement);

    void setup();
    int update();

    void setShowUpdateScreen(const bool isUpdating);
    void setShowBootScreen(const bool showBootScreen);
    void setWifiQuality(const int8_t quality);
    void drawBootScreen();
    void drawOtaProgress(const unsigned int progress, const unsigned int total);
  private:
    SSD1306Wire& display;
    OLEDDisplayUi& ui;
    TimeClient& timeClient;
    environmental::Measurement& measurement;
    wunderground::Conditions& conditions;

    int numberOfFrames;
    int8_t wifiQuality;
    volatile bool showBootScreen = false;
    volatile bool showUpdateScreen = false;
    int progress;
    int progressDirection;

    static void DrawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
    static void DrawCurrentWeather(OLEDDisplay        *display,
                                   OLEDDisplayUiState *state,
                                   int16_t x,
                                   int16_t y);
    static void DrawForecastDetails(const WeatherDisplay* self, OLEDDisplay *display, int x, int y, int dayIndex);
    static void DrawIndoorTemperature(OLEDDisplay        *display,
                                      OLEDDisplayUiState *state,
                                      int16_t x,
                                      int16_t y);
    static void DrawForecast(OLEDDisplay        *display,
                             OLEDDisplayUiState *state,
                             int16_t x,
                             int16_t y);

    static void DrawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);

    static void ConvertIconTextToMeteoconIcon(const String& iconText, String& icon);
};

#endif // _UI_WEATHER_DISPLAY_H_
