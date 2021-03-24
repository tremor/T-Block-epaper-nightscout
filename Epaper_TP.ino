//#include <*>

/*
* Based on the test code written by GxEPD / U8g2_for_Adafruit_GFX
* https://github.com/ZinggJM/GxEPD
* https://github.com/olikraus/U8g2_for_Adafruit_GFX
*
*/


#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "secret.h"
#include "ArduinoJson.h"
#include "FreeMonoBold30pt7b.h"
#include "FreeMonoBold35pt7b.h"
#include "FreeMonoBold40pt7b.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  120       /* Time ESP32 will go to sleep (in seconds) */

const int16_t arrow_pos_x = 10;
const int16_t arrow_pos_y = 110;
const int16_t value_pos_x = 80;
const int16_t value_pos_y = 160;
const int utcOffset = 1;        // Central European Time

extern const unsigned char logoIcon[280];

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
TTGOClass *twatch = nullptr;
GxEPD_Class *ePaper = nullptr;
PCF8563_Class *rtc = nullptr;
AXP20X_Class *power = nullptr;
Button2 *btn = nullptr;
uint32_t seupCount = 0;
bool pwIRQ = false;
bool touch_vaild = false;
uint32_t loopMillis = 0;
int16_t x, y;


void setupDisplay()
{
    u8g2Fonts.begin(*ePaper); // connect u8g2 procedures to Adafruit GFX
    u8g2Fonts.setFontMode(1);                   // use u8g2 transparent mode (this is default)
    u8g2Fonts.setFontDirection(0);              // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);  // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);  // apply Adafruit GFX color
    u8g2Fonts.setFont(u8g2_font_inr38_mn); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
}

#define RE_INIT_NEEDED
bool wifiConnect() {
  if(WiFi.status() == WL_CONNECTED) {
    return true;
  }
#ifdef RE_INIT_NEEDED
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect();
#endif

  if (!WiFi.getAutoConnect() || ( WiFi.getMode() != WIFI_STA) || ((WiFi.SSID() != ssid) && String(ssid) != "........"))
  {
    Serial.println();
    Serial.print("WiFi.getAutoConnect()=");
    Serial.println(WiFi.getAutoConnect());
    Serial.print("WiFi.SSID()=");
    Serial.println(WiFi.SSID());
    WiFi.mode(WIFI_STA); // switch off AP
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  }

  int ConnectTimeout = 30; // 15 seconds
  while (WiFi.status() != WL_CONNECTED) {
    if (--ConnectTimeout <= 0)
    {
      Serial.println();
      Serial.println("WiFi connect timeout");
      return false;
    }
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
  }
  Serial.println();
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(2000);

  return true;
}

