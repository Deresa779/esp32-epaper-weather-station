#include "weather_source.h"
#include "setup_data.h"
#include "Rx_Store_inf.h"
#include "weather_openmeteo.h"

// This layer does exactly one job: dispatch to the right implementation based
// on WxDataSource.
// Each implementation owns its own transport (OWM uses WiFiClient, Open-Meteo
// uses WiFiClientSecure), so callers no longer need to pass a client object.

bool Wx_FetchAll() {
  if (WxDataSource == WX_SRC_OWM) {
    return owm_fetch_all();
  }
  return om_fetch_all();
}

const char* Wx_SourceName() {
  return (WxDataSource == WX_SRC_OWM) ? "OpenWeatherMap" : "Open-Meteo";
}
