#include "weather_openmeteo.h"
#include "setup_data.h"
#include "Rx_Store_inf.h"
#include "certs.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string.h>

// Number of hourly points needed: the current hour plus the next 12 = 13,
// which matches max_readings - 1.
#define OM_POINTS        (max_readings - 1)
#define OM_PATH          "/v1/forecast"
#define OM_CONN_TIMEOUT  8000   // Connection timeout in milliseconds
#define OM_HS_TIMEOUT    15     // TLS handshake timeout in seconds. The default
                                // is 120s, which would keep the device awake for
                                // two minutes on a weak network.

// ---------------------------------------------------------------------------
// WMO weather code -> OpenWeatherMap-compatible condition ID + icon code
//
// Why map instead of letting downstream code consume WMO codes directly: the
// 116 lines of Paint_recommend() are written against OWM intensity details
// (511 freezing rain = rain AND snow, 611-613 sleet = hail icon,
//  600 / 601 / 620 / 621 light-to-moderate snow = wool hat),
// and iconcode_to_icon() inspects Id a second time (tints the fog icon red when
// 710 < Id < 732). Converting once here keeps downstream untouched.
//
// Rule of thumb: map to the OWM id that yields the same icon AND the same
// travel-advisory outcome.
// ---------------------------------------------------------------------------
struct WmoRow { uint8_t code; int16_t id; const char* d; const char* n; };

static const WmoRow WMO_MAP[] = {
  { 0, 800, "01d", "01n"},   // Clear sky
  { 1, 801, "02d", "02n"},   // Mainly clear
  { 2, 802, "03d", "03n"},   // Partly cloudy
  { 3, 804, "04d", "04n"},   // Overcast
  {45, 741, "50d", "50n"},   // Fog
  {48, 741, "50d", "50n"},   // Depositing rime fog
  {51, 300, "09d", "09n"},   // Light drizzle
  {53, 301, "09d", "09n"},   // Moderate drizzle
  {55, 302, "09d", "09n"},   // Dense drizzle
  {56, 511, "13d", "13n"},   // Light freezing drizzle -> 511, counted as both
                             // rain and snow downstream
  {57, 511, "13d", "13n"},   // Dense freezing drizzle
  {61, 500, "10d", "10n"},   // Slight rain
  {63, 501, "10d", "10n"},   // Moderate rain
  {65, 502, "10d", "10n"},   // Heavy rain
  {66, 511, "13d", "13n"},   // Light freezing rain
  {67, 511, "13d", "13n"},   // Heavy freezing rain
  {71, 600, "13d", "13n"},   // Slight snowfall
  {73, 601, "13d", "13n"},   // Moderate snowfall
  {75, 602, "13d", "13n"},   // Heavy snowfall
  {77, 600, "13d", "13n"},   // Snow grains
  {80, 520, "09d", "09n"},   // Slight rain showers
  {81, 521, "09d", "09n"},   // Moderate rain showers
  {82, 522, "09d", "09n"},   // Violent rain showers
  {85, 620, "13d", "13n"},   // Slight snow showers
  {86, 621, "13d", "13n"},   // Heavy snow showers
  {95, 200, "11d", "11n"},   // Thunderstorm
  {96, 201, "11d", "11n"},   // Thunderstorm with slight hail
  {99, 202, "11d", "11n"},   // Thunderstorm with heavy hail
};
static const int WMO_MAP_N = sizeof(WMO_MAP) / sizeof(WMO_MAP[0]);

void wmo_to_owm(uint8_t code, bool isDay, int16_t& owmId, const char*& icon) {
  for (int i = 0; i < WMO_MAP_N; i++) {
    if (WMO_MAP[i].code == code) {
      owmId = WMO_MAP[i].id;
      icon  = isDay ? WMO_MAP[i].d : WMO_MAP[i].n;
      return;
    }
  }
  // Unknown code: treat it as overcast. Emitting no advisory is safer than
  // wrongly suggesting sunglasses.
  owmId = 804;
  icon  = isDay ? "04d" : "04n";
}

