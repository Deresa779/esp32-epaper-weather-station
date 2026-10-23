#include "setup_data.h"
#include <Arduino.h>

// This file holds everything you need to fill in by hand.
// Change things here; nowhere else.
//
// The values committed here are placeholders, so the repository carries no
// personal data. Replace ssid / password / apikey with your own before building.

// Flip this to 0 once you have filled in your real credentials.
// It only silences the build-time reminder below.
#define WX_CREDENTIALS_ARE_PLACEHOLDERS 1

#if WX_CREDENTIALS_ARE_PLACEHOLDERS
#warning "src/setup_data.cpp still holds placeholder credentials - fill in ssid, password and apikey"
#endif

// Set up your WiFi
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ==================== 1. Weather data source (this is the only line to edit) ===
//   WX_SRC_OPENMETEO : Open-Meteo, no API key, HTTPS enforced. Recommended.
//   WX_SRC_OWM       : OpenWeatherMap, requires an apikey, plain HTTP:80.
// Both implementations are compiled into the firmware; switching sources only
// means editing this line and rebuilding. No other file needs to change.
const int8_t WxDataSource = WX_SRC_OPENMETEO;

// ==================== 2. Open-Meteo configuration ====================
const char*    OM_HOST   = "api.open-meteo.com"; // Normally no need to change
const char*    OM_APIKEY = "";                   // Leave empty for the free tier;
                                                 // commercial users fill in a
                                                 // customer- prefixed key
const uint16_t OM_PORT   = 443;                  // HTTPS only, do not change

// ==================== 3. OpenWeatherMap configuration ====================
const String   apikey        = "YOUR_OWM_API_KEY"; // Must support
                                                 // "Daily Forecast 16 Days" and
                                                 // "Hourly forecast: 4 days"
const char     serverdaily[]  = "api.openweathermap.org";          // Daily Forecast 16 Days
const char     serverhourly[] = "pro.openweathermap.org";          // Hourly forecast: 4 days
const uint16_t OWM_PORT       = 80;                                // Plain HTTP, do not change to 443

// ==================== 4. Location ====================
const String  LAT           = "28.1302";   // Changsha
const String  LON           = "112.9344";
const int32_t TZ_OFFSET_SEC = 28800;       // Must match Timezone below,
                                           // otherwise dates will be off by 8 hours.

// Language - pick your language here; editing this line is enough.
//   "cn" = Chinese     Loc_Id should be "zh_CN.UTF-8"
//   "en" = English     Loc_Id should be "en_US.UTF-8"
//   "es" = Spanish     Loc_Id should be "es_ES.UTF-8"
// All UI text lives in the string tables in lang.cpp; reword entries there.
const String Language = "en";
const char* Loc_Id = "en_US.UTF-8"; // Follows Language above: cn -> zh_CN.UTF-8,
                                    // en -> en_US.UTF-8, es -> es_ES.UTF-8

// Units
const char* Units = "M"; // "I" for imperial units or "M" for metric units

// ==================== 5. Time sync and timezone ====================
const char* ntpServer  = "ntp.aliyun.com";   // Primary
const char* ntpServer2 = "cn.pool.ntp.org";  // Backup
// POSIX TZ uses the inverted sign: UTC+8 is written as -8.
// China does not observe DST, so no M rule is needed.
const char* Timezone   = "CST-8";
