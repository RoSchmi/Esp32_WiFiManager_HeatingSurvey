#include <Arduino.h>
#include <time.h>
#include "ESPAsyncWebServer.h"
#include "defines.h"
#include "config.h"
//#include "config_secret.h"
#include "DateTime.h"
//#include "FreeRTOS.h"
#include <freertos/FreeRTOS.h>
#include "Esp.h"
#include "esp_task_wdt.h"
#include <rom/rtc.h>

#include "NTPClient_Generic.h"
#include "Timezone_Generic.h"
#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "WiFiUdp.h"
#include "WiFiClient.h"
#include "HTTPClient.h"

#include "SoundSwitcher.h"

// Default Esp32 stack size of 8192 byte is not enough for this application.
// --> configure stack size dynamically from code to 16384
// https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/4
// Patch: Replace C:\Users\thisUser\.platformio\packages\framework-arduinoespressif32\cores\esp32\main.cpp
// with the file 'main.cpp' from folder 'patches' of this repository, then use the following code to configure stack size
#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
  uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
#endif


#define LED_BUILTIN 2

#define LED_ON            HIGH
  #define LED_OFF           LOW
// Used to keep book of used stack
void * StackPtrAtStart;
void * StackPtrEnd;

UBaseType_t watermarkStart;
TaskHandle_t taskHandle_0 =  xTaskGetCurrentTaskHandleForCPU(0);
TaskHandle_t taskHandle_1 =  xTaskGetCurrentTaskHandleForCPU(1);

bool watchDogEnabled = false;
bool watchDogCommandSuccessful = false;

RESET_REASON resetReason_0;
RESET_REASON resetReason_1;

uint8_t lastResetCause = 0;

MicType usedMicType = (USED_MICROPHONE == 0) ? MicType::SPH0645LM4H : MicType::INMP441;

// function forward declarations
void print_reset_reason(RESET_REASON reason);

// Here: Begin of WiFi Manager definitions
//***************************************************************


#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#define ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET     "ESPAsync_WiFiManager v1.9.2"

// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _ESPASYNC_WIFIMGR_LOGLEVEL_    0

//For ESP32, To use ESP32 Dev Module, QIO, Flash 4MB/80MHz, Upload 921600

//Ported to ESP32
#ifdef ESP32
  #include <esp_wifi.h>
  #include <WiFi.h>
  #include <WiFiClient.h>

  // From v1.1.1
  #include <WiFiMulti.h>
  WiFiMulti wifiMulti;

  // LittleFS has higher priority than SPIFFS
  #if ( ARDUINO_ESP32C3_DEV )
    // Currently, ESP32-C3 only supporting SPIFFS and EEPROM. Will fix to support LittleFS
    #define USE_LITTLEFS          false
    #define USE_SPIFFS            true
  #else
    #define USE_LITTLEFS    true
    #define USE_SPIFFS      false
  #endif

  #if USE_LITTLEFS
    // Use LittleFS
    #include "FS.h"

    // The library has been merged into esp32 core release 1.0.6
    #include <LITTLEFS.h>             // https://github.com/lorol/LITTLEFS
    
    FS* filesystem =      &LITTLEFS;
    #define FileFS        LITTLEFS
    #define FS_Name       "LittleFS"
  #elif USE_SPIFFS
    #include <SPIFFS.h>
    FS* filesystem =      &SPIFFS;
    #define FileFS        SPIFFS
    #define FS_Name       "SPIFFS"
  #else
    // Use FFat
    #include <FFat.h>
    FS* filesystem =      &FFat;
    #define FileFS        FFat
    #define FS_Name       "FFat"
  #endif
  //////

   #define ESP_getChipId()   (ESP.getChipId())
  
  #define LED_ON      LOW
  #define LED_OFF     HIGH
#endif

// SSID and PW for Config Portal
//RoSchmi
String ssid = "ESP_" + String(ESP_getChipId(), HEX);

String password;

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

// From v1.1.1
// You only need to format the filesystem once
//#define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM         false

#define MIN_AP_PASSWORD_SIZE    8

#define SSID_MAX_LEN            32
//From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN            64

typedef struct
{
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw  [PASS_MAX_LEN];
}  WiFi_Credentials;

typedef struct
{
  String wifi_ssid;
  String wifi_pw;
}  WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS      2

// Assuming max 49 chars
#define TZNAME_MAX_LEN            50
#define TIMEZONE_MAX_LEN          50

typedef struct
{
  WiFi_Credentials  WiFi_Creds [NUM_WIFI_CREDENTIALS];
  char TZ_Name[TZNAME_MAX_LEN];     // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];        // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} WM_Config;

WM_Config         WM_config;

#define  CONFIG_FILENAME              F("/wifi_cred.dat")
//////

// Indicates whether ESP has WiFi credentials saved from previous session
// Is set to true if WiFi-Credentials could be read from SPIFFS/LittleFS
bool initialConfig = false;

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h>
#define USE_AVAILABLE_PAGES     true

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP     false

// Just use enough to save memory. On ESP8266, can cause blank ConfigPortal screen
// if using too much memory
#define USING_AFRICA        false
#define USING_AMERICA       false
#define USING_ANTARCTICA    false
#define USING_ASIA          false
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     false
#define USING_EUROPE        true
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP          false

// New in v1.0.11
#define USING_CORS_FEATURE          true
//////

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
// Force DHCP to be true
  #if defined(USE_DHCP_IP)
    #undef USE_DHCP_IP
  #endif
  #define USE_DHCP_IP     true
#else
  // You can select DHCP or Static IP here
  
  #define USE_DHCP_IP     false
#endif

#if ( USE_DHCP_IP )
  // Use DHCP
  #warning Using DHCP IP
  IPAddress stationIP   = IPAddress(0, 0, 0, 0);
  IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);
