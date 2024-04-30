
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERNAL

#include <Arduino.h>
#include <FastLED.h>
#include <SSD1306.h>
//#include <SSD1306Wire.h>
#include <OneButton.h> 
#include <EEPROM.h>
#include <Ticker.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <DFRobot_DF1201S.h>
#include <SoftwareSerial.h>
#include <ArduinoOTA.h>
#include "version.h" 
#include "wait.h"
#include <GeoIP.h> 

#define DATA_PIN 0                      // NodeMCU D3    FastLED is set to use raw ESP/Arduino pin numbers  
#define BUILT_IN_LED 2                  // NodeMCU D4    1Hz indicator - blue built-in LED
#define BUTTON_PIN 12                   // NodeMCU D6
#define DISPLAY_PIN 14                  // NodeMCU D5
#define SOFTWARE_SERIAL_RX 13           // NodeMCU D7
#define SOFTWARE_SERIAL_TX 15           // NodeMCU D8
#define SDA_PIN 4                       // NodeMCU D2
#define SCL_PIN 5                       // NodeMCU D1
#define DEFAULT_UTC_OFFSET -28800       // 8 hours in seconds, Pacific Standard Time
#define CHIME_START_HOUR 10
#define CHIME_END_HOUR 1
#define CHIME_VOLUME 1                  // values are 0 to 20
#define BREATHE_LEVEL_CHANGE 5          // Level change for each step in the breathing LEDs sequence, 5 to 10 seems to work
#define BREATHE_LOW_VALUE 30
#define BREATHE_HIGH_VALUE 150
#define NTP_SERVER "ca.pool.ntp.org"

//#define DEBUG     // Comment this out to remove debug code from release version

#ifdef DEBUG
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINT(x) Serial.print(x)
#else
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT(x)
#endif

//  #define NTP_SERVER "us.pool.ntp.org"
//  #define NTP_SERVER "time.nist.gov"
//  #define NTP_SERVER "time-a.timefreq.bldrdoc.gov"
//  #define NTP_SERVER "time-b.timefreq.bldrdoc.gov"
//  #define NTP_SERVER "time-c.timefreq.bldrdoc.gov" 

// HSV hues
#define YELLOW 60                   
#define GREEN 90  
#define CYAN 130
#define BLUE 160                      
#define MAGENTA 184
#define RED 254
#define ORANGE 15
#define DEFAULT_HUE 16              // Nixie orange

// Global variables
const char     ntpServerName[] = NTP_SERVER;
uint8_t        hue = DEFAULT_HUE;
CRGB           ledArray[5];                        // LED data 
time_t         local;
time_t         utc;
location_t     loc;
uint8_t        chimeFileSet;                       // 1 for first set, 2 for second set, 3 for the third set, etc.
uint8_t        fileSetsOnPlayer;             
uint16_t       breatheStepTime;
WiFiUDP Udp;   
unsigned int localPort = 8888;                     // local port to listen for UDP packets

// Objects
GeoIP geoip; 
OneButton functionButton(BUTTON_PIN, true, true);             // active low with pullup  
Ticker breathe;         
TimeChangeRule *tcr;
TimeChangeRule dstRule = {"DST", Second, Sun, Mar, 2, 60};    // only used to know when it's DST, 
TimeChangeRule stdRule = {"STD", First, Sun, Nov, 2, 0};      //    offset gets set based on ipapi.co or default or manual offset
Timezone tz(dstRule, stdRule);
SoftwareSerial DF1201SSerial(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX);  
DFRobot_DF1201S DF1201S; 
SSD1306 display(0x3C, SDA_PIN, SCL_PIN);


// Function definitions
void configModeCallback (WiFiManager *myWiFiManager);
void displayTime();
void serialMonitor();
bool setTimeZone();
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
bool timeInterval(uint8_t start, uint8_t end);
void chimes(uint8_t fileSet);
void fourLeds(uint8_t HSVhue);
void fourLeds(uint8_t r, uint8_t g, uint8_t b);
void confirmation(uint8_t r, uint8_t g, uint8_t b);
void playerFunctions();
void buttonClick();
void buttonDoubleClick();
void buttonLongPressStop();
void buttonLongPressStart();
void sequenceBreathing();
void header();