void mainPage(bool fullScreen)
{
    uint16_t tbw, tbh;
    static int16_t lastX, lastY;
    static uint16_t lastW, lastH;
    static uint8_t hh = 0, mm = 0;
    static uint8_t lastWeek = 0;
    static uint8_t lastDay = 0;
    static uint32_t lastSeupCount = 0;
    static int16_t getX, getY;
    static uint16_t getW, getH;
    char buff[64] = "00:00";
    const char *weekChars[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };

    if (lastSeupCount != seupCount) {
        lastSeupCount = seupCount;
        u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312a); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
        snprintf(buff, sizeof(buff), "%u 步", seupCount);
        u8g2Fonts.setCursor(getX - getW, getY);
        u8g2Fonts.print(buff);
    }

    RTC_Date d = rtc->getDateTime();
    if (mm == d.minute && !fullScreen) {
        return ;
    }

    mm = d.minute;
    hh = d.hour;
    if (lastDay != d.day) {
        lastDay = d.day;
        lastWeek  = rtc->getDayOfWeek(d.day, d.month, d.year);
        fullScreen = true;
    }

    snprintf(buff, sizeof(buff), "%02d:%02d", hh, mm);

    if (fullScreen) {
        lastX = 25;
        lastY = 100;
        ePaper->drawBitmap(5, 5, logoIcon, 75, 28, GxEPD_BLACK);

        //BATTERY ICON
        u8g2Fonts.setFont(u8g2_font_battery19_tn);
        u8g2Fonts.setCursor(175, 20);                // start writing at this position
        u8g2Fonts.setFontDirection(3);              // left to right (this is default)
        u8g2Fonts.print(4);
        u8g2Fonts.setFontDirection(0);              // left to right (this is default)


        ePaper->drawFastHLine(10, 40, ePaper->width() - 20, GxEPD_BLACK);
        ePaper->drawFastHLine(10, 150, ePaper->width() - 20, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_inr38_mn  ); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

        //u8g2Fonts.setCursor(lastX, lastY);                // start writing at this position
        //u8g2Fonts.print(buff);

        /* calculate the size of the box into which the text will fit */
        lastH = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
        lastW = u8g2Fonts.getUTF8Width(buff);

        u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312a); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

        snprintf(buff, sizeof(buff), "%s", weekChars[lastWeek]);

        tbh = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
        tbw = u8g2Fonts.getUTF8Width(buff);

        int16_t x, y;
        x = ((ePaper->width() - tbw) / 2) ;
        y = ((ePaper->height() - tbh) / 2) + 40  ;

        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(buff);
        u8g2Fonts.setCursor(20, y + 50);

        //u8g2Fonts.print("你今天已经走了:");
        //u8g2Fonts.print("1000步");

        getX = u8g2Fonts.getCursorX();
        getY = u8g2Fonts.getCursorY();
        getH  = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
        getW = u8g2Fonts.getUTF8Width("1000步");
        //ePaper->update();


    } else {
        //u8g2Fonts.setFont(u8g2_font_inr38_mn); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
        //ePaper->fillRect(lastX, lastY - u8g2Fonts.getFontAscent() - 3, lastW, lastH, GxEPD_WHITE);
        //ePaper->fillScreen(GxEPD_WHITE);
        //ePaper->setTextColor(GxEPD_BLACK);
        //lastW = u8g2Fonts.getUTF8Width(buff);
        //u8g2Fonts.setCursor(lastX, lastY);
        //u8g2Fonts.print(buff);
        //ePaper->updateWindow(lastX, lastY - u8g2Fonts.getFontAscent() - 3, lastW, lastH, false);
    }
}

const int validMinTime = (8 * 3600 * 2);

bool setClock() {
  time_t nowSecs = time(nullptr);
  if(nowSecs < validMinTime) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
    Serial.print("Waiting for NTP time sync: ");
    nowSecs = time(nullptr);
    int ConnectTimeout = 30; // 15 seconds
    while (nowSecs < validMinTime) {
      if (--ConnectTimeout <= 0)
      {
        Serial.println();
        Serial.println("Waiting for NTP time timeout");
        return false;
      }
      delay(500);
      Serial.print(".");
      yield();
      nowSecs = time(nullptr);
    }
    Serial.println();
  }

  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print("Current time: ");
  Serial.println(asctime(&timeinfo));
  return true;
}

RTC_DATA_ATTR double lastValueDate = 0;

