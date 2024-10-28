// AMA 28.09.2024 Adapted to use api.open-meteo.com

/**The MIT License (MIT)
 
 Copyright (c) 2020 by Chris Klinger
 
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
 */

///#include <ESPWiFi.h>
#include <WiFiClient.h>
#include "OpenMeteoOneCall.h"

String PATH_SEPERATOR = "/";

OpenWeatherMapOneCall::OpenWeatherMapOneCall()
{
}

void OpenWeatherMapOneCall::update(OpenWeatherMapOneCallData *data, String appId, float lat, float lon)
{
  doUpdate(data, buildPath(appId, lat, lon));
}

String OpenWeatherMapOneCall::buildPath(String appId, float lat, float lon)
{
//  String units = metric ? "metric" : "imperial";
//  return "/data/2.5/onecall?appid=" + appId + "&lat=" + lat + "&lon=" + lon + "&units=" + units + "&lang=" + language;

  return "/v1/forecast?latitude=" + String(lat,4) + "&longitude=" + String(lon,4) + "&current=temperature_2m,relative_humidity_2m,is_day,weather_code&hourly=temperature_2m,precipitation_probability,is_day,weather_code&forecast_hours=8&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max,rain_sum,precipitation_probability_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant&wind_speed_unit=kn&timeformat=unixtime&timezone=Europe%2FRome&forecast_days=8";
  //return "/v1/forecast?latitude=" + String(lat,4) + "&longitude=" + String(lon,4) + "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max,rain_sum,showers_sum,wind_speed_10m_max,wind_direction_10m_dominant&timeformat=unixtime&timezone=Europe%2FBerlin"
  //test:  https://api.open-meteo.com/v1/forecast?latitude=47.6463&longitude=7.7821&current=temperature_2m,relative_humidity_2m,is_day,weather_code&hourly=temperature_2m,is_day,weather_code&forecast_hours=8&daily=weather_code,temperature_2m_max,temperature_2m_min&timeformat=unixtime&timezone=Europe%2FBerlin&forecast_days=8
  //docs: https://open-meteo.com/en/docs
  //response example: {"latitude":47.64,"longitude":7.7799993,"generationtime_ms":0.1430511474609375,"utc_offset_seconds":7200,"timezone":"Europe/Berlin","timezone_abbreviation":"CEST","elevation":352.0,"current_units":{"time":"unixtime","interval":"seconds","temperature_2m":"°C","relative_humidity_2m":"%","weather_code":"wmo code"},"current":{"time":1727528400,"interval":900,"temperature_2m":11.9,"relative_humidity_2m":81,"weather_code":3},"hourly_units":{"time":"unixtime","temperature_2m":"°C","weather_code":"wmo code"},"hourly":{"time":[1727528400,1727532000,1727535600,1727539200,1727542800,1727546400,1727550000,1727553600],"temperature_2m":[11.9,11.7,11.8,11.4,10.8,10.0,9.3,7.8],"weather_code":[3,61,80,3,3,3,2,2]},"daily_units":{"time":"unixtime","weather_code":"wmo code","temperature_2m_max":"°C","temperature_2m_min":"°C"},"daily":{"time":[1727474400,1727560800,1727647200,1727733600,1727820000,1727906400,1727992800,1728079200],"weather_code":[80,3,61,61,80,80,61,45],"temperature_2m_max":[12.6,14.3,16.2,13.8,12.2,10.8,10.3,14.1],"temperature_2m_min":[6.8,3.6,6.6,12.0,9.2,8.6,8.6,7.2]}}  
}