// ---------------------------------------------------------------------------
// URL construction
// ---------------------------------------------------------------------------
static String om_build_uri() {
  String u = OM_PATH;
  u += "?latitude="  + LAT + "&longitude=" + LON;
  u += "&hourly=temperature_2m,weather_code,is_day";
  u += "&daily=temperature_2m_max,temperature_2m_min";
  u += "&timezone=auto&timeformat=unixtime&forecast_days=1";
  u += "&forecast_hours=" + String(OM_POINTS);
  u += (strcmp(Units, "M") == 0) ? "&temperature_unit=celsius" : "&temperature_unit=fahrenheit";
  if (OM_APIKEY != nullptr && OM_APIKEY[0] != '\0') {
    u += "&apikey=";
    u += OM_APIKEY;
  }
  return u;
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------
static bool om_decode(const String& payload) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[OM] JSON 解析失败: %s\n", err.c_str());
    return false;
  }

  JsonArray t   = doc["hourly"]["time"];
  JsonArray wc  = doc["hourly"]["weather_code"];
  JsonArray idy = doc["hourly"]["is_day"];

  if (t.isNull() || wc.isNull()) {
    Serial.println("[OM] 响应缺少 hourly.time 或 weather_code");
    return false;
  }

  const int N = t.size();
  if (N < OM_POINTS || (int)wc.size() < N) {
    Serial.printf("[OM] 小时数据不足: time=%d code=%d，需要 %d\n",
                  N, (int)wc.size(), (int)OM_POINTS);
    return false;
  }

  // UTC offset consumed by ConvertUnixTime_*(). Seed it with the configured
  // fallback first, then overwrite once parsing succeeds.
  WxDailyconditions[0].Timezonesec     = doc["utc_offset_seconds"] | TZ_OFFSET_SEC;
  WxDailyconditions[0].Temperature_MAX = doc["daily"]["temperature_2m_max"][0] | 0.0f;
  WxDailyconditions[0].Temperature_MIN = doc["daily"]["temperature_2m_min"][0] | 0.0f;

  // Locate the starting slot.
  // Normally forecast_hours=13 makes N exactly 13, so start is always 0 because
  // the server already trimmed the window for us.
  // If forecast_hours ever stops working and we fall back to a full
  // forecast_days=2 series (N=48), scan for the slot that contains "now".
  int start = 0;
  if (N > OM_POINTS) {
    long now = (long)time(nullptr);
    for (int i = 0; i < N; i++) {
      if (t[i].as<long>() <= now) start = i;
      else break;
    }
    if (start + OM_POINTS > N) start = N - OM_POINTS;
  }

  for (int r = 0; r < OM_POINTS; r++) {
    const int i = start + r;
    const uint8_t code = (uint8_t)(wc[i] | 999);        // Missing -> 999, hits the fallback
    const bool isDay   = (((int)(idy[i] | 1)) != 0);    // Assume daytime when is_day is missing

    int16_t owmId;
    const char* icon;
    wmo_to_owm(code, isDay, owmId, icon);

    WxHourlyForecast[r].Dt   = t[i].as<long>();
    WxHourlyForecast[r].Id   = owmId;
    WxHourlyForecast[r].Icon = icon;
  }

  // Clear the trailing slot so no stale data from a previous run survives.
  // (main.cpp only reads 0..12; this just leaves a clean state.)
  WxHourlyForecast[OM_POINTS].Dt = 0;
  WxHourlyForecast[OM_POINTS].Id = 0;
  return true;
}

// ---------------------------------------------------------------------------
// Fetching
// ---------------------------------------------------------------------------
bool om_fetch_all() {
  // Seed the fallback offset first: even if everything below fails, the screen
  // will not fall back to displaying UTC.
  WxDailyconditions[0].Timezonesec = TZ_OFFSET_SEC;

  // IMPORTANT: tls / http / payload must stay local to this function.
  // The TLS handshake peaks at roughly 35-45KB and has to be released before
  // setup() allocates the two 15000-byte framebuffers, so never make these
  // static or file-scope.
  WiFiClientSecure tls;
  tls.setCACert(ISRG_ROOT_X1_PEM);   // Stores the pointer only, no copy. The
                                     // array lives in .rodata and stays valid
                                     // for the lifetime of the program.
  tls.setHandshakeTimeout(OM_HS_TIMEOUT);

  HTTPClient http;
  http.setConnectTimeout(OM_CONN_TIMEOUT);
  http.setTimeout(OM_CONN_TIMEOUT);
  http.setReuse(false);

  const String uri = om_build_uri();
  Serial.printf("[OM] GET %s:%u%s\n", OM_HOST, (unsigned)OM_PORT, uri.c_str());

  if (!http.begin(tls, OM_HOST, OM_PORT, uri, true)) {   // Trailing true = https
    Serial.println("[OM] http.begin() 失败");
    http.end();
    tls.stop();
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OM] HTTP %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    tls.stop();
    return false;
  }

  // Open-Meteo replies with Transfer-Encoding: chunked.
  // getString() is mandatory so HTTPClient dechunks the body; feeding
  // getStream() straight into the JSON parser would hand it the chunk headers
  // and fail at the very first byte. The body is only ~800 bytes, so buffering
  // it is cheap.
  String payload = http.getString();
  Serial.printf("[OM] body %u bytes\n", (unsigned)payload.length());

  const bool ok = om_decode(payload);

  http.end();
  tls.stop();
  return ok;   // tls / http / payload are all destroyed on return; heap is freed
}
