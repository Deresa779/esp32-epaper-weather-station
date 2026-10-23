#include <Arduino.h>   
#include <WiFi.h>              // Built-in
#include "time.h"              // Built-in
#include <SPI.h>               // Built-in
#include <ArduinoJson.h> 
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "icon.h"
#include <stdlib.h>
#include "setup_data.h"
#include "Rx_Store_inf.h"
#include "icon.h"
#include "output_image.h"
#include "fonts.h"
#include "lang.h"
#include "weather_source.h"

#define SCREEN_WIDTH  400.0    
#define SCREEN_HEIGHT 300.0

// Matches the Waveshare e-Paper ESP32 Driver Board; SPI is bit-banged, so any GPIO works.
// Keep these in sync with DEV_Config.h and the wiring. Avoid GPIO 6-11 (flash), 34-39 (input only), 0/2/12/15 (strapping).
static const uint8_t EPD_BUSY = 25;
static const uint8_t EPD_CS   = 15;
static const uint8_t EPD_RST  = 26; 
static const uint8_t EPD_DC   = 27; 
static const uint8_t EPD_SCK  = 13;
static const uint8_t EPD_MISO = 12;  
static const uint8_t EPD_MOSI = 14;

int wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
long StartTime = 0;
long SleepDuration = 60; // Sleep time in minutes
int  WakeupTime    = 6;  // Don't wakeup until after 06:00 to save battery power
int  SleepTime     = 0; // Sleep after 00:00 to save battery power
long currentUnixTime;

// Function declarations:
uint8_t StartWiFi();
boolean SetupTime();
boolean UpdateLocalTime();
void InitialiseDisplay();
void StopWiFi();
void WaitForDisplayReady();
void PaintWeather(WeatherType Wx1[], WeatherType Wx2[], long Unixs, UBYTE *imageBlack, UBYTE *imageRed);
void Paint_Basicinf(WeatherType Wx1[], long Unixs, UBYTE *imageBlack);
void Paint_Futureinf(WeatherType Wx1[], WeatherType Wx2[], UBYTE *imageBlack, UBYTE *imageRed);
void Paint_recommend(WeatherType Wx[], UBYTE *imageBlack, UBYTE *imageRed);
void Print_icon(int x, int y, int w, int h, String& Icon, int& Id, UBYTE *imageBlack, UBYTE *imageRed);
void Print_time(sFONT* Font, int x, int y, int w, int h, int32_t Timezonesec, long Dt);
void DrawStringWithAlignmentBW(bool type, sFONT* Font, const char* text, UWORD x0, UWORD y0, UWORD box_w, UWORD box_h, bool center_horizontal);
void DrawStringWithAlignmentCN(const char* text, UWORD x0, UWORD y0, UWORD box_w, UWORD box_h, bool center_horizontal);
void BeginSleep();