#else
  // Use static IP
  #warning Using static IP
  
  #ifdef ESP32
    IPAddress stationIP   = IPAddress(192, 168, 2, 232);
  #else
    IPAddress stationIP   = IPAddress(192, 168, 2, 186);
  #endif
  
  IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);
#endif

#define USE_CONFIGURABLE_DNS      true

IPAddress dns1IP      = gatewayIP;
IPAddress dns2IP      = IPAddress(8, 8, 8, 8);

#define USE_CUSTOM_AP_IP          false

IPAddress APStaticIP  = IPAddress(192, 168, 100, 1);
IPAddress APStaticGW  = IPAddress(192, 168, 100, 1);
IPAddress APStaticSN  = IPAddress(255, 255, 255, 0);

#include <ESPAsync_WiFiManager.h>              //https://github.com/khoih-prog/ESPAsync_WiFiManager

#define HTTP_PORT     80

  #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
void setup()
{
  // put your setup code here, to run once:
  // Get Stackptr at start of setup()

void* SpStart = NULL;
  StackPtrAtStart = (void *)&SpStart;
  // Get StackHighWatermark at start of setup()
  watermarkStart =  uxTaskGetStackHighWaterMark(NULL);
  // Calculate (not exact) end-address of the stack
  StackPtrEnd = StackPtrAtStart - watermarkStart;

  __unused uint32_t minFreeHeap = esp_get_minimum_free_heap_size();
  uint32_t freeHeapSize = esp_get_free_heap_size();
  


  
  resetReason_0 = rtc_get_reset_reason(0);
  resetReason_1 = rtc_get_reset_reason(1);

  lastResetCause = resetReason_1;

  // Disable watchdog
  // volatile int wdt_State = esp_task_wdt_status(NULL);
  // volatile int wdt_deinit_result = esp_task_wdt_deinit();
  // ESP_ERR_INVALID_STATE  -- 259
  // ESP_ERR_NOT_FOUND -- 261

    if ((esp_task_wdt_status(NULL) == ESP_ERR_NOT_FOUND) || (esp_task_wdt_deinit() == ESP_OK))
    {
      watchDogEnabled = false;
      watchDogCommandSuccessful = true;    
    }
    else
    {
      watchDogCommandSuccessful = false;   
    }
   
// initialize the LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);


  Serial.begin(115200);
  while (!Serial);

  delay(4000);

  Serial.printf("\r\n\r\nAddress of Stackpointer near start is:  %p \r\n",  (void *)StackPtrAtStart);
  Serial.printf("End of Stack is near:                   %p \r\n",  (void *)StackPtrEnd);
  Serial.printf("Free Stack near start is:  %d \r\n",  (uint32_t)StackPtrAtStart - (uint32_t)StackPtrEnd);
  
  Serial.print(F("Free Heap: "));
  Serial.println(freeHeapSize);
  Serial.printf("Last Reset Reason: CPU_0 = %u, CPU_1 = %u\r\n", resetReason_0, resetReason_1);
  Serial.println("Reason CPU_0: ");
  print_reset_reason(resetReason_0);
  Serial.println("Reason CPU_1: ");
  print_reset_reason(resetReason_1);

  if(watchDogCommandSuccessful)
  {
    Serial.println(F("\r\nWatchdog is preliminary disabled"));
  }
  else
  {
    Serial.println(F("\r\nFailed to disable Watchdog"));
    Serial.println(F("\r\nRestarting"));
    // Wait some time (3000 ms)
    uint32_t start = millis();
    while ((millis() - start) < 3000)
    {
      delay(10);
    }
    ESP.restart();
  }
  Serial.printf("Selected Microphone Type = %s\r\n", (usedMicType == MicType::SPH0645LM4H) ? "SPH0645" : "INMP441");

  delay(4000);

  // If wanted -> Wait on press/release of boot button
  /*
  Serial.println(F("\r\n\r\nPress Boot Button to continue!"));
  while (!buttonPressed)
  {
    delay(100);
  }
  */
  // Wait some time (3000 ms)
  uint32_t start = millis();
  while ((millis() - start) < 3000)
  {
    delay(10);
  }
  
  Serial.print(F("\nStarting Async_ConfigOnStartup using ")); Serial.print(FS_Name);
  Serial.print(F(" on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION);

  if ( String(ESP_ASYNC_WIFIMANAGER_VERSION) < ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET )
  {
    Serial.print("Warning. Must use this example on Version later than : ");
    Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET);
  }



while (true)
  {
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(1000);
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(1000);
  }

  // Disable watchdog
  // volatile int wdt_State = esp_task_wdt_status(NULL);
  // volatile int wdt_deinit_result = esp_task_wdt_deinit();
  // ESP_ERR_INVALID_STATE  -- 259
  // ESP_ERR_NOT_FOUND -- 261

/*
    if ((esp_task_wdt_status(NULL) == ESP_ERR_NOT_FOUND) || (esp_task_wdt_deinit() == ESP_OK))
    {
      watchDogEnabled = false;
      watchDogCommandSuccessful = true;    
    }
    else
    {
      watchDogCommandSuccessful = false;   
    }
  */



  
 
}

void loop()
{
  while (true)
  {
    delay(1000);
  }
}


void print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET");break;          /**<1, Vbat power on reset*/
    case 3 : Serial.println ("SW_RESET");break;               /**<3, Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET");break;             /**<4, Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET");break;        /**<5, Deep Sleep reset digital core*/
    case 6 : Serial.println ("SDIO_RESET");break;             /**<6, Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET");break;       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET");break;       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET");break;       /**<9, RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : Serial.println ("NO_MEAN");
  }
}
