#include "Rx_Store_inf.h"
#include "setup_data.h"
#include "lang.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

const int max_readings = 14;
WeatherType WxDailyconditions[1];
WeatherType WxHourlyForecast[max_readings];

// Obtain data from OpenWeather API
bool DecodeWeather(WiFiClient& json, const String& Type) {
  Serial.print(F("\nCreating object...and "));
  // allocate the JsonDocument
  JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  //JsonObject root = doc.as<JsonObject>();
  //JsonArray  list = root["list"];
  Serial.println(" Decoding " + Type + " data");
  if (Type == "daily") {
    // All Serial.println statements are for diagnostic purposes and not required, remove if not needed
    WxDailyconditions[0].Temperature_MAX = doc["list"][0]["temp"]["max"].as<float>();
    WxDailyconditions[0].Temperature_MIN = doc["list"][0]["temp"]["min"].as<float>();
    WxDailyconditions[0].Timezonesec     = doc["city"]["timezone"].as<int32_t>();
    if (strcmp(Units, "M") == 0) {
      WxDailyconditions[0].Temperature_MAX -= 273.15;
      WxDailyconditions[0].Temperature_MIN -= 273.15;
    }
    Serial.println("Tempmax:     "+String(WxDailyconditions[0].Temperature_MAX));
    Serial.println("Tempmin:     "+String(WxDailyconditions[0].Temperature_MIN));
    Serial.println("Timezonesec: "+String(WxDailyconditions[0].Timezonesec));
  }  
  if (Type == "hourly") {
    //Serial.println(json);
    Serial.print(F("\nReceiving Forecast period - ")); 
    for (byte r = 0 ; r < max_readings - 1 ; r++) {
      Serial.println("\nPeriod-" + String(r) + "--------------");
      WxHourlyForecast[r].Dt   = doc["list"][r+1]["dt"].as<long>();                             
      WxHourlyForecast[r].Icon = doc["list"][r+1]["weather"][0]["icon"].as<String>();   
      WxHourlyForecast[r].Id   = doc["list"][r+1]["weather"][0]["id"].as<int>();           
      Serial.println("Dt:   "+String(WxHourlyForecast[r].Dt));
      Serial.println("Icon: "+String(WxHourlyForecast[r].Icon));
      Serial.println("Id:   "+String(WxHourlyForecast[r].Id));
    }    
  }
  return true;
}

// Receive and storage data of the date 
DateType ConvertUnixTime_date(long unix_time, int32_t timezonesec) {
  // The date format depends on the language: Chinese 2026/10/16,
  // metric 16/10/2026, imperial 10/16/2026.
  DateType Dateinf;
  // The timestamp is UTC, so first add the timezone offset reported by the data
  // source, then break it into year/month/day with gmtime().
  // Previously the offset was accepted but never applied, so the display always
  // showed UTC - Changsha (UTC+8) was 8 hours behind, meaning the date shown was
  // the previous day until 08:00 every morning.
  time_t t = (time_t)unix_time + (time_t)timezonesec;
  struct tm *now_tm = gmtime(&t);
  if (now_tm == nullptr) {
    Dateinf.date[0] = '\0';
    Dateinf.weekday[0] = '\0';
    return Dateinf;
  }
  setlocale(LC_TIME, Loc_Id);
  if (Lang_IsChinese()) {
    strftime(Dateinf.date, sizeof(Dateinf.date), "%Y/%m/%d", now_tm);
  }
  else if (strcmp(Units, "M") == 0) {
    strftime(Dateinf.date, sizeof(Dateinf.date), "%d/%m/%Y", now_tm);
  }
  else {
    strftime(Dateinf.date, sizeof(Dateinf.date), "%m/%d/%Y", now_tm);
  }
  // Weekday names always come from the string tables in lang.cpp; we no longer
  // depend on strftime output because on-board locale support is incomplete.
  strcpy(Dateinf.weekday, Lang_GetWeekday(now_tm->tm_wday));
  return Dateinf;
}

String ConvertUnixTime_hourtime(long unix_time, int32_t timezonesec) {
  // Returns a clock time such as 21:00 or 9:00PM; Chinese always uses 24-hour format.
  time_t t = (time_t)unix_time + (time_t)timezonesec;   // Same as ConvertUnixTime_date: add the timezone offset first
  struct tm *future_tm = gmtime(&t);
  if (future_tm == nullptr) return String("--:--");
  setlocale(LC_TIME, Loc_Id);
  char hourtime[10];   // Was 6, too small for the 12-hour "9:00PM" and overflowed
  if (strcmp(Units, "M") == 0 || Lang_IsChinese()) {
    strftime(hourtime, sizeof(hourtime), "%H:%M", future_tm);
  }
  else {
    int hour = future_tm->tm_hour;
    bool isPM = false;

    if (hour == 0) {
        hour = 12;  
    } else if (hour == 12) {
        isPM = true; 
    } else if (hour > 12) {
        hour -= 12;   
        isPM = true; 
    }
    // Format the hour and minute
    snprintf(hourtime, sizeof(hourtime), "%d:%02d", hour, future_tm->tm_min);

    // Add AM/PM manually
    if (isPM) {
        strcat(hourtime, "PM");
    } else {
        strcat(hourtime, "AM");
    }
  }
  return hourtime;
}

// Receive and store data from OpenWeather
bool obtain_wx_data(WiFiClient& client, const String& RequestType) {
  // There used to be a units variable (metric/imperial) that was computed but
  // never used; it has been removed.
  // The URI actually carries the raw Units switch ("M"/"I"), and temperatures are
  // converted by hand inside DecodeWeather().
  client.stop(); // close connection before sending a new request
  HTTPClient http;    
  String uri = "/data/2.5/forecast/" + RequestType + "?lat=" + LAT + "&lon=" + LON + "&appid=" + apikey + "&mode=json&units=" + Units ;//The default output mode here is json.
  if(RequestType == "daily")
  {
    uri += "&cnt=1"; 
    http.begin(client, serverdaily, 80, uri);
    Serial.println("Ask daily success.");
    Serial.println(uri);
  }
  else
  {
    uri += "&cnt=" + String(max_readings);
    http.begin(client, serverhourly, 80, uri);
    Serial.println("Ask forecast success.");
    Serial.println(uri);
  }  
  //http.begin(uri,test_root_ca); //HTTPS example connection
  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK) {
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
    http.end();
    return true;
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}

// ---------------------------------------------------------------------------
// OpenWeatherMap adapter entry point.
//
// It owns its own transport (a plain WiFiClient), so callers no longer need to
// pass a client object. Internally it calls obtain_wx_data twice exactly as
// before, leaving the existing path untouched and carrying zero regression risk.
// This wrapper exists so that it shares the same signature as om_fetch_all(),
// allowing weather_source.cpp to dispatch uniformly.
// ---------------------------------------------------------------------------
bool owm_fetch_all() {
  WiFiClient client;   // Local variable, destroyed on return
  if (!obtain_wx_data(client, "daily")) {
    client.stop();
    return false;
  }
  if (!obtain_wx_data(client, "hourly")) {
    client.stop();
    return false;
  }
  client.stop();
  return true;
}




