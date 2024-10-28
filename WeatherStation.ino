// Further developed by AMA in 2020-2024

/**The MIT License (MIT)

Copyright (c) 2018 by Daniel Eichhorn - ThingPulse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at https://thingpulse.com
*/

#include <Arduino.h>

///#include <ESPWiFi.h>
#include <ESP8266WiFi.h> ///
///#include <ESPHTTPClient.h>
#include <ESP8266HTTPClient.h> ///
#include <JsonListener.h>
#include <ArduinoOTA.h> ///

// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()

//select the oled display controller here >>>
#define SSD1306
//#define SH1106

#if defined(SSD1306)
#include "SSD1306Wire.h" //AMA 0.96 inch OLED display. Check what controller your display has!
#else
#include "SH1106Wire.h" //AMA 1.3 inch OLED display. Check what controller your display has!
#endif
#include "OLEDDisplayUi.h"
//#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include "OpenWeatherMapCurrent.h"
//#include "OpenWeatherMapForecast.h"
//#include "OpenWeatherMapOneCall.h" // uses OneCall API of OpenWeatherMap
#include "OpenMeteoOneCall.h" // uses OneCall API of Open-Meteo
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"


/***************************
 * Begin Settings
 **************************/

// WIFI
const char* ESP_HOST_NAME = "esp-" + ESP.getFlashChipId(); //AMA
const char* WIFI_SSID = "XXXXX"; //AMA
const char* WIFI_PWD = "XXXXX"; //AMA

/*#define TZ              2 //0 //2 //1 //Berlin //2       // (utc+) TZ in hours
#define DST_MN          0 //60      // use 60mn for summer time in some countries
*/

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;

#if defined(SSD1306)
//AMA 0.96 inch OLED display SSD1306Wire:
const int SDA_PIN = D3;
const int SDC_PIN = D4; //D4 = LED_BUILTIN, annoying blinking
#else
//AMA 1.3 inch OLED display SH1106Wire:
const int SDA_PIN = D1; //D1; //D3;
const int SDC_PIN = D3; //D2; //D4; //D4 = LED_BUILTIN, annoying blinking
#endif

TwoWire Wire2; //BME280 AMA <<<<<<<<<<<<<<<<<<<<<<<<<<
Adafruit_BME280 bme; // I2C BME280
// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = "XXXXX"; //AMA not needed any more

/*
Go to https://www.latlong.net/ and search for a location. Go through the
result set and select the entry closest to the actual location you want to display
data for. Use Latitude and Longitude values here.
 */
//Maulburg, DE
float OPEN_WEATHER_MAP_LOCATTION_LAT = 47.6463; //52.520008; // Berlin, DE
float OPEN_WEATHER_MAP_LOCATTION_LON = 7.7821; //13.404954; // Berlin, DE
 
// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "en"; //"ru"; //"de"; //AMA for the name under the weather icon 
const uint8_t MAX_FORECASTS = 8; //"One Call API" gives 8 days

const boolean IS_METRIC = true;

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
//const String WDAY_NAMES[] = {"ВС", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"}; //AMA Did not work, nothing is displayed
//const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
const String WIND_NAMES[] = {"N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW","N"};

/***************************
 * End Settings
 **************************/
 // Initialize the oled display for address 0x3c
 // sda-pin=14 and sdc-pin=12
 #if defined(SSD1306)
 SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN); //AMA 0.96 inch OLED display. Check what controller your display has!
 #else
 SH1106Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN); //AMA 1.3 inch OLED display. Check what controller your display has!
 #endif
 OLEDDisplayUi   ui( &display );

/*OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;

OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;
*/
OpenWeatherMapOneCallData openWeatherMapOneCallData; //AMA use OneCall API
OpenWeatherMapOneCall oneCallClient; //AMA 

/*#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)*/
//time_t now;

const int UPDATE_INTERVAL_SECS = 15 * 60; //15 min // open-meteo updates data every 15 minutes, at: :00, :15, :30, :45
bool readyForWeatherUpdate = false; // flag changed in the ticker function every UPDATE_INTERVAL_SECS
long timeSinceLastWUpdate = 0;
//String lastUpdate = "--";

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y); //AMA
void drawHourly(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHourly2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y); //AMA
void drawHourly3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y); //AMA
void drawCurrentDetails(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y); //AMA
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void setReadyForWeatherUpdate();


// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left

//FrameCallback frames[] = { drawForecast }; //AMA only show the forecast
//int numberOfFrames = 1;
//FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast };
//int numberOfFrames = 3;
//FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast, drawForecast2 }; //AMA
//int numberOfFrames = 4;
//FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawHourly, drawHourly2, drawForecast, drawForecast2 }; //AMA
//int numberOfFrames = 6;
//FrameCallback frames[] = { drawHourly, drawHourly2, drawHourly3, drawForecast, drawForecast2, drawCurrentDetails }; //AMA
//int numberOfFrames = 6;
//FrameCallback frames[] = { drawCurrentWeather, drawHourly, drawHourly2, drawHourly3, drawForecast, drawForecast2, drawCurrentDetails }; //AMA
//int numberOfFrames = 7;
//FrameCallback frames[] = { drawCurrentWeather, drawHourly, drawHourly2, drawCurrentWeather, drawForecast, drawForecast2 }; //AMA
//int numberOfFrames = 6;

FrameCallback frames[] = { drawCurrentWeather, drawCurrentDetails, drawHourly, drawHourly2, drawCurrentWeather, drawForecast, drawForecast2 }; //AMA
int numberOfFrames = 7;

//OverlayCallback overlays[] = { drawHeaderOverlay };
//int numberOfOverlays = 1;
///OverlayCallback overlays[] = { drawHeaderOverlay, drawHeaderOverlay2 }; //will be drawn over each other
///int numberOfOverlays = 2;

void setup() {
  Serial.begin(115200); //good for debug purposes, e.g. to investigate the reboot reason
  Serial.println("\r\nReset");
  //Serial.println();
  //Serial.println();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  ///display.setContrast(255); //high
  display.setContrast(50); ///80 90 120 60 //here and below
  //display.setBrightness(100); /// 60 //here and below

  ///display.flipScreenVertically(); //AMA // Turn the display upside down

  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    display.clear();
    //display.drawString(64, 10, "Connecting to WiFi"); //commented out to avoid oled burn-in if wifi is not available
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();

    counter++;
  }
  // Get time from network time service
  /// configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);          // Zeitzone MEZ setzen //https://www.mikrocontroller.net/topic/479624
  while (!time(nullptr)) // vorsichtshalber auf die Initialisierund der Lib warten
  {
    //Serial.print(".");
    //display.clear();
    //display.drawString(64, 10, "Getting time");
    //display.display();
    delay(500);
  }
  //Serial.println("OK");
  delay(1000); // Es kann einen Moment dauern, bis man die erste NTP-Zeit hat, solange bekommt man noch eine ungültige Zeit
  
  ///ui.setTargetFPS(30);
  ui.setTargetFPS(1); // updates display every second to show seconds counting

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  //https://github.com/ThingPulse/esp8266-oled-ssd1306/blob/master/README.md
  ui.disableAllIndicators(); //disableIndicator(); //AMA

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT); ///SLIDE_LEFT

  ui.setFrames(frames, numberOfFrames);
  ui.setTimePerFrame(2000); //6000 //5000 //AMA (time in ms) Set the approx. time a frame is displayed (incl. transition)
  ui.setTimePerTransition(0); //100 //0 //AMA (time in ms) Set the approx. time a transition will take
  //ui.disableAutoTransition(); //AMA do not slide my only frame
  
  //ui.setOverlays(overlays, numberOfOverlays);

  // Inital UI takes care of initalising the display too.
  ui.init();

  display.setContrast(60); /// 70 80 90 120 60
  //display.setBrightness(100); /// 
  ///display.flipScreenVertically(); //AMA // Turn the display upside down

  //Serial.println("");

  updateData(&display);

  Wire2.begin(SDA_PIN, SDC_PIN); // begin(SDA, SCL);  BME280 AMA <<<<<<<<<<<<<<<<<<<<<<<<<<
  bme.begin(0x76, &Wire2); //  BME280 AMA <<<<<<<<<<<<<<<<<<<<<<<<<<
  // http://adafruit.github.io/Adafruit_BME280_Library/html/class_adafruit___b_m_e280.html
  // Sets a value in Celcius to be added to each temperature reading. This adjusted temperature is used in pressure and humidity readings.
  //bme.setTemperatureCompensation(-2.8f); //  BME280 AMA <<<<<<<<<<<<<<<<<<<<<<<<<<
  // with Adafruit_BME280::MODE_FORCED you need to call Adafruit_BME280::takeForcedMeasurement() every time you need a new measurement
  bme.setSampling( Adafruit_BME280::MODE_NORMAL, //  BME280 AMA <<<<<
    Adafruit_BME280::SAMPLING_X1,
    Adafruit_BME280::SAMPLING_X1,
    Adafruit_BME280::SAMPLING_X1,
    Adafruit_BME280::FILTER_OFF,
    Adafruit_BME280::STANDBY_MS_1000 // STANDBY_MS_0_5 
  );
    
  ArduinoOTA.begin();     // enable to receive update/uploade firmware via Wifi OTA
}