void OpenWeatherMapOneCall::doUpdate(OpenWeatherMapOneCallData *data, String path)
{
  //unsigned long lostTest = 10000UL;
  unsigned long lostTest = 7000UL; //AMA timeout
  unsigned long lost_do = millis();
  const uint maxResponseLength = 25000u; //30000u //AMA max response length to break if the responce is too long
  uint cntResponseLength = 0u; //AMA current responce lenth counter
  this->weatherItemCounter = 0;
  this->hourlyItemCounter = 0;
  this->dailyItemCounter = 0;
  this->data = data;
  JsonStreamingParser parser;
  parser.setListener(this);
  Serial.printf("[HTTP] Requesting resource at http://%s:%u%s\n", host.c_str(), port, path.c_str());

  WiFiClient client;
  #if defined(ESP8266)
  if (client.connect(host, port)) {
  #else
  if (client.connect(host.c_str(), port)) {
  #endif
    bool isBody = false;
    char c;
    Serial.println("[HTTP] connected, now GETting data");
    client.print("GET " + path + " HTTP/1.1\r\n"
                                 "Host: " +
                 host + "\r\n"
                        "Connection: close\r\n\r\n");

    while ( (client.connected() || client.available()) 
          && ((millis() - lost_do) < lostTest) ) //AMA
    {
      if ( (client.available()) 
         && (cntResponseLength < maxResponseLength) ) //AMA
      {
        cntResponseLength ++; //AMA
        /*if ((millis() - lost_do) > lostTest) {
          Serial.println("[HTTP] lost in client with a timeout");
          yield(); //AMA
          client.stop();
          //ESP.restart();
          this->data = nullptr; //AMA
          return; //AMA
        }*/
        c = client.read();
        if (c == '{' || c == '[') {
          isBody = true;
        }
        if (isBody) {
          parser.parse(c);
        }
      }
      // give WiFi and TCP/IP libraries a chance to handle pending events
      yield();
    }
    client.stop();
  } else {
    Serial.println("[HTTP] failed to connect to host");
  }
  this->data = nullptr;
}

void OpenWeatherMapOneCall::whitespace(char c)
{
  Serial.println("whitespace");
}

void OpenWeatherMapOneCall::startDocument()
{
  Serial.println("start document");
}

void OpenWeatherMapOneCall::key(String key)
{
  currentKey = key;
}

void OpenWeatherMapOneCall::value(String value)
{

  // "current": {..}
  if (currentParent == "current") {
    // "current": {.. "dt": 1587216739, .. }
    if (currentKey == "time") {
      this->data->current.dt = value.toInt();
    }
    if (currentKey == "temperature_2m") {
      this->data->current.temp = value.toFloat();
    }
    // if (currentKey == "pressure") {
    //   this->data->current.pressure = value.toInt();
    // }
    if (currentKey == "relative_humidity_2m") {
      this->data->current.humidity = value.toInt();
    }
    if (currentKey == "is_day") {
      this->data->current.is_day = value.toInt();
    }
    if (currentKey == "weather_code") {
      this->data->current.weatherId = value.toInt();
      this->data->current.weatherIconMeteoCon = getMeteoconIcon(this->data->current.weatherId, this->data->current.is_day);
    }
  }

  // "hourly": [..]
  if((currentParent == "hourly") && (hourlyItemCounter < maxHourlyForecasts)) {
    if (currentKey == "time") {
      this->data->hourly[hourlyItemCounter].dt = value.toInt();
    }
    if (currentKey == "temperature_2m") {
      this->data->hourly[hourlyItemCounter].temp = value.toFloat();
    }
    if (currentKey == "is_day") {
      this->data->hourly[hourlyItemCounter].is_day = value.toInt();
    }    
    if (currentKey == "weather_code") {
      this->data->hourly[hourlyItemCounter].weatherId = value.toInt();
      this->data->hourly[hourlyItemCounter].weatherIconMeteoCon = getMeteoconIcon(this->data->hourly[hourlyItemCounter].weatherId, this->data->hourly[hourlyItemCounter].is_day);
    }
    if (currentKey == "precipitation_probability") {
      this->data->hourly[hourlyItemCounter].rain_prob = value.toInt();
    }
    hourlyItemCounter++;
  }

  // "daily": [..]
  if((currentParent == "daily") && (dailyItemCounter < maxDailyForecasts)) {
    if (currentKey == "time") {
      this->data->daily[dailyItemCounter].dt = value.toInt();
    }
    if (currentKey == "temperature_2m_min") {
      this->data->daily[dailyItemCounter].tempMin = value.toFloat();
    }
    if (currentKey == "temperature_2m_max") {
      this->data->daily[dailyItemCounter].tempMax = value.toFloat();
    }
    if (currentKey == "weather_code") {
      this->data->daily[dailyItemCounter].weatherId = value.toInt();
      this->data->daily[dailyItemCounter].weatherIconMeteoCon = getMeteoconIcon(this->data->daily[dailyItemCounter].weatherId, 1u);
    }
    if (currentKey == "sunrise") {
      this->data->daily[dailyItemCounter].sunrise = value.toInt();
    }
    if (currentKey == "sunset") {
      this->data->daily[dailyItemCounter].sunset = value.toInt();
    }
    if (currentKey == "wind_speed_10m_max") {
      this->data->daily[dailyItemCounter].windSpeed = value.toFloat();
    }
    if (currentKey == "wind_gusts_10m_max") {
      this->data->daily[dailyItemCounter].windGusts = value.toFloat();
    }
    if (currentKey == "wind_direction_10m_dominant") {
      this->data->daily[dailyItemCounter].windDeg = value.toFloat();
    }
    if (currentKey == "uv_index_max") {
      this->data->daily[dailyItemCounter].uvi = value.toFloat();
    }
    if (currentKey == "rain_sum") {
      this->data->daily[dailyItemCounter].rain = value.toFloat();
    }
    if (currentKey == "precipitation_probability_max") {
      this->data->daily[dailyItemCounter].rain_prob = value.toInt();
    }
    
    dailyItemCounter++;
  }
}

void OpenWeatherMapOneCall::endArray()
{
  // currentKey = "";
  // currentParent= currentParent.substring(0, currentParent.lastIndexOf(PATH_SEPERATOR));
}

void OpenWeatherMapOneCall::startObject()
{
  // if(currentKey == "") {
  //   currentKey = "_obj";
  // }
  // currentParent += PATH_SEPERATOR + currentKey;

  currentParent = currentKey;
}

void OpenWeatherMapOneCall::endObject()
{
  // if (currentParent == "/ROOT/current/weather[]/_obj" || currentParent == "/ROOT/daily[]/_obj/weather[]/_obj" || currentParent == "/ROOT/daily[]/_obj/weather[]/_obj"  ) {
  //   weatherItemCounter++;
  // }
  // if (currentParent == "hourly") {
  //   hourlyItemCounter++;
  // }
  // if (currentParent == "/ROOT/daily[]/_obj") {
  //   dailyItemCounter++;
  // }  
  // currentKey = "";
  // currentParent= currentParent.substring(0, currentParent.lastIndexOf(PATH_SEPERATOR));

  currentParent = "";
}

void OpenWeatherMapOneCall::endDocument()
{
}

void OpenWeatherMapOneCall::startArray()
{
  // weatherItemCounter = 0;
  
  // currentParent += PATH_SEPERATOR + currentKey + "[]";
  // currentKey = "";
  hourlyItemCounter = 0;
  dailyItemCounter = 0;
}

String OpenWeatherMapOneCall::getMeteoconIcon(uint16_t weather_code, uint8_t is_day) {
  switch (weather_code) {

  case 0:
  case 1:
 	// clear sky
  // 01d
  //if (icon == "01d") 	{
    if(is_day == 1) return "B"; else
  // }
  // // 01n
  // if (icon == "01n") 	{
     return "C";
  // }

  case 2:
  // few clouds
  // 02d
  // if (icon == "02d") 	{
    if(is_day == 1) return "H"; else
  // }
  // // 02n
  // if (icon == "02n") 	{
    //  return "4";
    return "I";
  // }

  case 3:
  // scattered clouds
    return "N";  
  // 03d
  // if (icon == "03d") 	{
    // if(is_day == 1) return "N"; else
  // }
  // // 03n
  // if (icon == "03n") 	{
    //  return "5";
  // }

  // case ?:
  // // broken clouds
  // // 04d
  // if (icon == "04d") 	{
  //   return "Y";
  // }
  // // 04n
  // if (icon == "04n") 	{
  //   return "%";
  // }

  case 80:
  case 81:
  case 82:
  // shower rain
    return "R";  
  // 09d
  // if (icon == "09d") 	{
    // if(is_day == 1) return "R"; else
  // }
  // // 09n
  // if (icon == "09n") 	{
    //  return "8";
  // }

  case 51:  
  case 53:  
  case 55:
  case 56:
  case 57:  
  case 61:
  case 63:  
  case 65:  
  case 66:  
  case 67:  
  // rain
    return "Q";
  // 10d
  // if (icon == "10d") 	{
    // if(is_day == 1) return "Q"; else
  // }
  // // 10n
  // if (icon == "10n") 	{
    //  return "7";
  // }

  case 95:
  case 96:
  case 99:
  // thunderstorm
    return "P";
  // 11d
  // if (icon == "11d") 	{
    // if(is_day == 1) return "P"; else
  // }
  // // 11n
  // if (icon == "11n") 	{
    //  return "6";
  // }

  case 71:
  case 73:
  case 75:
  case 77:
  case 85:
  case 86:
  // snow
    return "W";
  // 13d
  // if (icon == "13d") 	{
    // if(is_day == 1) return "W"; else
  // }
  // // 13n
  // if (icon == "13n") 	{
    //  return "#";
  // }

  case 45:
  case 48:
  // mist
  // 50d
  // if (icon == "50d") 	{
    if(is_day == 1) return "J"; else
  // }
  // // 50n
  // if (icon == "50n") 	{
     return "K";
  // }
  // return "M";

  default:
  // Nothing matched: N/A
  return ")";
  }
}

/*
https://open-meteo.com/en/docs
WMO Weather interpretation codes (WW)

Code	Description
0	Clear sky
1, 2, 3	Mainly clear, partly cloudy, and overcast
45, 48	Fog and depositing rime fog
51, 53, 55	Drizzle: Light, moderate, and dense intensity
56, 57	Freezing Drizzle: Light and dense intensity
61, 63, 65	Rain: Slight, moderate and heavy intensity
66, 67	Freezing Rain: Light and heavy intensity
71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
77	Snow grains
80, 81, 82	Rain showers: Slight, moderate, and violent
85, 86	Snow showers slight and heavy
95 *	Thunderstorm: Slight or moderate
96, 99 *	Thunderstorm with slight and heavy hail
(*) Thunderstorm forecast with hail is only available in Central Europe
*/