bool getValue() {
  bool ret = false;
  HTTPClient http;
  http.begin(api_url, root_ca); //Specify the URL and certificate
//  http.begin(api_url, "82:6F:E8:01:13:78:A9:2A:76:2B:B6:9F:2F:8C:EB:11:D2:4E:2E:4D:9F:66:FA:71:E8:C7:88:55:FB:C2:BB:AB"); 
  http.addHeader("accept", "application/json");
  http.addHeader("api_secret", api_secret);
  int httpCode = http.GET();    //Make the request

  if (httpCode < 0) { //Check for the returning code
    Serial.print("Error < 0 on HTTP request ");
    Serial.println(httpCode);
  } else {
    String payload = http.getString();
    if (httpCode != 200) {
      Serial.print("Error not 200 on HTTP request ");
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      // Allocate the JSON document
      // Use https://arduinojson.org/v6/assistant/ to compute the capacity.
      const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
      DynamicJsonDocument doc(capacity);
      auto error = deserializeJson(doc, payload); //Parse message
      if (error) { //Check for errors in parsing
        Serial.println("Parsing HTTP response failed");
        Serial.println(payload);
      } else {
        JsonObject root_0 = doc[0];
        const char* root_0_type = root_0["type"];
        if(strcmp(root_0_type, "sgv") != 0) {
          Serial.print("JSON response not of type 'sgv' ");
          Serial.println(root_0_type);
        } else {
          double date = root_0["date"].as<double>();
          date = date / 1000;
          if(lastValueDate == date) {
            Serial.print("Date value unchanged ");
            Serial.println(date);
          } else {
            lastValueDate = date;
            Serial.print("Date value changed ");
            Serial.println(date);

            //const char* dateString = root_0["dateString"];
            const char* direction = root_0["direction"];
            int sgv = root_0["sgv"];
            Serial.print("SGV value ");
            Serial.println(sgv);
            showValue(date, sgv, direction);
          }
          ret = true;
        }
      }
    }
  }
  http.end(); //Free the resources
  return ret;
}
#define SECS_PER_HOUR (3600UL)

void convertDate(double date, char* buffer, int buffersize) {
  int totalSeconds = (int)date;
  time_t rawtime = (time_t)totalSeconds;
  struct tm * timeinfo;
  timeinfo = localtime (&rawtime);

  // UTC to local time
  rawtime += (utcOffset + dstOffset (timeinfo->tm_mday, timeinfo->tm_mon, timeinfo->tm_year + 1900, timeinfo->tm_hour)) * SECS_PER_HOUR;
  timeinfo = localtime (&rawtime);
  
  //strftime(buffer, buffersize, "%Y-%m-%d %H:%M:%S", timeinfo);
  //strftime(buffer, buffersize, "%d.%m. %H:%M", timeinfo);
  strftime(buffer, buffersize, "%H:%M", timeinfo);
}

/* This function returns the DST offset for the current UTC time.
 * This is valid for the EU, for other places see
 * http://www.webexhibits.org/daylightsaving/i.html
 * 
 * Results have been checked for 2012-2030 (but should work since
 * 1996 to 2099) against the following references:
 * - http://www.uniquevisitor.it/magazine/ora-legale-italia.php
 * - http://www.calendario-365.it/ora-legale-orario-invernale.html
 */
byte dstOffset (byte d, byte m, unsigned int y, byte h) {
  // Day in March that DST starts on, at 1 am
  byte dstOn = (31 - (5 * y / 4 + 4) % 7);

  // Day in October that DST ends  on, at 2 am
  byte dstOff = (31 - (5 * y / 4 + 1) % 7);

  if ((m > 3 && m < 10) ||
      (m == 3 && (d > dstOn || (d == dstOn && h >= 1))) ||
      (m == 10 && (d < dstOff || (d == dstOff && h <= 1))))
    return 1;
  else
    return 0;
}

void showValue(const double date, const int sgv, const char* direction){
  ePaper->setRotation(1);
  ePaper->fillScreen(GxEPD_WHITE);
  ePaper->setTextColor(GxEPD_BLACK);
  ePaper->setFont(&FreeMonoBold9pt7b);
  ePaper->setCursor(0, 15);
  ePaper->print("IP: ");
  ePaper->println(WiFi.localIP());

  char dateString[13];
  convertDate(date, dateString, sizeof(dateString));
  ePaper->setFont(&FreeMono18pt7b);
  ePaper->setCursor(0, 42);
  ePaper->print("Wert von ");
  ePaper->println(dateString);
  //display.println();
  
  ePaper->setFont(&FreeMonoBold35pt7b);
  char val[6];
  convertSGV(sgv, val, sizeof(val));
  Serial.print("mmol value ");
  Serial.println(val);
  if(sgv < 4.0) {
    ePaper->setTextColor(GxEPD_RED);
  }
  ePaper->setCursor(value_pos_x, value_pos_y);
  ePaper->println(val);
  ePaper->setTextColor(GxEPD_BLACK);

  drawDirection(direction);
  
  ePaper->update();
}