void loop() {

  ArduinoOTA.handle();          // listen for update OTA request from clients 

  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display); //gets weather data from openweathermap site, this takes ca. 3 seconds
    delay(0); //give CPU time to the Wi-Fi/TCP stacks, https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }


}

/*void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}*/

void updateData(OLEDDisplay *display) {
  //drawProgress(display, 10, "Updating time...");
  ///drawProgress(display, 30, "Updating weather...");
/*  currentWeatherClient.setMetric(IS_METRIC);
  currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient.updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  drawProgress(display, 50, "Updating forecasts...");
  forecastClient.setMetric(IS_METRIC);
  forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
*/
//oneCall
  oneCallClient.setMetric(IS_METRIC);
  oneCallClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  oneCallClient.update(&openWeatherMapOneCallData, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
  /*OpenWeatherMapOneCall *oneCallClient = new OpenWeatherMapOneCall();
  oneCallClient->setMetric(IS_METRIC);
  oneCallClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  oneCallClient->update(&openWeatherMapOneCallData, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
  delete oneCallClient;
  oneCallClient = nullptr;*/
  
  readyForWeatherUpdate = false;
  ///drawProgress(display, 100, "Done");
  ///delay(500);
  delay(0); //share time
}

/*void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = WDAY_NAMES[timeInfo->tm_wday];

  //sprintf_P(buff, PSTR("%s, %02d/%02d/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  sprintf_P(buff, PSTR("%s %02d.%02d.%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(64 + x, 5 + y, String(buff));
  display->setFont(ArialMT_Plain_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x, 15 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}*/

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16); //24
  //sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(x, 36 + y, String(buff));

  display->setFont(Meteocons_Plain_36); //21
  display->drawString(x, y, openWeatherMapOneCallData.current.weatherIconMeteoCon);
//  display->setFont(ArialMT_Plain_16);
//  String temp = String(openWeatherMapOneCallData.current.temp, 1) + "°"; //(IS_METRIC ? "°C" : "°F"); //,1
//  display->drawString(26 + x, 34 + y, temp); //26+x, 24+
  display->setFont(ArialMT_Plain_16); //24
  String temp = String(openWeatherMapOneCallData.current.temp, 1) + "°"; //(IS_METRIC ? "°C" : "°F"); //,1
  display->drawString(42 + x, 0 + y, temp); 
  temp = String(openWeatherMapOneCallData.current.humidity) + " %";
  display->drawString(42 + x, 18 + y, temp); 
    
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  temp = String(bme.readTemperature() -3.0f, 1) + "°"; //-2.9 //attention: corrections of BME280 readings here
  display->drawString(128 + x, 0 + y, temp); //was 64 for TEXT_ALIGN_LEFT
  temp = String(bme.readHumidity() +7, 0) + " %"; //+8 //attention: corrections of BME280 readings here
  display->drawString(128 + x, 18 + y, temp);
