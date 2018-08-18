#include "weather_display.h"
#include "fonts/lato.h"
#include "fonts/meteocons.h"
#include "weather_station_images.h"

WeatherDisplay::WeatherDisplay(OLEDDisplay& display, OLEDDisplayUi& ui, TimeClient& timeClient, wunderground::Conditions& conditions, environmental::Measurement& measurement) : display(display), ui(ui), timeClient(timeClient), conditions(conditions),
        measurement(measurement) {}

void WeatherDisplay::setup() {
  ui.setTargetFPS(60);

  // Hack until disableIndicator works:
  // Set an empty symbol
  ui.setActiveSymbol(emptySymbol);
  ui.setInactiveSymbol(emptySymbol);

  ui.disableIndicator();

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.getUiState()->userData = this;

  static FrameCallback frames[] =
  { WeatherDisplay::DrawDateTime, WeatherDisplay::DrawCurrentWeather,
    WeatherDisplay::DrawForecast, WeatherDisplay::DrawIndoorTemperature };
  numberOfFrames = 4;

  //
  ui.setFrames(frames, numberOfFrames);

  static OverlayCallback overlays[] = { WeatherDisplay::DrawHeaderOverlay };
  static int numberOfOverlays       = 1;

  ui.setOverlays(overlays, numberOfOverlays);

  ui.init();

  // display.setI2cAutoInit(true);
  display.flipScreenVertically();
}

void WeatherDisplay::setShowUpdateScreen(const bool showUpdateScreen) {
  this->showUpdateScreen = showUpdateScreen;
}

void WeatherDisplay::setShowBootScreen(const bool showBootScreen) {
  this->showBootScreen = showBootScreen;

  if(this->showBootScreen) {
    this->progress = 0;
    this->progressDirection = 0;
    this->drawBootScreen();
  }
}

void WeatherDisplay::setWifiQuality(const int8_t quality) {
    wifiQuality = quality;
}

int WeatherDisplay::update() {
  if(this->showUpdateScreen) {
    // this->drawOtaProgress(const unsigned int progress, const unsigned int total)
  } else if(this->showBootScreen) {
    this->drawBootScreen();
  } else {
    return ui.update();
  }

  return 800;
}

void WeatherDisplay::drawBootScreen() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  // display.drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress)
  // display.setFont(ArialMT_Plain_10);
  // display.drawString(64, 10, F("Booting"));
  // display.drawProgressBar(2, 28, 124, 10, progress++);

  // if(progressDirection == 0 && progress > 16) {
  //   progressDirection = 1;
  // } else if (progressDirection == 1 && progress < 8) {
  //   progressDirection = 0;
  // }
  //
  // if(progressDirection == 0) {
  //   progress++;
  // } else {
  //   progress--;
  // }
  progress++;
  // // display.fillCircle(50, 35, progress);
  // display.drawCircleQuads(50, 35, 16, progress);
  // display.drawCircleQuads(50, 35, 8, progress);
  // display.drawCircle(28, 28, progress);
  display.drawXbm(46, 30, 8, 8, progress % 3 == 0 ? activeSymbol2 : inactiveSymbol);
  display.drawXbm(60, 30, 8, 8, progress % 3 == 1 ? activeSymbol2 : inactiveSymbol);
  display.drawXbm(74, 30, 8, 8, progress % 3 == 2 ? activeSymbol2 : inactiveSymbol);

  display.display();
}

void WeatherDisplay::drawOtaProgress(const unsigned int progress, const unsigned int total) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, F("OTA Update"));
  display.drawProgressBar(2, 28, 124, 10, progress / (total / 100));
  display.display();
}