// Setup code, run everytime to refresh the infformation of the screen.
void setup() {
  StartTime = millis();
  DEV_Module_Init();
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    Serial.println(CurrentHour);
    Serial.println(WakeupTime);
    Serial.println(SleepTime);
    if ((CurrentHour >= WakeupTime && CurrentHour <= 23) || (CurrentHour >= 0 && CurrentHour <= SleepTime)) {
      // Fetching is delegated to the data-source adapter layer. Open-Meteo
      // returns daily and hourly in a single request (one TLS handshake);
      // OpenWeatherMap still performs its original two plain HTTP requests.
      // Switching sources only means editing WxDataSource in setup_data.cpp.
      bool RxOk = false;
      for (byte Attempts = 1; Attempts <= 2 && !RxOk; Attempts++) {
        Serial.printf("Weather fetch %u/2 via %s\n", (unsigned)Attempts, Wx_SourceName());
        RxOk = Wx_FetchAll();
      }
      if (RxOk) { // Only proceed once both datasets are in hand
        Serial.println("Weather data collection successful");
        StopWiFi(); // Reduces power consumption
        DEV_Delay_ms(2000);
        Serial.println("Entered succesfully!");
        InitialiseDisplay(); // Give screen time to initialise by getting weather data!
        ///Create a new image cache named IMAGE_BW and fill it with white
        UBYTE *BlackImage, *RYImage; // Red or Yellow
        UWORD Imagesize = ((EPD_4IN2B_V2_WIDTH % 8 == 0) ? (EPD_4IN2B_V2_WIDTH / 8 ) : (EPD_4IN2B_V2_WIDTH / 8 + 1)) * EPD_4IN2B_V2_HEIGHT;
        if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
          printf("Failed to apply for black memory...\r\n");
          while(1);
        }
        if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL) {
          printf("Failed to apply for red memory...\r\n");
          while(1);
        }
        printf("NewImage:BlackImage and RYImage\r\n");
        Paint_NewImage(BlackImage, EPD_4IN2B_V2_WIDTH, EPD_4IN2B_V2_HEIGHT, 180, WHITE);
        Paint_NewImage(RYImage, EPD_4IN2B_V2_WIDTH, EPD_4IN2B_V2_HEIGHT, 180, WHITE);
        Paint_SelectImage(BlackImage);
        Paint_Clear(WHITE);
        Paint_SelectImage(RYImage);
        Paint_Clear(WHITE);
        PaintWeather(WxDailyconditions, WxHourlyForecast, currentUnixTime, BlackImage, RYImage);
        Serial.println("Start displaying content...");
        EPD_4IN2B_V2_Display(BlackImage, RYImage);
        WaitForDisplayReady();   // Used to be WaitForDisplayReady; - the missing
                                 // parentheses meant it never waited, so the
                                 // framebuffers were freed mid-refresh.
        free(BlackImage);
        free(RYImage);
        BlackImage = NULL;
        RYImage = NULL;
      }
    }
  }
  BeginSleep();
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:

uint8_t StartWiFi() {
  Serial.print("\r\nConnecting to: "); Serial.println(String(ssid));
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection) {
    connectionStatus = WiFi.status();
    if (millis() > start + 10000) { // Wait 10-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
  return connectionStatus;
}

// function to set up time, which doesn't affect the date display on the e-ink screen, only the timing of the next refresh
boolean SetupTime() {
  configTime(0, 0, ntpServer, ntpServer2); // (gmtOffset_sec, daylightOffset_sec, ntpServer, backup)
  setenv("TZ", Timezone, 1);  // setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  bool TimeStatus = UpdateLocalTime();
  Serial.println("Time set successfully.");
  return TimeStatus;
}

boolean UpdateLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) { 
    Serial.println("Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  time_t unixTime = time(nullptr);  
  currentUnixTime = static_cast<long>(unixTime);
  //The following 4 line code is for debugging to verify if the time has been set correctly (to the hour), remove it if you have verified it.
  char timeinfostorage[22];
  strftime(timeinfostorage, sizeof(timeinfostorage), "%H %d/%m/%y", &timeinfo);
  Serial.print("Current Unixtime is : ");
  Serial.println(timeinfostorage);
  
  return true; 
}

void InitialiseDisplay() {
  Serial.print("Begin to initialize.");
  EPD_4IN2B_V2_Init();
  Serial.print("Begin to clear the screen");
  EPD_4IN2B_V2_Clear();
  delay(10000);
  WaitForDisplayReady();
  Serial.println("InitialiseDisplay successfully!");
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

void WaitForDisplayReady() {
    while (DEV_Digital_Read(EPD_BUSY) == 1) { 
        Serial.println("e-Paper is busy...");
        delay(100); 
    }
    Serial.println("e-Paper is ready!");
}

void PaintWeather(WeatherType Wx1[], WeatherType Wx2[], long Unixs, UBYTE *imageBlack, UBYTE *imageRed) {
  Paint_Basicinf(Wx1, Unixs, imageBlack);
  Paint_Futureinf(Wx1, Wx2, imageBlack, imageRed);
  Paint_recommend(Wx2, imageBlack, imageRed);
  Serial.println("Draw display successfully!");
}

void Paint_Basicinf(WeatherType Wx1[], long Unixs, UBYTE *imageBlack) {
    Paint_SetRotate(0); 
    DateType Dateinf = ConvertUnixTime_date(Unixs, Wx1[0].Timezonesec);
    Paint_SelectImage(imageBlack);
    DrawStringWithAlignmentBW(false, &Font24, Dateinf.date, 15, 20, 200, 26, false);  // Left aligned
    // Weekday: Chinese uses the custom bitmap font FontCN_zh (32x32, 32px per
    // glyph); other languages use the ASCII font.
    if (Lang_IsChinese()) {
      DrawStringWithAlignmentCN(Dateinf.weekday, 15, 64, 140, 32, false);  // Left aligned
    } else {
      DrawStringWithAlignmentBW(false, &Font20, Dateinf.weekday, 15, 73, 80, 20, false);  // Left aligned
    }
    // Title: text always comes from the string tables in lang.cpp; the Chinese
    // variant additionally switches to FontCN_zh.
    if (Lang_IsChinese()) {
      // Left aligned. Occupies y 106-138, staying clear of the y=140 divider.
      DrawStringWithAlignmentCN(Lang_GetTitle(), 15, 106, 140, 32, false);
    } else {
      DrawStringWithAlignmentBW(false, &Font16, Lang_GetTitle(), 15, 124, 100, 16, false);  // Left aligned
    }
    Paint_DrawLine(15, 140, 160, 140, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawRectangle(220, 15, 380, 140, BLACK, DOT_PIXEL_2X2 , DRAW_FILL_EMPTY);
    Paint_DrawLine(220, 118, 380, 118, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    String Temperature_max = String(Wx1[0].Temperature_MAX, 1);  
    String Temperature_min = String(Wx1[0].Temperature_MIN, 1);  
    DrawStringWithAlignmentBW(false, &Font16, Temperature_min.c_str(), 223, 121, 70, 17, true);  // Centered
    DrawStringWithAlignmentBW(true, &Font16, "/", 293, 121, 20, 17, true);  // Centered
    DrawStringWithAlignmentBW(false, &Font16, Temperature_max.c_str(), 313, 121, 70, 17, true);  // Centered
    
    delay(200);
    Serial.println("Painted Display_Basicinf successfully!");
}

void Paint_Futureinf(WeatherType Wx1[], WeatherType Wx2[], UBYTE *imageBlack, UBYTE *imageRed) {
  // print present weather
  Paint_SetRotate(0); 
  Print_icon(140, 60, 48, 48, Wx2[0].Icon, Wx2[0].Id, imageBlack, imageRed);
  delay(200);
  int Wx_icontimex[] = {15, 78, 141, 204, 267, 330};
  int Wx_iconyl[] = {145, 215};
  int Wx_timeyl[] = {195, 265};
  int8_t Wx_iconw = 48;
  int8_t Wx_iconh = 48;
  int8_t Wx_timew = 48;
  int8_t Wx_timeh = 15;
  // print forecast weather 
  byte n = 1;
  // count columns l
  for(byte l = 0 ; l < 2 ; l++) { 
    // Count rows r
    for(byte r = 0 ; r < 6 ; r++) { 
      // Count the number 
      Serial.println(n);
      Print_icon(Wx_icontimex[r], Wx_iconyl[l], Wx_iconw, Wx_iconh, Wx2[n].Icon, Wx2[n].Id, imageBlack, imageRed);
      Paint_SelectImage(imageBlack);
      Print_time(&Font16, Wx_icontimex[r], Wx_timeyl[l], Wx_timew, Wx_timeh, Wx1[0].Timezonesec, Wx2[n].Dt);
      n++;
    }
  }
}

// distinguish the type of weather and which thing is recommended to bring with, the meaning of each Id shows on: https://openweathermap.org/weather-conditions
// The 12 icons drawn below are the advice icons listed in the README, in this order:
//   1 stay at home   2 rain + hail   3 rain + snow   4 rain + lightning   5 rain
//   6 hail   7 snow   8 lightning   9 face shield   10 mask   11 wool hat   12 sunglasses
void Paint_recommend(WeatherType Wx[], UBYTE *imageBlack, UBYTE *imageRed) {
  int ra = 0, sn = 0, th = 0, sl = 0; // ra=rain, sn=snow, th=lightning, sl=hail
  int um = 0, st = 0, ma = 0, sa = 0, wa = 0, su = 0; // um=umbrella, st=wool hat, ma=mask,
                                                      // sa=face shield (sand / dust / ash),
                                                      // wa=stay at home (tornado / squalls),
                                                      // su=sunglasses
  // statistics on the specific type of weather for the present plus the next 12 hours, and the respective recommendations for what to take out of the house
  for(byte r = 0 ; r < max_readings-1 ; r++ ) {
    // Lightning (OWM 2xx thunderstorm), drizzle and rain
    Serial.print("Wx[");
    Serial.print(r);
    Serial.print("].Id=");
    Serial.print(Wx[r].Id);
    if(Wx[r].Id >= 200 && Wx[r].Id < 600) {    
      um++;
      if(Wx[r].Id == 511) ra++, sn++;                 // 511 freezing rain counts as rain AND snow
      else if(Wx[r].Id >= 210 && Wx[r].Id <= 221) th++;  // 210-221 plain thunderstorm
      else if(Wx[r].Id < 300) th++, ra++;              // 200-209 / 230-232 thunderstorm with rain or drizzle
      else ra++;                                       // 3xx drizzle and 5xx rain
    }
    // Snow 
    else if(Wx[r].Id >= 600 && Wx[r].Id < 700) {
      if(Wx[r].Id >= 611 && Wx[r].Id <= 613) {
        // OWM calls 611-613 sleet; the artwork and the README call it hail, which
        // is why this feeds the hail icon (icon 6) and not the snow icon.
        sl++;
        um++;
      }  
      else if(Wx[r].Id <= 601 || (Wx[r].Id >= 620 && Wx[r].Id <= 621)) {
        // Light and moderate snowfall (600, 601, 620, 621). Not worth opening an
        // umbrella, so the advice is to keep your head warm: wool hat (icon 11).
        sn++;
        st++;
      }
      else {
        // Heavy snowfall (602, 622) and rain mixed with snow (615, 616) do call
        // for cover: umbrella with snow (icon 7).
        sn++;
        um++;
      }
    }
    // Atmosphere 
    else if(Wx[r].Id >= 700 && Wx[r].Id < 800) {
      if(Wx[r].Id == 701 || Wx[r].Id == 741) ;                 // mist / fog: no advice
      else if(Wx[r].Id == 711 || Wx[r].Id == 721) ma++;        // smoke / haze -> mask
      else if(Wx[r].Id == 771) wa++;                           // squalls
      else if(Wx[r].Id == 781) wa++;                           // tornado
      else sa++;                                               // sand / dust / ash -> face shield
    }  
    // Extreme (OWM 9xx). Hail is the only code in this group that carries an advice,
    // and it is only produced by the OWM source: Open-Meteo reports hail through WMO
    // 96/99, which wmo_to_owm() folds into 201/202, handled by the lightning branch.
    else if(Wx[r].Id == 906) {                                 // hail -> umbrella (icon 6)
      sl++;
      um++;
    }
    // Clear and Clouds 
    else if(Wx[r].Id >= 800) { 
      if(Wx[r].Id == 800) su++;                                // clear sky -> sunglasses
      else ;
    }  
  }
  Serial.print("ra: "); Serial.print(ra);
  Serial.print(", sn: "); Serial.print(sn);
  Serial.print(", th: "); Serial.print(th);
  Serial.print(", sl: "); Serial.print(sl);
  Serial.print(", um: "); Serial.print(um);
  Serial.print(", st: "); Serial.print(st);
  Serial.print(", ma: "); Serial.print(ma);
  Serial.print(", sa: "); Serial.print(sa);
  Serial.print(", wa: "); Serial.print(wa);
  Serial.print(", su: "); Serial.println(su);
  int x = 227, y = 18, w = 144, h =90;
  // print out the most important things to bring based on priority level, you can adjust the sorting to suit your needs 
  // the priority is stay at home > umbrella (rain, optionally combined with hail / snow /
  // lightning) > hail > wool hat > snow umbrella > lightning > face shield > mask > sunglasses
  if(wa > 0) {                                   // icon 1:  stay at home
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_wa_b[6], x, y, w, h);
    Paint_SelectImage(imageRed);
    Paint_DrawImage(&gImage_wa_r[6], x, y, w, h);
  }
  // um is only useful here when it can be combined with rain; otherwise the branch would
  // swallow hail / snow / lightning and leave the slot blank, which the README reads as
  // "nothing to bring". So bail out to the single-condition icons below instead.
  else if(um > 0 && ra > 0) {
    // the recommend icon will also print the weather correspond, It can print up to two different weather
    // arranged in the order of rain&hail > rain&snow > rain&lightning > rain
    if(ra > 0) { 
      if(sl > 0) {                               // icon 2:  umbrella with rain and hail
        Paint_SelectImage(imageBlack);
        Paint_DrawImage(&gImage_um12[6], x, y, w, h);      
      }
      else if(sn > 0) {                          // icon 3:  umbrella with rain and snow
        Paint_SelectImage(imageBlack);
        Paint_DrawImage(&gImage_um13[6], x, y, w, h);
      }
      else if(th > 0) {                          // icon 4:  umbrella with rain and lightning
        Paint_SelectImage(imageBlack);
        Paint_DrawImage(&gImage_um14[6], x, y, w, h);
      }
      else {                                     // icon 5:  umbrella with rain
        Paint_SelectImage(imageBlack);
        Paint_DrawImage(&gImage_um1[6], x, y, w, h);   
      }
    }
  }
  else if(sl > 0) {                              // icon 6:  umbrella with hail
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_um2[6], x, y, w, h);
  } 
  // The wool hat is tested before the snow umbrella on purpose: light snow only
  // raises st, and it must win over icon 7, which every snow code also sets.
  else if(st > 0) {                              // icon 11: wool hat (light snow)
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_st[6], x, y, w, h);    
  }
  else if(sn > 0) {                              // icon 7:  umbrella with snow
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_um3[6], x, y, w, h);
  }
  else if(th > 0) {                              // icon 8:  umbrella with lightning
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_um4[6], x, y, w, h);
  }
  else if(sa > 0) {                              // icon 9:  face shield
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_sa[6], x, y, w, h);   
  } 
  else if(ma > 0) {                              // icon 10: mask
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_ma[6], x, y, w, h);   
  }
  else if(su > 0) {                              // icon 12: sunglasses
    Paint_SelectImage(imageBlack);
    Paint_DrawImage(&gImage_su[6], x, y, w, h); 
  }
}