//  temp = String(bme.readPressure()/100.0F +41.0f, 0) + "hPa"; //+41 +42 //attention: corrections of BME280 readings here
  temp = String(bme.readPressure()/100.0F +41.0f, 0); //+ "hPa"; //+41 +42 //attention: corrections of BME280 readings here
  display->drawString(128 - 22 + x, 36 + y, temp); //128+x
  display->setFont(ArialMT_Plain_10);
  display->drawString(128 + x, 41 + y, "hPa"); //in small font

  //drawHeaderOverlay2(display, state, x, y); //footer string
  drawHeaderOverlay1(display, state, x, y); //footer string
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  //display->drawHorizontalLine(0, 12, 32); //underline the current day
  drawForecastDetails(display, x + 32, y, 1); //AMA show 4 days instead of 3
  drawForecastDetails(display, x + 64, y, 2); 
  drawForecastDetails(display, x + 96, y, 3); 
  //no footer string
}

//AMA
void drawForecast2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x,      y, 4);
  drawForecastDetails(display, x + 32, y, 5);
  drawForecastDetails(display, x + 64, y, 6); 
  drawForecastDetails(display, x + 96, y, 7); 
  //no footer string
}

void drawHourly(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawHourlyDetails(display, x, y, 0);
  drawHourlyDetails(display, x + 32, y, 1); 
  drawHourlyDetails(display, x + 64, y, 2); 
  drawHourlyDetails(display, x + 96, y, 3); 
  drawHeaderOverlay1(display, state, x, y); //footer string
}

void drawHourly2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawHourlyDetails(display, x,      y, 4);
  drawHourlyDetails(display, x + 32, y, 5); //5  7
  drawHourlyDetails(display, x + 64, y, 6); //6  10
  drawHourlyDetails(display, x + 96, y, 7); //7  13
  drawHeaderOverlay1(display, state, x, y); //footer string
}

/*void drawHourly3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawHourlyDetails(display, x,      y, 8);
  drawHourlyDetails(display, x + 32, y, 9);
  drawHourlyDetails(display, x + 64, y, 10); 
  drawHourlyDetails(display, x + 96, y, 11); 
}*/

/*******************************************/
// Daily Forecast Details
/*******************************************/
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = openWeatherMapOneCallData.daily[dayIndex].dt; //forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  //Serial.print("dayIndex = ");Serial.println(dayIndex);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  /*if (dayIndex == 0) {
    display->fillRect( x+2,  y+1,  29,  11); // fill white (x,  y,  width,  height)
    display->setColor(BLACK);
  }*/
  display->drawString(x + 16, y, WDAY_NAMES[timeInfo->tm_wday]);
  //Serial.print("WDAY_NAMES = ");Serial.println(WDAY_NAMES[timeInfo->tm_wday]);
  display->setColor(WHITE);
  if (dayIndex == 0) display->drawHorizontalLine(x+2, y+12, 29); //line under the current day

  // white rectangular around SAT and SUN
  if ((timeInfo->tm_wday == 0) || (timeInfo->tm_wday == 6)) {
    display->drawRect( x+2,  y+1,  29,  11); // fill white (x,  y,  width,  height)
  }
  
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 16, y + 15, openWeatherMapOneCallData.daily[dayIndex].weatherIconMeteoCon); //y+14
  String tempMin = String(openWeatherMapOneCallData.daily[dayIndex].tempMin, 0)+"°";// + (IS_METRIC ? "°C" : "°F");
  String tempMax = String(openWeatherMapOneCallData.daily[dayIndex].tempMax, 0)+"°";// + (IS_METRIC ? "°C" : "°F");
  String rain_prob = String(openWeatherMapOneCallData.daily[dayIndex].rain_prob)+"%"; // rain_probability_percentage
  display->setFont(ArialMT_Plain_10);
  //String temps = tempMax+"/"+tempMin+"°";
  //display->drawString(x + 16, y + 38, temps); //y+36
  display->drawString(x + 16, y + 34, rain_prob);
  display->drawString(x + 16, y + 44, tempMin);
  display->drawString(x + 16, y + 54, tempMax);

  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

