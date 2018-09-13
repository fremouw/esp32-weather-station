# ESP32 Weather Station

[ESP32](https://www.espressif.com/en/products/hardware/esp32/overview) based weather station. It shows four screens. 1) Current time and date (synced over NTP) 2) The current (local) weather conditions (data from the Weather underground) 3) 3-day forecast (also with data from the weather underground) 4) Current temperature, humidity, air pressure and, air quality (eCO2 and TVOC). This project was once based on https://github.com/squix78/esp8266-weather-station-platformio-demo, but most of it probably 90% is rewritten and targeted for the ESP32.

## Demo
[![](http://img.youtube.com/vi/XPXsWYJnScY/0.jpg)](http://www.youtube.com/watch?v=XPXsWYJnScY "demo video")
## Software

Please see the config.example.h to configure your board.

## Hardware

My version uses a [2.42" OLED display](https://www.ebay.com/itm/SPI-2-42-OLED-128x64-Graphic-OLED-Module-Display-Arduino-PIC-AVR-Multi-wii/162156495387?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2060353.m2749.l2649) over SPI. A Bosch BME280 chip for measuring the temperature, humidity and air pressure. The SGP30 is used to measure the air quality, eCO2 (equivalent calculated carbon-dioxide) and TVOC (Total Volatile Organic Compound) concentration. Both these sensors are attached using a 4-wire molex connector, to keep some distance from the board and display; as these generate some heat which influences the sensors.

The ESP32 board used is a DOIT ESP32 Devkit v1, you can find these on ebay.