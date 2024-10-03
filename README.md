# esp8266-weather-station
 esp8266 weather station using Open-Meteo API
 
 ![alt text](https://github.com/AlexeyMal/esp8266-weather-station/blob/main/Demo.jpg)
 
 Can be programmed to ESP8266 using Arduino IDE.
 
 Based on
 https://github.com/ThingPulse/esp8266-weather-station
 Here you can find detailed instructions on building your weather station hardware if you need.
 The disadvantage of the ThingPulse code is that it uses OpenWeatherMap APIs that are limited and require user registration.
 Up to now I have been using OpenWeatherMap OneCall API 2.5 but they have discontinued serving it on 17 Sept 2024, suggesting to change to their paid API 3.0.
 As OpenWeatherMap require a (payed) subscription for API 3.0 with a credit card I have decided to change the provider and just reprogrammed my weather station to use a new, free and flexible open-meteo API.
 
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