/*******************************************/
// Hourly Forecast Details
/*******************************************/
void drawHourlyDetails(OLEDDisplay *display, int x, int y, int hourIndex) {
  time_t observationTimestamp = openWeatherMapOneCallData.hourly[hourIndex].dt; //forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  char buff[14];
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  /*if (hourIndex == 0) {
    display->fillRect( x+2,  y+1,  29,  11); // fill white (x,  y,  width,  height)
    display->setColor(BLACK);
  }*/
  display->drawString(x + 16, y, String(buff));
  display->setColor(WHITE);
  //if (hourIndex == 0) display->drawHorizontalLine(x+4, y, x+24); //line above the current hour //x, 0, 32
  if (hourIndex == 0) display->drawHorizontalLine(x+2, y+12, 29); //line under the current hour

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 16, y + 16, openWeatherMapOneCallData.hourly[hourIndex].weatherIconMeteoCon); //y+14
  String temp = String(openWeatherMapOneCallData.hourly[hourIndex].temp, 0) + "°"; //+ (IS_METRIC ? "°C" : "°F");
  String rain_prob = String(openWeatherMapOneCallData.hourly[hourIndex].rain_prob)+"%"; // rain_probability_percentage
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 16, y + 34, rain_prob);
  display->drawString(x + 16, y + 45, temp); //y+36
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