void convertSGV(int sgv, char* buffer, int buffersize) {
  float val = (float)sgv / 18.0;
  //val = roundf(val / 10.0) * 10.0;
  snprintf(buffer, buffersize, "%.1f", val);
}

void drawDirection(const char* direction){
  //DoubleDown, DoubleUp, SingleDown, SingleUp, FortyFiveDown, FortyFiveUp, Flat
  if(strcmp(direction, "Flat") == 0) {
    const uint16_t color = GxEPD_BLACK;
    drawLineY5(arrow_pos_x + 00, arrow_pos_y + 20, arrow_pos_x + 60, arrow_pos_y + 20, color);
    drawLineX7(arrow_pos_x + 60, arrow_pos_y + 20, arrow_pos_x + 40, arrow_pos_y + 00, color);
    drawLineX7(arrow_pos_x + 60, arrow_pos_y + 20, arrow_pos_x + 40, arrow_pos_y + 40, color);
  } else if (strcmp(direction, "FortyFiveUp") == 0) {
    const uint16_t color = GxEPD_BLACK;
    drawLineX5(arrow_pos_x + 20, arrow_pos_y + 40, arrow_pos_x + 60, arrow_pos_y + 00, color);
    drawLineX5(arrow_pos_x + 60, arrow_pos_y + 00, arrow_pos_x + 60, arrow_pos_y + 30, color);
    drawLineY5(arrow_pos_x + 60, arrow_pos_y + 00, arrow_pos_x + 30, arrow_pos_y + 00, color);
  } else if (strcmp(direction, "FortyFiveDown") == 0) {
    const uint16_t color = GxEPD_BLACK;
    drawLineX5(arrow_pos_x + 20, arrow_pos_y + 00, arrow_pos_x + 60, arrow_pos_y + 40, color);
    drawLineX5(arrow_pos_x + 60, arrow_pos_y + 40, arrow_pos_x + 60, arrow_pos_y + 10, color);
    drawLineY5(arrow_pos_x + 60, arrow_pos_y + 40, arrow_pos_x + 30, arrow_pos_y + 40, color);
  } else if (strcmp(direction, "SingleUp") == 0) {
    const uint16_t color = GxEPD_RED;
    drawLineX5(arrow_pos_x + 30, arrow_pos_y + 60, arrow_pos_x + 30, arrow_pos_y + 00, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 00, arrow_pos_x + 10, arrow_pos_y + 20, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 00, arrow_pos_x + 50, arrow_pos_y + 20, color);
  } else if (strcmp(direction, "SingleDown") == 0) {
    const uint16_t color = GxEPD_RED;
    drawLineX5(arrow_pos_x + 30, arrow_pos_y + 00, arrow_pos_x + 30, arrow_pos_y + 60, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 60, arrow_pos_x + 10, arrow_pos_y + 40, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 60, arrow_pos_x + 50, arrow_pos_y + 40, color);
  } else if (strcmp(direction, "DoubleUp") == 0) {
    const uint16_t color = GxEPD_RED;
    drawLineX5(arrow_pos_x + 30, arrow_pos_y + 60, arrow_pos_x + 30, arrow_pos_y + 00, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 00, arrow_pos_x + 10, arrow_pos_y + 20, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 00, arrow_pos_x + 50, arrow_pos_y + 20, color);
    
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 20, arrow_pos_x + 10, arrow_pos_y + 40, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 20, arrow_pos_x + 50, arrow_pos_y + 40, color);
  } else if (strcmp(direction, "DoubleDown") == 0) {
    const uint16_t color = GxEPD_RED;
    drawLineX5(arrow_pos_x + 30, arrow_pos_y + 00, arrow_pos_x + 30, arrow_pos_y + 60, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 60, arrow_pos_x + 10, arrow_pos_y + 40, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 60, arrow_pos_x + 50, arrow_pos_y + 40, color);

    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 40, arrow_pos_x + 10, arrow_pos_y + 20, color);
    drawLineX7(arrow_pos_x + 30, arrow_pos_y + 40, arrow_pos_x + 50, arrow_pos_y + 20, color);
  }
}