void Print_icon(int x, int y, int w, int h, String& Icon, int& Id, UBYTE *imageBlack, UBYTE *imageRed) {
  delay(200);
  imagedataType imageinf = iconcode_to_icon(Icon, Id);
  Serial.println("Start to print icon:");
  switch(imageinf.colortype) {
    case 1:
      if (imageinf.image_b != nullptr) {
        Paint_SelectImage(imageBlack);
        Serial.println("Start to print iconb");
        Paint_DrawImage(&imageinf.image_b[6], x, y, w, h);
      }
      else Serial.println("Failed to print iconb");
      break;
    case 2:
      if (imageinf.image_r != nullptr) {
        Paint_SelectImage(imageRed);
        Serial.println("Start to print iconr");
        Paint_DrawImage(&imageinf.image_r[6], x, y, w, h);
      }
      else Serial.println("Failed to print iconr");
      break; 
    case 3:
      Serial.println("Start to print iconb_and_r");
      if (imageinf.image_b != nullptr) {
        Paint_SelectImage(imageBlack);
        Paint_DrawImage(&imageinf.image_b[6], x, y, w, h);
      }
      if (imageinf.image_r != nullptr) {
        Paint_SelectImage(imageRed);
        Paint_DrawImage(&imageinf.image_r[6], x, y, w, h);
      }
      else Serial.println("Failed to print iconb_and_r");
    default : 
      Serial.println("Error: Colortype");
      Serial.print("Colortype = ");
      Serial.println(imageinf.colortype);
  }
}