/**********************************/
// Show daily[0] Details (current)
/**********************************/
void drawCurrentDetails(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t timestamp;
  struct tm* timeInfo;
  char buff[25];
  char buff2[10];
  String temp;
  String temp2;
  String temp3;
    
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  //timestamp = openWeatherMapOneCallData.daily[0].dt;
  timestamp = openWeatherMapOneCallData.current.dt;
  timeInfo = localtime(&timestamp);
  sprintf_P(buff, PSTR("%02d:%02d %02d.%02d.%04d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString( x, y + 0, "Update: " + String(buff));
  
  timestamp = openWeatherMapOneCallData.daily[0].sunrise;
  timeInfo = localtime(&timestamp);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  temp3 = String(openWeatherMapOneCallData.daily[0].uvi, 1);
  //Serial.print(temp3);Serial.println(openWeatherMapOneCallData.daily[0].uvi);
  display->drawString( x, y + 14, "Sunrise: " + String(buff) + "   " + "UVI: " + temp3);
   
  timestamp = openWeatherMapOneCallData.daily[0].sunset;
  timeInfo = localtime(&timestamp);
  temp3 = String(openWeatherMapOneCallData.daily[0].rain, 0);
  sprintf_P(buff2, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  display->drawString( x, y + 26, "Sunset: " + String(buff2) + "  " + "Rain: " + temp3);
  //display->drawString( x, y + 11, "Sun:       " + String(buff) + " - " + String(buff2));
    
  //temp = String(openWeatherMapOneCallData.current.humidity);
  //temp2 = String(openWeatherMapOneCallData.current.pressure);
  //display->drawString( x, y + 22, "RH, AP:  " + temp + " %  " + temp2 + " hPa");
 
  //temp = String(openWeatherMapOneCallData.current.windSpeed * 3.6, 1); // *3600/1000 m/s -> km/h
  temp = String(openWeatherMapOneCallData.daily[0].windSpeed,1); // *3600/1000 m/s -> km/h
  //temp2 = String(openWeatherMapOneCallData.current.windDeg, 0);
  temp2 = WIND_NAMES[(int)roundf((float)openWeatherMapOneCallData.daily[0].windDeg / 22.5)]; // Rounds the wind direction out into 17 sectors. Sectors 1 and 17 are both N.
  temp3 = String(openWeatherMapOneCallData.daily[0].windGusts,1); // *3600/1000 m/s -> km/h
  //display->drawString( x, y + 30, "Wind:     " + temp + "km/h  " + temp2 + "°  " + temp3);
  display->drawString( x, y + 38, "Wind: " + temp + " kn " + temp2 + " " + temp3 + " kn");
  //temp3 = String(openWeatherMapOneCallData.current.visibility * 0.001, 0); //[m]->km
  //display->drawString( x, y + 33, "W, S:  " + temp + " km/h " + temp2 + " " + temp3 + " km");

//  temp = String(openWeatherMapOneCallData.current.visibility * 0.001, 0); //[m]->km
//  display->drawString( x+1, y + 40, "Visibility: " + temp + " km");

  //temp = String(openWeatherMapOneCallData.current.feels_like , 0);
  temp = String(openWeatherMapOneCallData.daily[0].tempMin, 1);
  temp2 = String(openWeatherMapOneCallData.daily[0].tempMax , 1);
  //display->drawString( x, y + 44, "Feels:    " + temp + "°  Dew point: " + temp2 + "°");
  display->drawString( x, y + 50, "T.min: " + temp + "°  T.max: " + temp2 + "°");
    
  //drawHeaderOverlay1(display, state, x, y); //footer string
}

/*void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  time_t now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[25];

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);

  //if (state->currentFrame%2==0) //then even number, else - odd number
  if (!(state->currentFrame%2 == 0)) //inverted
  {
    
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  ///sprintf_P(buff, PSTR("%02d:%02d:%02d   %02d.%02d.%04d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec, timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(0, 54, String(buff));

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  //sprintf_P(buff, PSTR("%02d.%02d.%04d"), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  //display->drawString(64, 54, String(buff));
//  String temp = String(openWeatherMapOneCallData.current.temp, 0) + "°"; //+ (IS_METRIC ? "°C" : "°F");
//  display->drawString(56, 54, temp);
  //sprintf_P(buff, PSTR("%s %02d.%02d.%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(58, 54, WDAY_NAMES[timeInfo->tm_wday].c_str());
  
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  //String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  //String temp = String(openWeatherMapOneCallData.current.temp, 0) + "°"; //+ (IS_METRIC ? "°C" : "°F");
  //display->drawString(128, 54, temp);
  sprintf_P(buff, PSTR("%02d.%02d.%04d"), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(128, 54, String(buff));
  
  }
  else 
  {
    
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //String temp = String(openWeatherMapOneCallData.current.temp, 0) + "°  " + openWeatherMapOneCallData.current.weatherDescription;
  String temp = String(openWeatherMapOneCallData.current.temp, 1) + "°";
  display->drawString(0, 54, temp);

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128, 52, openWeatherMapOneCallData.current.weatherDescription); //54 low case text needs two pixels below the line
    
  }
  //display->drawHorizontalLine(0, 52, 128); //(0, 52, 128)
}*/

void drawHeaderOverlay1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[25];

  //hotfix against 1s to early overlay drawing (display lib bug?) - not a bug, need to respect x and y coordinate offsets
  //display->setColor(BLACK);
  //display->fillRect( 0, 54,  128,  10); // fill white (x,  y,  width,  height)
    
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  ///sprintf_P(buff, PSTR("%02d:%02d:%02d   %02d.%02d.%04d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec, timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(x + 0, y + 54, String(buff));

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(x + 58, y + 54, WDAY_NAMES[timeInfo->tm_wday].c_str());
  
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  sprintf_P(buff, PSTR("%02d.%02d.%04d"), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(x + 128, y + 54, String(buff));

  //display->drawHorizontalLine(0, 52, 128); //(0, 52, 128)
}

void drawHeaderOverlay2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  //hotfix against 1s to early overlay drawing (display lib bug?) - not a bug, need to respect x and y coordinate offsets
  //display->setColor(BLACK);
  //display->fillRect( 0, 54,  128,  10); // fill white (x,  y,  width,  height)
  
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //String temp = String(openWeatherMapOneCallData.current.temp, 0) + "°  " + openWeatherMapOneCallData.current.weatherDescription;
  //String temp = String(openWeatherMapOneCallData.current.temp, 1) + "°";
  //display->drawString(x + 0, y + 54, temp);
  display->drawString(x + 0, y + 52, openWeatherMapOneCallData.current.weatherDescription); //54 low case text needs two pixels below the line

  //display->setTextAlignment(TEXT_ALIGN_RIGHT);
  //display->drawString(x + 128, y + 52, openWeatherMapOneCallData.current.weatherDescription); //54 low case text needs two pixels below the line
}

void setReadyForWeatherUpdate() {
  //Serial.println("Setting readyForWeatherUpdate to true");
  readyForWeatherUpdate = true;
}