void WeatherDisplay::DrawHeaderOverlay(OLEDDisplay        *display,
                                       OLEDDisplayUiState *state) {
  if ((state == NULL) || (state->userData == NULL)) {
      return;
  }

  const WeatherDisplay *self = static_cast<WeatherDisplay *>(state->userData);

  if (self != NULL) {
    display->setColor(WHITE);
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 54,
                        String(state->currentFrame + 1) + F("/") +
                        String(self->numberOfFrames));

    String time;
    self->timeClient.getFormattedTime(time);
    time = time.substring(0, 5);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(21, 54, time);

    for (int8_t i = 0; i < 4; i++) {
      for (int8_t j = 0; j < 2 * (i + 1); j++) {
        if ((self->wifiQuality > i * 25) || (j == 0)) {
          display->setPixel(120 + 2 * i, 63 - j);
        }
      }
    }

    display->setTextAlignment(TEXT_ALIGN_LEFT);

    wunderground::Conditions::Observation observation;
    self->conditions.getCurrentObservation(observation);

    float indoorTemperature = self->measurement.temperature;

    String temp = String(indoorTemperature, 1) + " | " + String(observation.temperature, 1) + "°C";
    display->drawString(53, 54, temp);

    display->drawHorizontalLine(0, 52, 128);
  }
}

void WeatherDisplay::DrawDateTime(OLEDDisplay        *display,
                                  OLEDDisplayUiState *state,
                                  int16_t             x,
                                  int16_t             y) {
  if ((state == NULL) || (state->userData == NULL)) {
    return;
  }

  const WeatherDisplay *self = static_cast<WeatherDisplay *>(state->userData);

  if (self != NULL) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);

    String date;
    self->timeClient.getFormattedDate(date);
    int textWidth = display->getStringWidth(date);

    display->drawString(64 + x, 5 + y, date);
    display->setFont(ArialMT_Plain_24);

    String time;
    self->timeClient.getFormattedTime(time);
    textWidth = display->getStringWidth(time);

    display->drawString(64 + x, 15 + y, time);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
  }
}

void WeatherDisplay::DrawCurrentWeather(OLEDDisplay        *display,
                                        OLEDDisplayUiState *state,
                                        int16_t             x,
                                        int16_t             y) {
  if ((state == NULL) || (state->userData == NULL)) {
    return;
  }

  const WeatherDisplay *self = static_cast<WeatherDisplay *>(state->userData);

  if (self != NULL) {

    wunderground::Conditions::Observation observation;
    self->conditions.getCurrentObservation(observation);

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(Lato_Regular_9);
    display->drawString(46 + x, 38 + y, observation.city);

    display->setFont(ArialMT_Plain_10);
    display->drawString(52 + x, 5 + y, observation.title);

    display->setFont(ArialMT_Plain_24);
    String temp = String(observation.temperature, 1) + F("°C");

    display->drawString(52 + x, 15 + y, temp);
    int tempWidth = display->getStringWidth(temp);

    display->setFont(Meteocons_Plain_42);
    String weatherIcon;
    ConvertIconTextToMeteoconIcon(observation.icon, weatherIcon);

    int weatherIconWidth = display->getStringWidth(weatherIcon);
    display->drawString(24 + x - weatherIconWidth / 2, 05 + y, weatherIcon);

  }
}

void WeatherDisplay::DrawIndoorTemperature(OLEDDisplay        *display,
                                           OLEDDisplayUiState *state,
                                           int16_t             x,
                                           int16_t             y) {

  if ((state == NULL) || (state->userData == NULL)) {
   return;
  }

  const WeatherDisplay *self = static_cast<WeatherDisplay *>(state->userData);

  if (self != NULL) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0 + y, F("Indoor"));

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_16);

    float temperature = self->measurement.temperature;
    float humidity    = self->measurement.humidity;
    display->drawString(4 + x, 12 + y, String(temperature) + F("°C"));
    display->drawString(4 + x, 30 + y, String(humidity) + F("%H"));

    display->setFont(ArialMT_Plain_10);
    float pressure = self->measurement.pressure;
    display->drawString(70 + x, 14 + y, String(pressure) + F("hPa"));
  }
}

