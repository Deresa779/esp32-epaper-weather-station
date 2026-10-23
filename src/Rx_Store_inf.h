#ifndef RX_STORE_INF_H_
#define RX_STORE_INF_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

typedef struct {
    long Dt;
    int32_t Timezonesec; // from -43200 to 50400
    float Temperature_MAX;
    float Temperature_MIN;
    int Id;              // OWM-compatible weather condition ID (200-804).
                         // Open-Meteo WMO codes are already converted into this
                         // scheme inside weather_openmeteo.cpp, so this field does
                         // NOT hold the raw value returned by the API. The
                         // downstream Paint_recommend() relies on this convention.
    String Icon;         // OWM icon code, restricted to the 18 values
                         // 01d/01n/02d/02n/.../50d/50n
} WeatherType; 

typedef struct {
    char date[15];
    char weekday[12];
} DateType;

extern const int max_readings;//The weather information start from one hour before the current time , to get present weather and forecast for the next 12h we need weather data of fourteen hours.

extern WeatherType WxDailyconditions[];
extern WeatherType WxHourlyForecast[];

bool DecodeWeather(WiFiClient& json, const String& Type);
DateType ConvertUnixTime_date(long unix_time, int32_t timezonesec);
String ConvertUnixTime_hourtime(long unix_time, int32_t timezonesec);
bool obtain_wx_data(WiFiClient& client, const String& RequestType);

// OpenWeatherMap data source adapter entry point (owns its own WiFiClient);
// dispatched by Wx_FetchAll() in weather_source.cpp.
bool owm_fetch_all();

#endif /* RX_STORE_INF_H_ */