void Print_time(sFONT* Font, int x, int y, int w, int h, int32_t Timezonesec, long Dt) {
  String hourtime = ConvertUnixTime_hourtime(Dt, Timezonesec);
  DrawStringWithAlignmentBW(false, &Font12, hourtime.c_str(), x, y, w, h, true);
}

void DrawStringWithAlignmentBW(bool type, sFONT* Font, const char* text, UWORD x0, UWORD y0, UWORD box_w, UWORD box_h, bool center_horizontal) {
  uint16_t text_length = strlen(text);  
  uint16_t string_width = text_length * Font->Width;  
  uint16_t string_height = Font->Height;  

  int16_t x1, y1;

  // Horizontal alignment
  x1 = center_horizontal ? x0 + (box_w - string_width) / 2 : x0;

  // Vertical alignment
  y1 = y0 + (box_h - string_height) / 2;

  if (x1 < 0) x1 = 0;
  if (x1 + string_width > SCREEN_WIDTH) {
      string_width = SCREEN_WIDTH - x1;
      text_length = string_width / Font->Width;
  }

  // Truncate string if needed
  char truncated_string[text_length + 1];
  strncpy(truncated_string, text, text_length);
  truncated_string[text_length] = '\0';

  if (x1 + string_width > SCREEN_WIDTH || y1 + Font->Height > SCREEN_HEIGHT) {
    Serial.println("Error: Drawing outside of display bounds.");
  }

  if (y1 + string_height <= SCREEN_HEIGHT) {
      // Draw characters or full string
      if (type) {
          for (int i = 0; i < text_length; i++) {
              Paint_DrawChar(x1 + i * Font->Width, y1, truncated_string[i], Font, BLACK, WHITE);
          }
      } else {
          Paint_DrawString_EN(x1, y1, truncated_string, Font, WHITE, BLACK);
      }
  }
}