void WeatherDisplay::DrawForecast(OLEDDisplay        *display,
                                  OLEDDisplayUiState *state,
                                  int16_t             x,
                                  int16_t             y) {
  if ((state == NULL) || (state->userData == NULL)) {
    return;
  }

  const WeatherDisplay *self = static_cast<WeatherDisplay *>(state->userData);

  if (self != NULL) {
    DrawForecastDetails(self, display, x,      y, 1);
    DrawForecastDetails(self, display, x + 44, y, 2);
    DrawForecastDetails(self, display, x + 88, y, 3);
  }
}

void WeatherDisplay::DrawForecastDetails(const WeatherDisplay *self,
                                         OLEDDisplay          *display,
                                         int                   x,
                                         int                   y,
                                         int                   dayIndex) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);

  wunderground::Conditions::Forecast forecast;
  self->conditions.getForecastForPeriod(dayIndex, forecast);

  String day = forecast.title.substring(0, 3);
  day.toUpperCase();
  display->drawString(x + 20, y, day);

  String forecastIcon;
  ConvertIconTextToMeteoconIcon(forecast.icon, forecastIcon);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, forecastIcon);

  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, String(round(
                      forecast.lowTemperature)) + "|" +
                      String(round(forecast.highTemperature)));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void WeatherDisplay::ConvertIconTextToMeteoconIcon(const String& iconText,
                                                   String      & icon) {
    if (iconText == F("chanceflurries")) {
        icon = F("F");
    } else if (iconText == F("chancerain")) {
        icon = F("Q");
    } else if (iconText == F("chancesleet")) {
        icon = F("W");
    } else if (iconText == F("chancesnow")) {
        icon = F("V");
    } else if (iconText == F("chancetstorms")) {
        icon = F("S");
    } else if (iconText == F("clear")) {
        icon = F("B");
    } else if (iconText == F("cloudy")) {
        icon = F("Y");
    } else if (iconText == F("flurries")) {
        icon = F("F");
    } else if (iconText == F("fog")) {
        icon = F("M");
    } else if (iconText == F("hazy")) {
        icon = F("E");
    } else if (iconText == F("mostlycloudy")) {
        icon = F("Y");
    } else if (iconText == F("mostlysunny")) {
        icon = F("H");
    } else if (iconText == F("partlycloudy")) {
        icon = F("H");
    } else if (iconText == F("partlysunny")) {
        icon = F("J");
    } else if (iconText == F("sleet")) {
        icon = F("W");
    } else if (iconText == F("rain")) {
        icon = F("R");
    } else if (iconText == F("snow")) {
        icon = F("W");
    } else if (iconText == F("sunny")) {
        icon = F("B");
    } else if (iconText == F("tstorms")) {
        icon = F("0");
    } else if (iconText == F("nt_chanceflurries")) {
        icon = F("F");
    } else if (iconText == F("nt_chancerain")) {
        icon = F("7");
    } else if (iconText == F("nt_chancesleet")) {
        icon = F("#");
    } else if (iconText == F("nt_chancesnow")) {
        icon = F("#");
    } else if (iconText == F("nt_chancetstorms")) {
        icon = F("&");
    } else if (iconText == F("nt_clear")) {
        icon = F("2");
    } else if (iconText == F("nt_cloudy")) {
        icon = F("Y");
    } else if (iconText == F("nt_flurries")) {
        icon = F("9");
    } else if (iconText == F("nt_fog")) {
        icon = F("M");
    } else if (iconText == F("nt_hazy")) {
        icon = F("E");
    } else if (iconText == F("nt_mostlycloudy")) {
        icon = F("5");
    } else if (iconText == F("nt_mostlysunny")) {
        icon = F("3");
    } else if (iconText == F("nt_partlycloudy")) {
        icon = F("4");
    } else if (iconText == F("nt_partlysunny")) {
        icon = F("4");
    } else if (iconText == F("nt_sleet")) {
        icon = F("9");
    } else if (iconText == F("nt_rain")) {
        icon = F("7");
    } else if (iconText == F("nt_snow"))  {
        icon = F("#");
    } else if (iconText == F("nt_sunny")) {
        icon = F("4");
    } else if (iconText == F("nt_tstorms")) {
        icon = F("&");
    } else {
        icon = F(")");
    }
}
