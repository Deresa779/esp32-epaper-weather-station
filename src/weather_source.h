#ifndef WEATHER_SOURCE_H_
#define WEATHER_SOURCE_H_

#include <Arduino.h>

/*
 * Unified entry point for weather data sources.
 *
 * Each source has its own implementation file:
 *   Open-Meteo      -> weather_openmeteo.cpp (HTTPS, no API key required)
 *   OpenWeatherMap  -> owm_fetch_all() in Rx_Store_inf.cpp (plain HTTP, key required)
 *
 * Pick one with WxDataSource in setup_data.cpp and rebuild; that single line is
 * the only thing that needs to change.
 * Callers (main.cpp) only ever call Wx_FetchAll() and stay agnostic about which
 * backend is in use.
 */

#define WX_SRC_OWM       0   // OpenWeatherMap
#define WX_SRC_OPENMETEO 1   // Open-Meteo

// Fetches data and fills the two global arrays:
//   WxDailyconditions[0]     Temperature_MAX / Temperature_MIN / Timezonesec
//   WxHourlyForecast[0..12]  Dt / Id / Icon
// Returns true on success.
bool Wx_FetchAll();

// Human-readable name of the active source; used for serial logging only.
const char* Wx_SourceName();

#endif /* WEATHER_SOURCE_H_ */