void drawLineX5(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  ePaper->drawLine(x0 - 2, y0 - 0, x1 - 2, y1 - 0, color);
  ePaper->drawLine(x0 - 1, y0 - 0, x1 - 1, y1 - 0, color);
  ePaper->drawLine(x0 + 0, y0 + 0, x1 + 0, y1 + 0, color);
  ePaper->drawLine(x0 + 1, y0 + 0, x1 + 1, y1 + 0, color);
  ePaper->drawLine(x0 + 2, y0 + 0, x1 + 2, y1 + 0, color);
}

void drawLineX7(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  ePaper->drawLine(x0 - 3, y0 - 0, x1 - 3, y1 - 0, color);
  ePaper->drawLine(x0 - 2, y0 - 0, x1 - 2, y1 - 0, color);
  ePaper->drawLine(x0 - 1, y0 - 0, x1 - 1, y1 - 0, color);
  ePaper->drawLine(x0 + 0, y0 + 0, x1 + 0, y1 + 0, color);
  ePaper->drawLine(x0 + 1, y0 + 0, x1 + 1, y1 + 0, color);
  ePaper->drawLine(x0 + 2, y0 + 0, x1 + 2, y1 + 0, color);
  ePaper->drawLine(x0 + 3, y0 + 0, x1 + 3, y1 + 0, color);
}

void drawLineY5(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  ePaper->drawLine(x0 - 0, y0 - 2, x1 - 0, y1 - 2, color);
  ePaper->drawLine(x0 - 0, y0 - 1, x1 - 0, y1 - 1, color);
  ePaper->drawLine(x0 + 0, y0 + 0, x1 + 0, y1 + 0, color);
  ePaper->drawLine(x0 + 0, y0 + 1, x1 + 0, y1 + 1, color);
  ePaper->drawLine(x0 + 0, y0 + 2, x1 + 0, y1 + 2, color);
}

void drawLineY7(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  ePaper->drawLine(x0 - 0, y0 - 3, x1 - 0, y1 - 3, color);
  ePaper->drawLine(x0 - 0, y0 - 2, x1 - 0, y1 - 2, color);
  ePaper->drawLine(x0 - 0, y0 - 1, x1 - 0, y1 - 1, color);
  ePaper->drawLine(x0 + 0, y0 + 0, x1 + 0, y1 + 0, color);
  ePaper->drawLine(x0 + 0, y0 + 1, x1 + 0, y1 + 1, color);
  ePaper->drawLine(x0 + 0, y0 + 2, x1 + 0, y1 + 2, color);
  ePaper->drawLine(x0 + 0, y0 + 3, x1 + 0, y1 + 3, color);
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup()
{
    Serial.begin(115200);

    delay(500);
    
    Serial.println();
    Serial.println("setup");
    
    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    // Get watch object
    twatch = TTGOClass::getWatch();

    twatch->begin();

    rtc = twatch->rtc;

    power = twatch->power;

    btn = twatch->button;

    ePaper = twatch->ePaper;

    // Use compile time as RTC input time
    rtc->check();

    // Turn on power management button interrupt
    power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);

    // Clear power interruption
    power->clearIRQ();

    // Set MPU6050 to sleep
    twatch->mpu->setSleepEnabled(true);

    // Set Pin to interrupt
    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        pwIRQ = true;
    }, FALLING);


    btn->setPressedHandler([]() {
        // esp_restart();
        touch_vaild = !touch_vaild;

        if (touch_vaild) {
            ePaper->fillScreen( GxEPD_WHITE);
            //ePaper->update();
        } else {
            ePaper->fillScreen( GxEPD_WHITE);
            //mainPage(true);
        }
    });

    btn->setDoubleClickHandler([]() {
        esp_restart();
    });
  
    int retry = 3;
    while (--retry > 0) {
      delay(2000);
      if(!wifiConnect()) {
        continue;
      }
      if(!setClock()) {
        continue;
      }
      if(!getValue()) {
        continue;
      }
    }

    // Initialize the ink screen
    setupDisplay();

    // Initialize the interface
    mainPage(true);

    // Reduce CPU frequency
    setCpuFrequencyMhz(40);

    // Goto Sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.flush();
    delay(250);
    esp_deep_sleep_start();
    // Next line is never executed
    Serial.println("setup done");
}


