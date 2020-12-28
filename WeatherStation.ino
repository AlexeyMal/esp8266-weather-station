// Further developed by AMA in 2020

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

#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonListener.h>

// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()

//#include "SSD1306Wire.h" //AMA 0.96 inch OLED display. Check what controller your display has!
#include "SH1106Wire.h" //AMA 1.3 inch OLED display. Check what controller your display has!
#include "OLEDDisplayUi.h"
#include "Wire.h"
//#include "OpenWeatherMapCurrent.h"
//#include "OpenWeatherMapForecast.h"
#include "OpenWeatherMapOneCall.h" // uses OneCall API of OpenWeatherMap
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"


/***************************
 * Begin Settings
 **************************/

// WIFI
const char* ESP_HOST_NAME = "esp-" + ESP.getFlashChipId(); //AMA
const char* WIFI_SSID = "XXXXX"; //AMA
const char* WIFI_PWD = "XXXXX"; //AMA

#define TZ              0 //2 //1 //Berlin //2       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries

// Setup
const int UPDATE_INTERVAL_SECS = 15 * 60; //20 * 60; // Update every 20 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
#if defined(ESP8266)
const int SDA_PIN = D3;
const int SDC_PIN = D4;
#else
const int SDA_PIN = 5; //D3;
const int SDC_PIN = 4; //D4;
#endif

// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = "XXXXX"; //AMA

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
 //SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN); //AMA 0.96 inch OLED display. Check what controller your display has!
 SH1106Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN); //AMA 1.3 inch OLED display. Check what controller your display has!
 OLEDDisplayUi   ui( &display );

/*OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;

OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;
*/
OpenWeatherMapOneCallData openWeatherMapOneCallData; //AMA use OneCall API
//OpenWeatherMapOneCall oneCallClient; //AMA Supposing a bug in the OneCallAPI: statically declared , this object will update the weather data only once :(

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;

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
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
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
FrameCallback frames[] = { drawHourly, drawHourly2, drawHourly3, drawForecast, drawForecast2, drawCurrentDetails }; //AMA
int numberOfFrames = 6;

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  display.flipScreenVertically(); //AMA // Turn the display upside down

  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();

    counter++;
  }
  // Get time from network time service
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");

  ui.setTargetFPS(30);

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
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);
  ui.setTimePerFrame(6000); //5000 //AMA (time in ms) Set the approx. time a frame is displayed (incl. transition)
  ui.setTimePerTransition(100); //0 //AMA (time in ms) Set the approx. time a transition will take
  //ui.disableAutoTransition(); //AMA do not slide my only frame
  
  ui.setOverlays(overlays, numberOfOverlays);

  // Inital UI takes care of initalising the display too.
  ui.init();

  display.flipScreenVertically(); //AMA // Turn the display upside down

  Serial.println("");

  updateData(&display);

}

void loop() {

  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }


}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display) {
  //drawProgress(display, 10, "Updating time...");
  drawProgress(display, 30, "Updating weather...");
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
/*  oneCallClient.setMetric(IS_METRIC);
  oneCallClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  oneCallClient.update(&openWeatherMapOneCallData, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);*/
  OpenWeatherMapOneCall *oneCallClient = new OpenWeatherMapOneCall();
  oneCallClient->setMetric(IS_METRIC);
  oneCallClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  oneCallClient->update(&openWeatherMapOneCallData, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
  delete oneCallClient;
  oneCallClient = nullptr;
  
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done");
  delay(500);
  
  //AMA display the timestemp of the received weather data
  /*time_t observationTimestamp = openWeatherMapOneCallData.current.dt;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  char buff[20];
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  sprintf_P(buff, PSTR("%02d:%02d:%02d  %02d.%02d.%04d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec, timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  //display->drawString(x + 16, y, String(buff));
  drawProgress(display, 100, String(buff));
  Serial.println(String(buff));
  
  delay(5000);//1000
  */
}



void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
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
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  //display->drawString(64 + x, 38 + y, currentWeather.description);
  display->drawString(64 + x, 38 + y, openWeatherMapOneCallData.current.weatherDescription);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  String temp = String(openWeatherMapOneCallData.current.temp, 1) + "°"; //(IS_METRIC ? "°C" : "°F");
  display->drawString(60 + x, 5 + y, temp);

  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  //display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon);
  display->drawString(32 + x, 0 + y, openWeatherMapOneCallData.current.weatherIconMeteoCon);
}


void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 32, y, 1); //AMA show 4 days instead of 3
  drawForecastDetails(display, x + 64, y, 2); 
  drawForecastDetails(display, x + 96, y, 3); 
}

