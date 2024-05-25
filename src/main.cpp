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
