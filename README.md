# esp8266-weather-station
 esp8266 weather station using free Open-Meteo API (instead of discontinued OpenWeatherMapOneCall API 2.5)
 
 ![alt text](https://github.com/AlexeyMal/esp8266-weather-station/blob/main/Demo.jpg)
 
 Can be programmed to ESP using Arduino IDE.
 
 Requires this library:
 https://github.com/ThingPulse/esp8266-weather-station
 Here you can also find detailed instructions on building your weather station.
 You have to install/copy this library in the Arduino IDE.
 
 Features you get with my code: 
 - uses Open-Meteo API (see https://open-meteo.com/en/docs)
 - 4 icons per display page (instead of originally 3)
 - houry weather forecast for 8 hours
 - daily weather forecast for one week with min and max temperatures
 - BME280 sensor for indoor temperature, humidity, pressure
 
 Usable display: SSD1306 0.96 inch OLED or SH1106 1.3 inch OLED.
 
 Parts: 
  - SSD1306 0.96 inch OLED or SH1106 1.3 inch OLED
  - esp8266 module
  - optional: BME280 sensor

 You have to set the following things in the WeatherStation.ino file to make it work for you:
  - Your WiFi SSID and password - WIFI_SSID and WIFI_PWD
  - Your Latitude and Longitude - OPEN_WEATHER_MAP_LOCATTION_LAT and OPEN_WEATHER_MAP_LOCATTION_LON
  - Your timezone in line setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
  - Your display - #define SSD1306 or #define SH1106
  - Your SDA_PIN and SDC_PIN for the display (and optional BME280 sensor)
 
 Enjoy!
 (2024) Alexey
