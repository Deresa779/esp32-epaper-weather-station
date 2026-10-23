#ifndef WEATHER_OPENMETEO_H_
#define WEATHER_OPENMETEO_H_

#include <Arduino.h>
#include <stdint.h>

/*
 * Open-Meteo data source implementation.
 *
 * Three fundamental differences from OpenWeatherMap are all absorbed inside this
 * file, so neither main.cpp nor the drawing layer needs to know about them:
 *   1. The JSON layout is completely different (no list / weather[0].icon / id);
 *   2. Weather codes are WMO 0-99 instead of OWM 200-781;
 *   3. It is HTTPS-only, so WiFiClientSecure plus a root certificate is mandatory.
 */

// Fetches data and writes it into the global arrays. Returns true on success.
bool om_fetch_all();

// Maps a WMO weather code to an OpenWeatherMap-compatible ID and icon code.
// isDay selects the "01d" or "01n" icon variant. Unknown codes fall back to
// 804 (overcast clouds), which never triggers a travel advisory.
// Exposed separately so the mapping table can be verified in isolation.
void wmo_to_owm(uint8_t code, bool isDay, int16_t& owmId, const char*& icon);

#endif /* WEATHER_OPENMETEO_H_ */