// Counts the characters in a UTF-8 string (a Han character counts as one).
static int UTF8CharCount(const char* s) {
  int n = 0;
  while (*s) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80)        s += 1;   // ASCII
    else if ((c & 0xE0) == 0xC0) s += 2;
    else if ((c & 0xF0) == 0xE0) s += 3;   // Han characters fall in this class
    else                 s += 4;
    n++;
  }
  return n;
}

// Draws Chinese text. Alignment logic mirrors DrawStringWithAlignmentBW; the
// difference is that it uses the custom bitmap font FontCN_zh and routes
// through Paint_DrawString_CN.
void DrawStringWithAlignmentCN(const char* text, UWORD x0, UWORD y0, UWORD box_w, UWORD box_h, bool center_horizontal) {
  uint16_t string_width  = UTF8CharCount(text) * FontCN_zh.Width;
  uint16_t string_height = FontCN_zh.Height;

  int16_t x1 = center_horizontal ? x0 + (int16_t)(box_w - string_width) / 2 : (int16_t)x0;
  if (x1 < 0) x1 = 0;
  int16_t y1 = y0 + (int16_t)(box_h - string_height) / 2;
  if (y1 < 0) y1 = 0;

  if (x1 + string_width > SCREEN_WIDTH || y1 + string_height > SCREEN_HEIGHT) {
    Serial.println("Error: CN string drawing outside of display bounds.");
  }
  Paint_DrawString_CN(x1, y1, text, &FontCN_zh, BLACK, WHITE);
}

void BeginSleep() {
  UpdateLocalTime(); // only used to update the real-time clock 
  EPD_4IN2B_V2_Sleep();
  long SleepTimer = SleepDuration * 60; // theoretical sleep duration
  long offset = (CurrentMin % SleepDuration) * 60 + CurrentSec; // number of seconds elapsed after last theoretical wake-up time point
  if (offset > SleepDuration/2 * 60) { // waking up too early will cause <offset> too large
    offset -= SleepDuration * 60; // then we should make it negative, so as to extend this coming sleep duration
  }
  esp_sleep_enable_timer_wakeup((SleepTimer - offset) * 1000000LL); // do compensation to cover ESP32 RTC timer source inaccuracies
  Serial.print("Calculated offset: "); 
Serial.println(offset);
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, HIGH);
#endif
  Serial.println("Entering " + String(SleepTimer - offset) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();      // Sleep for 1 hour, depending of the parameters that you set.
}