void loop()
{
    btn->loop();

    if (pwIRQ) {
        pwIRQ = false;

        // Get interrupt status
        power->readIRQ();

        if (power->isPEKShortPressIRQ()) {
            //do something
        }
        // After the interruption, you need to manually clear the interruption status
        power->clearIRQ();
    }

    while (touch_vaild) {
        btn->loop();
        if (twatch->getTouch(x, y)) {
            Serial.printf("X:%d Y:%d\n", x, y);
            ePaper->fillCircle(x, y, 5, GxEPD_BLACK);
            ePaper->update();
        }
    }

    if (millis() - loopMillis > 1000) {
        loopMillis = millis();
        // Partial refresh
        mainPage(false);
    }
}

const unsigned char logoIcon[280] =  {
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X80,
    0X00, 0X00, 0X00, 0X00, 0XF8, 0X00, 0X00, 0X00, 0X03, 0XC0, 0X00, 0X00, 0X00, 0X03, 0XFF, 0X00,
    0X00, 0X00, 0X07, 0XE0, 0X00, 0X00, 0X00, 0X0F, 0XFF, 0X80, 0X00, 0X00, 0X07, 0XE0, 0X02, 0X30,
    0X00, 0X0F, 0XC3, 0X80, 0X00, 0X00, 0X07, 0XE0, 0X07, 0X78, 0X00, 0X1F, 0X81, 0X80, 0X00, 0X00,
    0X07, 0XE0, 0X0F, 0XF8, 0X00, 0X3F, 0X80, 0X00, 0X00, 0X00, 0X07, 0XE0, 0X07, 0X78, 0X00, 0X3F,
    0X80, 0X00, 0X00, 0X00, 0X07, 0XE0, 0X02, 0X7B, 0XC3, 0XBF, 0X0F, 0X87, 0XF0, 0X00, 0X07, 0XE0,
    0X07, 0X7B, 0XC7, 0XFF, 0X1F, 0XCF, 0XF8, 0X00, 0X07, 0XE0, 0X0F, 0X7B, 0XC7, 0XFF, 0X1F, 0XDE,
    0X3C, 0X00, 0X07, 0XE0, 0X0F, 0XFB, 0XC7, 0XBF, 0X07, 0XDE, 0X3E, 0X00, 0X07, 0XE0, 0X7F, 0X7B,
    0XC7, 0XBF, 0X83, 0XFE, 0X3E, 0X00, 0X07, 0XE0, 0X7F, 0XFB, 0XC7, 0XBF, 0X83, 0XFE, 0X3E, 0X00,
    0X07, 0XE0, 0X7F, 0XFB, 0XC7, 0X9F, 0X83, 0XDE, 0X3E, 0X00, 0X07, 0XE0, 0XEF, 0X7B, 0XC7, 0X9F,
    0XC3, 0XDE, 0X3C, 0X00, 0X07, 0XFF, 0XEF, 0XFB, 0XEF, 0X8F, 0XFF, 0XDF, 0X7C, 0X00, 0X03, 0XFF,
    0XC7, 0X79, 0XFF, 0X83, 0XFF, 0XCF, 0XF8, 0X00, 0X01, 0XFF, 0X86, 0X30, 0XE7, 0X80, 0X79, 0X81,
    0XE0, 0X00, 0X00, 0X00, 0X00, 0X00, 0X07, 0X80, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03,
    0X07, 0X80, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03, 0XDF, 0X80, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X03, 0XFF, 0X80, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0XFF, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
};
