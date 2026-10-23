#ifndef SETUP_DATA_H_
#define SETUP_DATA_H_
#include <Arduino.h>
#include "weather_source.h"   // For WX_SRC_OPENMETEO / WX_SRC_OWM

// WiFi
extern const char* ssid;
extern const char* password;

// ---------- 1. Weather data source selection ----------
//   WX_SRC_OPENMETEO / WX_SRC_OWM, values defined in weather_source.h
extern const int8_t WxDataSource;

// ---------- 2. Open-Meteo configuration ----------
extern const char*    OM_HOST;
extern const char*    OM_APIKEY;
extern const uint16_t OM_PORT;

// ---------- 3. OpenWeatherMap configuration ----------
extern const String   apikey;
extern const char     serverdaily[];
extern const char     serverhourly[];
extern const uint16_t OWM_PORT;

// ---------- 4. Location ----------
extern const String  LAT;
extern const String  LON;
extern const int32_t TZ_OFFSET_SEC;   // UTC offset in seconds, must match Timezone

// Language selection: use "cn" for Chinese, "en" for English, "es" for Spanish
// (see LANG_CN / LANG_EN / LANG_ES in lang.h)
extern const String Language;
extern const char* Loc_Id;   // Locale paired with the language; see setup_data.cpp

// Units
extern const char* Units;

// Set up your timezone
extern const char* ntpServer;
extern const char* ntpServer2;
extern const char* Timezone;

/*
 * OpenWeatherMap reference URLs (Changsha coordinates, for manual checks only):
 * https://api.openweathermap.org/data/2.5/forecast/daily?lat=28.1302&lon=112.9344&cnt=1&appid=<your-key>
 * https://pro.openweathermap.org/data/2.5/forecast/hourly?lat=28.1302&lon=112.9344&cnt=14&appid=<your-key>
 *
 * Open-Meteo reference URL (no API key required):
 * https://api.open-meteo.com/v1/forecast?latitude=28.1302&longitude=112.9344&hourly=temperature_2m,weather_code,is_day&daily=temperature_2m_max,temperature_2m_min&timezone=auto&timeformat=unixtime&forecast_days=1&forecast_hours=13
 */

#endif /* SETUP_DATA_H_ */