//AMA
void drawForecast2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x,      y, 4);
  drawForecastDetails(display, x + 32, y, 5);
  drawForecastDetails(display, x + 64, y, 6); 
  drawForecastDetails(display, x + 96, y, 7); 
}

void drawHourly(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawHourlyDetails(display, x, y, 0);
  drawHourlyDetails(display, x + 32, y, 1); 
  drawHourlyDetails(display, x + 64, y, 2); 
  drawHourlyDetails(display, x + 96, y, 3); 
}

void drawHourly2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawHourlyDetails(display, x,      y, 4);
  drawHourlyDetails(display, x + 32, y, 5);
  drawHourlyDetails(display, x + 64, y, 6); 
  drawHourlyDetails(display, x + 96, y, 7); 
}

void drawHourly3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawHourlyDetails(display, x,      y, 8);
  drawHourlyDetails(display, x + 32, y, 9);
  drawHourlyDetails(display, x + 64, y, 10); 
  drawHourlyDetails(display, x + 96, y, 11); 
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = openWeatherMapOneCallData.daily[dayIndex].dt; //forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 16, y, WDAY_NAMES[timeInfo->tm_wday]);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 16, y + 12, openWeatherMapOneCallData.daily[dayIndex].weatherIconMeteoCon);
  String tempMin = String(openWeatherMapOneCallData.daily[dayIndex].tempMin, 0);// + (IS_METRIC ? "°C" : "°F");
  String tempMax = String(openWeatherMapOneCallData.daily[dayIndex].tempMax, 0);// + (IS_METRIC ? "°C" : "°F");
  display->setFont(ArialMT_Plain_10);
  String temps = tempMax+"/"+tempMin+"°";
  display->drawString(x + 16, y + 34, temps);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHourlyDetails(OLEDDisplay *display, int x, int y, int hourIndex) {
  time_t observationTimestamp = openWeatherMapOneCallData.hourly[hourIndex].dt; //forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  char buff[14];
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  display->drawString(x + 16, y, String(buff));
  
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 16, y + 12, openWeatherMapOneCallData.hourly[hourIndex].weatherIconMeteoCon);
  String temp = String(openWeatherMapOneCallData.hourly[hourIndex].temp, 0) + "°"; //+ (IS_METRIC ? "°C" : "°F");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 16, y + 34, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawCurrentDetails(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t timestamp;
  struct tm* timeInfo;
  char buff[25];
  char buff2[10];
  String temp;
  String temp2;
  //String temp3;
    
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  timestamp = openWeatherMapOneCallData.current.dt;
  timeInfo = localtime(&timestamp);
  sprintf_P(buff, PSTR("%02d:%02d %02d.%02d.%04d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString( x, y + 0, "Update: " + String(buff));
  
  timestamp = openWeatherMapOneCallData.current.sunrise;
  timeInfo = localtime(&timestamp);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  //display->drawString( x, y + 10, "Sunrise: " + String(buff));
 
  timestamp = openWeatherMapOneCallData.current.sunset;
  timeInfo = localtime(&timestamp);
  sprintf_P(buff2, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  //display->drawString( x, y + 20, "Sunset:  " + String(buff));
  display->drawString( x, y + 10, "Sun:      " + String(buff) + " - " + String(buff2));
  
  temp = String(openWeatherMapOneCallData.current.humidity);
  temp2 = String(openWeatherMapOneCallData.current.pressure);
  display->drawString( x, y + 20, "RH, AP: " + temp + "%  " + temp2 + "hPa");
 
  temp = String(openWeatherMapOneCallData.current.windSpeed * 3.6, 1); // *3600/1000 m/s -> km/h
  //temp2 = String(openWeatherMapOneCallData.current.windDeg, 0);
  temp2 = WIND_NAMES[(int)roundf((float)openWeatherMapOneCallData.current.windDeg / 22.5)]; // Rounds the wind direction out into 17 sectors. Sectors 1 and 17 are both N.
  //display->drawString( x, y + 30, "Wind:     " + temp + "km/h  " + temp2 + "°  " + temp3);
  display->drawString( x, y + 30, "Wind:     " + temp + "km/h  " + temp2);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString( x + 64, y + 40, openWeatherMapOneCallData.current.weatherDescription);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[25];

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  sprintf_P(buff, PSTR("%02d:%02d:%02d  %02d.%02d.%04d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec, timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(0, 54, String(buff));

  //AMA added date 
  //display->setTextAlignment(TEXT_ALIGN_CENTER);
  //sprintf_P(buff, PSTR("%02d.%02d.%04d"), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  //display->drawString(64, 54, String(buff));
  
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  //String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  String temp = String(openWeatherMapOneCallData.current.temp, 0) + "°"; //+ (IS_METRIC ? "°C" : "°F");
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForWeatherUpdate to true");
  readyForWeatherUpdate = true;
}
