// Program 'Esp32_WiFiManager_HeatingSurvey' Branch Master

// Last updated: 2024_07_10
// Copyright: RoSchmi 2021, 2024 License: Apache 2.0

// Now uses
// platform espressif32@6.7.0 and platform_package framework-arduinoespressif32 @ 3.20017.0
// In cases of a wrong (old) format of the LittleFS flash storage,
// the flash must be cleared with the command (cmd window)
// python C:\Users\<user>\.platformio\packages\tool-esptoolpy\esptool.py erase_flash

// Definition of your TimeZone has to be made in config.h

// The application doesn't compile without a trick:
// The libraries NTPClient_Generic and Timezone_Generic load the
// dependency:
// {
//      "owner": "khoih-prog",
//      "name": "ESP8266_AT_WebServer",
//      "version": ">=1.5.4",
//      "platforms": ["*"]
//    },
// This dependency doesn't compile. So we delete in folder libdeps/ESP32 
// the dependency in the 'library.json' file and delete the entry 
// 'ESP8266_AT_WebServer' in the folder libdeps/ESP32


// This App for Esp32 monitors the activity of the burner of an oil-heating
// (or any other noisy thing) by measuring the sound of e.g. the heating burner 
// using an I2S microphone (Adafruit SPH0645 or INMP441)
// The on/off states are transferred to the Cloud (Azure Storage Tables)
// via WLAN and internet and can be visulized graphically through the iPhone- or Android-App
// Charts4Azure.
// The WiFi Credentials can be entered via a Portal page which is provided for one minute
// by the Esp32 after powering up the device. After having entered the credentials
// one time they stay permanently on the Esp32 and need not to be entered every time.

// This App uses an adaption of the 'Async_ConfigOnStartup.ino' as WiFi Mangager
// from: -https://github.com/khoih-prog/ESPAsync_WiFiManager 
// More infos about the functions and Licenses see:
// -https://github.com/khoih-prog/ESPAsync_WiFiManager/tree/master/examples/Async_ConfigOnStartup
 

// The WiFiManager will open a configuration portal for 60 seconds when first powered up if the boards has stored WiFi Credentials.
// Otherwise, it'll stay indefinitely in ConfigPortal until getting WiFi Credentials and connecting to WiFi
 
#include <Arduino.h>
#include <time.h>
#include "ESPAsyncWebServer.h"
#include "defines.h"
#include "config.h"
#include "DateTime.h"
#include <freertos/FreeRTOS.h>
#include "Esp.h"
#include "esp_task_wdt.h"
#include <rom/rtc.h>

#include "CloudStorageAccount.h"
#include "TableClient.h"
#include "TableEntityProperty.h"
#include "TableEntity.h"
#include "AnalogTableEntity.h"
#include "OnOffTableEntity.h"

#include "NTPClient_Generic.h"
#include "Timezone_Generic.h"
#include "WiFiUdp.h"
#include "WiFiClient.h"
#include "HTTPClient.h"

#include "DataContainerWio.h"
#include "OnOffDataContainerWio.h"
#include "OnOffSwitcherWio.h"
#include "SoundSwitcher.h"
#include "ImuManagerWio.h"
#include "AnalogSensorMgr.h"

#include "azure/core/az_platform.h"
#include "azure/core/az_http.h"
#include "azure/core/az_http_transport.h"
#include "azure/core/az_result.h"
#include "azure/core/az_config.h"
#include "azure/core/az_context.h"
#include "azure/core/az_span.h"

#include "Rs_TimeNameHelper.h"

// Now support ArduinoJson 6.0.0+ ( tested with v6.14.1 )
#include "ArduinoJson.h"      // get it from https://arduinojson.org/ or install via Arduino library manager

// Default Esp32 stack size of 8192 byte is not enough for this application.
// --> configure stack size dynamically from code to 16384
// https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/4

SET_LOOP_TASK_STACK_SIZE ( 16*1024 ); // 16KB

// Allocate memory space in memory segment .dram0.bss, ptr to this memory space is later
// passed to TableClient (is used there as the place for some buffers to preserve stack )
uint8_t bufferStore[4000] {0};
uint8_t * bufferStorePtr = &bufferStore[0];


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

#define GPIOPin 0
bool buttonPressed = false;

const char analogTableName[45] = ANALOG_TABLENAME;

const char OnOffTableName_1[45] = ON_OFF_TABLENAME_01;
const char OnOffTableName_2[45] = ON_OFF_TABLENAME_02;
const char OnOffTableName_3[45] = ON_OFF_TABLENAME_03;
const char OnOffTableName_4[45] = ON_OFF_TABLENAME_04;

// The PartitionKey for the analog table may have a prefix to be distinguished, here: "Y2_" 
const char * analogTablePartPrefix = (char *)ANALOG_TABLE_PART_PREFIX;

// The PartitionKey for the On/Off-tables may have a prefix to be distinguished, here: "Y3_" 
const char * onOffTablePartPrefix = (char *)ON_OFF_TABLE_PART_PREFIX;

// The PartitinKey can be augmented with a string representing year and month (recommended)
const bool augmentPartitionKey = true;

// The TableName can be augmented with the actual year (recommended)
const bool augmentTableNameWithYear = true;

#define LED_BUILTIN 2

//RoSchmi
//const char *ssid = IOT_CONFIG_WIFI_SSID;
//const char *password = IOT_CONFIG_WIFI_PASSWORD;

typedef const char* X509Certificate;

// https://techcommunity.microsoft.com/t5/azure-storage/azure-storage-tls-critical-changes-are-almost-here-and-why-you/ba-p/2741581
// baltimore_root_ca will expire in 2025, then take digicert_globalroot_g2_ca
//X509Certificate myX509Certificate = baltimore_root_ca;

X509Certificate myX509Certificate = digicert_globalroot_g2_ca;

// Init the Secure client object

#if TRANSPORT_PROTOCOL == 1
    static WiFiClientSecure wifi_client;
  #else
    static WiFiClient wifi_client;
  #endif

  // A UDP instance to let us send and receive packets over UDP
  WiFiUDP ntpUDP;
  static NTPClient timeClient(ntpUDP);
  
  HTTPClient http;
  
  // Ptr to HTTPClient
  static HTTPClient * httpPtr = &http;
  
// Define Datacontainer with SendInterval and InvalidateInterval as defined in config.h
int sendIntervalSeconds = (SENDINTERVAL_MINUTES * 60) < 1 ? 1 : (SENDINTERVAL_MINUTES * 60);

DataContainerWio dataContainer(TimeSpan(sendIntervalSeconds), TimeSpan(0, 0, INVALIDATEINTERVAL_MINUTES % 60, 0), (float)MIN_DATAVALUE, (float)MAX_DATAVALUE, (float)MAGIC_NUMBER_INVALID);

AnalogSensorMgr analogSensorMgr(MAGIC_NUMBER_INVALID);

OnOffDataContainerWio onOffDataContainer;

OnOffSwitcherWio onOffSwitcherWio;

// Possible configuration for Adafruit Huzzah Esp32
static const i2s_pin_config_t pin_config_Adafruit_Huzzah_Esp32 = {
    .bck_io_num = 14,                   // BCKL
    .ws_io_num = 15,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 32                   // DOUT
};
// Possible configuration for some Esp32 DevKitC V4
static const i2s_pin_config_t pin_config_Esp32_dev = {
    .bck_io_num = 26,                   // BCKL
    .ws_io_num = 25,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 22                   // DOUT
};

MicType usedMicType = (USED_MICROPHONE == 0) ? MicType::SPH0645LM4H : MicType::INMP441;

SoundSwitcher soundSwitcher(pin_config_Esp32_dev, usedMicType);
//SoundSwitcher soundSwitcher(pin_config_Adafruit_Huzzah_Esp32, usedMicType);

FeedResponse feedResult;

//int soundSwitcherThreshold = SOUNDSWITCHER_THRESHOLD;
int soundSwitcherUpdateInterval = SOUNDSWITCHER_UPDATEINTERVAL;
uint32_t soundSwitcherReadDelayTime = SOUNDSWITCHER_READ_DELAYTIME;

uint64_t loopCounter = 0;
int insertCounterAnalogTable = 0;
uint32_t tryUploadCounter = 0;
uint32_t failedUploadCounter = 0;
uint32_t timeNtpUpdateCounter = 0;
// not used on Esp32
int32_t sysTimeNtpDelta = 0;

  bool ledState = false;
  
  const int timeZoneOffset = (int)TIMEZONEOFFSET;
  const int dstOffset = (int)DSTOFFSET;

  Rs_TimeNameHelper timeNameHelper;

  DateTime dateTimeUTCNow;    // Seconds since 2000-01-01 08:00:00

  Timezone myTimezone;

// Set transport protocol as defined in config.h
static bool UseHttps_State = TRANSPORT_PROTOCOL == 0 ? false : true;

// RoSchmi
const char * CONFIG_FILE = "/ConfigSW.json";

#define AZURE_CONFIG_ACCOUNT_NAME "AzureStorageAccName"

#define AZURE_CONFIG_ACCOUNT_KEY   "YourStorageAccountKey"

// Parameter for WiFi-Manager
char azureAccountName[20] =  AZURE_CONFIG_ACCOUNT_NAME;
char azureAccountKey[90] =  AZURE_CONFIG_ACCOUNT_KEY;
char sSwiThresholdStr[6] = SOUNDSWITCHER_THRESHOLD;

#define AzureAccountName_Label "azureAccountName"
#define AzureAccountKey_Label "azureAccountKey"
#define SoundSwitcherThresholdString_Label "sSwiThresholdStr"

// Function Prototypes for WiFiManager
bool readConfigFile();
bool writeConfigFile();

// Note: AzureAccountName and azureccountKey will be changed later in setup
CloudStorageAccount myCloudStorageAccount(azureAccountName, azureAccountKey, UseHttps_State);
CloudStorageAccount * myCloudStorageAccountPtr = &myCloudStorageAccount;

void GPIOPinISR()
{
  buttonPressed = true;
}





// function forward declarations
void print_reset_reason(RESET_REASON reason);
void scan_WIFI();
boolean connect_Wifi(const char * ssid, const char * password);
String floToStr(float value);
float ReadAnalogSensor(int pSensorIndex);
void createSampleTime(DateTime dateTimeUTCNow, int timeZoneOffsetUTC, char * sampleTime);
az_http_status_code  createTable(CloudStorageAccount * myCloudStorageAccountPtr, X509Certificate pCaCert, const char * tableName);
az_http_status_code insertTableEntity(CloudStorageAccount *myCloudStorageAccountPtr,X509Certificate pCaCert, const char * pTableName, TableEntity pTableEntity, char * outInsertETag);
void makePartitionKey(const char * partitionKeyprefix, bool augmentWithYear, DateTime dateTime, az_span outSpan, size_t *outSpanLength);
void makeRowKey(DateTime actDate, az_span outSpan, size_t *outSpanLength);
int getDayNum(const char * day);
int getMonNum(const char * month);
int getWeekOfMonthNum(const char * weekOfMonth);
uint8_t connectMultiWiFi();
//bool readConfigFile();
//bool writeConfigFile();


// Here: Begin of WiFi Manager definitions
//***************************************************************


#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#define ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET     "ESPAsync_WiFiManager v1.10.1"

// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _ESPASYNC_WIFIMGR_LOGLEVEL_    0

//For ESP32, To use ESP32 Dev Module, QIO, Flash 4MB/80MHz, Upload 921600

//Ported to ESP32
#ifdef ESP32
  #include <esp_wifi.h>
  #include <WiFi.h>
  #include <WiFiClient.h>
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
    //#include "FS.h"

    //Former: #include <LITTLEFS.h>             // https://github.com/lorol/LITTLEFS
    #include <LittleFS.h>
    
    FS* filesystem =      &LittleFS;
    #define FileFS        LittleFS
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

  #define LED_BUILTIN       2
  #define LED_ON            HIGH
  #define LED_OFF           LOW

#else
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
  //needed for library
  #include <DNSServer.h>

  // From v1.1.0
  #include <ESP8266WiFiMulti.h>
  ESP8266WiFiMulti wifiMulti;

  #define USE_LITTLEFS      true
  
  #if USE_LITTLEFS
    #include <LittleFS.h>
    FS* filesystem = &LittleFS;
    #define FileFS    LittleFS
    #define FS_Name       "LittleFS"
  #else
    FS* filesystem = &SPIFFS;
    #define FileFS    SPIFFS
    #define FS_Name       "SPIFFS"
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

// You only need to format the filesystem once
//#define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM         false

#define MIN_AP_PASSWORD_SIZE    8

#define SSID_MAX_LEN            32
//WPA2 passwords can be up to 63 characters long.
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
  //#warning Using DHCP IP
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

// Onboard LED I/O pin on NodeMCU board
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. GPIO2/ADC12 of ESP32. Controls the onboard LED.

///////////////////////////////////////////
// New in v1.4.0
/******************************************
 * // Defined in ESPAsync_WiFiManager.h
typedef struct
{
  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;

}  WiFi_AP_IPConfig;

typedef struct
{
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;
#if USE_CONFIGURABLE_DNS  
  IPAddress _sta_static_dns1;
  IPAddress _sta_static_dns2;
#endif
}  WiFi_STA_IPConfig;
******************************************/

WiFi_AP_IPConfig  WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig)
{
  in_WM_AP_IPconfig._ap_static_ip   = APStaticIP;
  in_WM_AP_IPconfig._ap_static_gw   = APStaticGW;
  in_WM_AP_IPconfig._ap_static_sn   = APStaticSN;
}

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig)
{
  in_WM_STA_IPconfig._sta_static_ip   = stationIP;
  in_WM_STA_IPconfig._sta_static_gw   = gatewayIP;
  in_WM_STA_IPconfig._sta_static_sn   = netMask;
#if USE_CONFIGURABLE_DNS  
  in_WM_STA_IPconfig._sta_static_dns1 = dns1IP;
  in_WM_STA_IPconfig._sta_static_dns2 = dns2IP;
#endif
}

void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, ", gatewayIP =", in_WM_STA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
#if USE_CONFIGURABLE_DNS
  LOGERROR3(F("dns1IP ="), in_WM_STA_IPconfig._sta_static_dns1, ", dns2IP =", in_WM_STA_IPconfig._sta_static_dns2);
#endif
}

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  #if USE_CONFIGURABLE_DNS  
    // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn, in_WM_STA_IPconfig._sta_static_dns1, in_WM_STA_IPconfig._sta_static_dns2);  
  #else
    // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn);
  #endif 
}

///////////////////////////////////////////



void toggleLED()
{
  //toggle state
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

#if USE_ESP_WIFIMANAGER_NTP

void printLocalTime()
{
#if ESP8266
  static time_t now;
  
  now = time(nullptr);
  
  if ( now > 1451602800 )
  {
    Serial.print("Local Date/Time: ");
    Serial.print(ctime(&now));
  }
#else
  struct tm timeinfo;

  getLocalTime( &timeinfo );

  // Valid only if year > 2000. 
  // You can get from timeinfo : tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec
  if (timeinfo.tm_year > 100 )
  {
    Serial.print("Local Date/Time: ");
    Serial.print( asctime( &timeinfo ) );
  }
#endif
}

#endif

void heartBeatPrint()
{
#if USE_ESP_WIFIMANAGER_NTP
  printLocalTime();
#else
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
  {
    //Serial.print(F("H"));        // H means connected to WiFi
  }
  else
  {
    //Serial.print(F("F"));        // F means not connected to WiFi
  }

  if (num == 80)
  {
    //Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    //Serial.print(F(" "));
  }
#endif  
}

void check_WiFi()
{
  if ( (WiFi.status() != WL_CONNECTED) )
  {
    Serial.println(F("\nWiFi lost. Call connectMultiWiFi in loop"));
    connectMultiWiFi();
  }
}  

void check_status()
{
  static ulong checkstatus_timeout  = 0;
  static ulong LEDstatus_timeout    = 0;
  static ulong checkwifi_timeout    = 0;

  static ulong current_millis;

#define WIFICHECK_INTERVAL    1000L

#if USE_ESP_WIFIMANAGER_NTP
  #define HEARTBEAT_INTERVAL    60000L
#else
  #define HEARTBEAT_INTERVAL    10000L
#endif

#define LED_INTERVAL          2000L

  current_millis = millis();
  
  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0))
  {
    check_WiFi();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  if ((current_millis > LEDstatus_timeout) || (LEDstatus_timeout == 0))
  {
    // Toggle LED at LED_INTERVAL = 2s
    toggleLED();
    LEDstatus_timeout = current_millis + LED_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}

int calcChecksum(uint8_t* address, uint16_t sizeToCalc)
{
  uint16_t checkSum = 0;
  
  for (uint16_t index = 0; index < sizeToCalc; index++)
  {
    checkSum += * ( ( (byte*) address ) + index);
  }

  return checkSum;
}

bool loadConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  memset((void *) &WM_config,       0, sizeof(WM_config));

  // New in v1.4.0
  memset((void *) &WM_STA_IPconfig, 0, sizeof(WM_STA_IPconfig));
  //////

  if (file)
  {
    file.readBytes((char *) &WM_config,   sizeof(WM_config));

    // New in v1.4.0
    file.readBytes((char *) &WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));

    if ( WM_config.checksum != calcChecksum( (uint8_t*) &WM_config, sizeof(WM_config) - sizeof(WM_config.checksum) ) )
    {
      LOGERROR(F("WM_config checksum wrong"));
      
      return false;
    }
    
    // New in v1.4.0
    displayIPConfigStruct(WM_STA_IPconfig);
    //////

    return true;
  }
  else
  {
    LOGERROR(F("failed"));

    return false;
  }
}

void saveConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveWiFiCfgFile "));

  if (file)
  {
    WM_config.checksum = calcChecksum( (uint8_t*) &WM_config, sizeof(WM_config) - sizeof(WM_config.checksum) );   
    file.write((uint8_t*) &WM_config, sizeof(WM_config));
    displayIPConfigStruct(WM_STA_IPconfig);

    // New in v1.4.0
    file.write((uint8_t*) &WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));
  }
  else
  {
    LOGERROR(F("failed"));
  }
}

bool readConfigFile()
{
  // this opens the config file in read-mode
  File f = FileFS.open(CONFIG_FILE, "r");

  if (!f)
  {
    Serial.println(F("Configuration file not found"));
    return false;
  }
  else
  { 
    // we could open the file
    size_t size = f.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size + 1]);

    // Read and store file contents in buf
    f.readBytes(buf.get(), size);
    // Closing file
    f.close();
    // Using dynamic JSON buffer which is not the recommended memory model, but anyway
    // See https://github.com/bblanchon/ArduinoJson/wiki/Memory%20model

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
    
    DynamicJsonDocument json(1024);
    auto deserializeError = deserializeJson(json, buf.get());
    if ( deserializeError )
    {
      Serial.println(F("JSON parseObject() failed"));
      return false;
    }
    Serial.println(F("Here we could print the WiFi Credentials read from SPIFFS/LittleFS"));
    //serializeJson(json, Serial);
#else
    DynamicJsonBuffer jsonBuffer;
    // Parse JSON string
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    // Test if parsing succeeds.
    if (!json.success())
    {
      Serial.println(F("JSON parseObject() failed"));
      return false;
    }
    json.printTo(Serial);
#endif

    // Parse all config file parameters, override
    // local config variables with parsed values    
    if (json.containsKey(AzureAccountName_Label))
    {
      strcpy(azureAccountName, json[AzureAccountName_Label]);      
    }
    if (json.containsKey(AzureAccountKey_Label))
    {
      strcpy(azureAccountKey, json[AzureAccountKey_Label]);    
    }
    if (json.containsKey(SoundSwitcherThresholdString_Label))
    {
      strcpy(sSwiThresholdStr, json[SoundSwitcherThresholdString_Label]);      
    }
  }
  Serial.println(F("\nConfig file was successfully parsed")); 
  return true;
}

bool writeConfigFile()
{
  Serial.println(F("Saving config file"));

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
  DynamicJsonDocument json(1024);
#else
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
#endif

  // JSONify local configuration parameters 
  json[AzureAccountName_Label] = azureAccountName;
  json[AzureAccountKey_Label] = azureAccountKey;
  json[SoundSwitcherThresholdString_Label] = sSwiThresholdStr;
  // Open file for writing
  File f = FileFS.open(CONFIG_FILE, "w");
  
  if (!f)
  {
    Serial.println(F("Failed to open config file for writing"));
    return false;
  }

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
  Serial.println(F("Here we could print the parameter written to flash"));
  serializeJsonPretty(json, Serial);

  // Write data to file and close it
  serializeJson(json, f);
#else
  json.prettyPrintTo(Serial);
  // Write data to file and close it
  json.printTo(f);
#endif

  f.close();

  Serial.println(F("\nConfig file was successfully saved"));
  return true;
}



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
  
  /*
  UBaseType_t watermarkStart_0 = uxTaskGetStackHighWaterMark(taskHandle_0);
  UBaseType_t watermarkStart_1 = uxTaskGetStackHighWaterMark(NULL);
  */

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
  //while (!Serial);

  delay(4000);
  
  // is needed for Button
  // attachInterrupt(GPIOPin, GPIOPinISR, RISING);
  
  Serial.printf("\r\n\r\nAddress of Stackpointer near start is:  %p \r\n",  (void *)StackPtrAtStart);
  Serial.printf("End of Stack is near:                   %p \r\n",  (void *)StackPtrEnd);
  Serial.printf("Free Stack near start is:  %d \r\n",  (uint32_t)StackPtrAtStart - (uint32_t)StackPtrEnd);

  /*
  // Get free stack at actual position (can be used somewhere in program)
  void* SpActual = NULL;
  Serial.printf("\r\nFree Stack at actual position is: %d \r\n", (uint32_t)&SpActual - (uint32_t)StackPtrEnd);
  */

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
  
  // 
  Serial.setDebugOutput(false);
  
  if (FORMAT_FILESYSTEM)
  {
    #ifdef ESP32
      FileFS.begin(false, "/littlefs", 10, "spiffs");
    #endif
    FileFS.format();
  }
  else
  {
  #ifdef ESP32
    FileFS.begin(false, "/littlefs", 10, "spiffs");
    if (!FileFS.begin(true, "/littlefs", 10, "spiffs"))
    {
      Serial.println(F("SPIFFS/LittleFS failed! Already tried formatting."));
    }
  #else
    if (!FileFS.begin())
    {
      Serial.println(F("SPIFFS/LittleFS failed! Already tried formatting."));
    }
  #endif
  }     
    if (!FileFS.begin())
    {     
      // prevents debug info from the library to hide err message.
      delay(100);
      
#if USE_LITTLEFS
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));
#else
      Serial.println(F("SPIFFS failed!. Please use LittleFS or EEPROM. Stay forever"));
#endif

      while (true)
      {
        delay(1);
      }
    }
  

  unsigned long startedAt = millis();

  // New in v1.4.0
  initAPIPConfigStruct(WM_AP_IPconfig);
  initSTAIPConfigStruct(WM_STA_IPconfig);
  //////

  if (!readConfigFile())
  {
    Serial.println(F("Failed to read ConfigFile, using default values"));
  }

  //Local intialization. Once its business is done, there is no need to keep it around
  // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  //ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer);
  // Use this to personalize DHCP hostname (RFC952 conformed)
  AsyncWebServer webServer(HTTP_PORT);

#if ( USING_ESP32_S2 || USING_ESP32_C3 )
  ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, NULL, "AsyncConfigOnStartup");
#else
  // RoSchmi
  //DNSServer dnsServer;
  AsyncDNSServer dnsServer;
  
  //dnsServer.start()
  
  
  ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer, "AsyncConfigOnStartup");
#endif

#if USE_CUSTOM_AP_IP 
  //set custom ip for portal
  // New in v1.4.0
  ESPAsync_wifiManager.setAPStaticIPConfig(WM_AP_IPconfig);
  //////
#endif

  ESPAsync_wifiManager.setMinimumSignalQuality(-1);

  // From v1.0.10 only
  // Set config portal channel, default = 1. Use 0 => random channel from 1-13
  ESPAsync_wifiManager.setConfigPortalChannel(0);
  //////

#if !USE_DHCP_IP    
    // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway, Subnetmask). New in v1.0.5
    // New in v1.4.0
    ESPAsync_wifiManager.setSTAStaticIPConfig(WM_STA_IPconfig);
    //////
#endif

  // New from v1.1.1
#if USING_CORS_FEATURE
  ESPAsync_wifiManager.setCORSHeader("Your Access-Control-Allow-Origin");
#endif

  // We can't use WiFi.SSID() in ESP32 as it's only valid after connected.
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
  // Have to create a new function to store in EEPROM/SPIFFS for this purpose
  Router_SSID = ESPAsync_wifiManager.WiFi_SSID();
  Router_Pass = ESPAsync_wifiManager.WiFi_Pass();

  Serial.println(F("Here we could print the used WiFi-Credentials"));
  //Remove this line if you do not want to see WiFi password printed
  //Serial.println("ESP Self-Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);
  
  ssid.toUpperCase();
  password = "My" + ssid;

  // RoSchmi new
  // Extra parameters to be configured
  // After connecting, parameter.getValue() will get you the configured value
  // Format: <ID> <Placeholder text> <default value> <length> <custom HTML> <label placement>
  
  ESPAsync_WMParameter p_azureAccountName(AzureAccountName_Label, "Storage Account Name", azureAccountName, 20); 
  ESPAsync_WMParameter p_azureAccountKey(AzureAccountKey_Label, "Storage Account Key", "", 90);
  ESPAsync_WMParameter p_soundSwitcherThreshold(SoundSwitcherThresholdString_Label, "Threshold", sSwiThresholdStr, 6);
  // Just a quick hint
  ESPAsync_WMParameter p_hint("<small>*Hint: if you want to reuse the currently active WiFi credentials, leave SSID and Password fields empty. <br/>*Portal Password = MyESP_'hexnumber'</small>");
  ESPAsync_WMParameter p_hint2("<small><br/>*Hint: to enter the long Azure Key, send it to your Phone by E-Mail and use copy and paste.</small>");
    
  //add all parameters here
  ESPAsync_wifiManager.addParameter(&p_hint);
  ESPAsync_wifiManager.addParameter(&p_hint2);
  ESPAsync_wifiManager.addParameter(&p_azureAccountName);
  ESPAsync_wifiManager.addParameter(&p_azureAccountKey);
  ESPAsync_wifiManager.addParameter(&p_soundSwitcherThreshold);

  //Check if there is stored WiFi router/password credentials.
  //If not found, device will remain in configuration mode until switched off via webserver.
  Serial.println(F("Opening configuration portal."));
  
  bool configDataLoaded = false;

  // From v1.1.0, Don't permit NULL password
  if ( (Router_SSID != "") && (Router_Pass != "") )
  {
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
    
    ESPAsync_wifiManager.setConfigPortalTimeout(60); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Here we could print the used WiFi-Credentials"));
    //Remove this line if you do not want to see WiFi password printed
    //Serial.println(F("Got ESP Self-Stored Credentials. Timeout 60s for Config Portal"));
  }
  
  if (loadConfigData())
  {
    configDataLoaded = true;
    
    ESPAsync_wifiManager.setConfigPortalTimeout(60); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Got stored Credentials. Timeout 60s for Config Portal"));

#if USE_ESP_WIFIMANAGER_NTP      
    if ( strlen(WM_config.TZ_Name) > 0 )
    {
      LOGERROR3(F("Current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

  #if ESP8266
      configTime(WM_config.TZ, "pool.ntp.org"); 
  #else
      //configTzTime(WM_config.TZ, "pool.ntp.org" );
      configTzTime(WM_config.TZ, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  #endif   
    }
    else
    {
      Serial.println(F("Current Timezone is not set. Enter Config Portal to set."));
    } 
#endif
  }
  else
  {
    // Enter CP only if no stored SSID on flash and file 
    Serial.println(F("Open Config Portal without Timeout: No stored Credentials."));
    initialConfig = true;
  }

  //RoSchmi must always be true in this App 
  initialConfig = true;

  if (initialConfig)
  {
    Serial.print(F("Starting configuration portal @ "));
    
#if USE_CUSTOM_AP_IP    
    Serial.print(APStaticIP);
#else
    Serial.print(F("192.168.4.1"));
#endif

    Serial.print(F(", SSID = "));
    Serial.print(ssid);
    Serial.print(F(", PWD = "));
    Serial.println(password);

    digitalWrite(LED_BUILTIN, LED_ON); // Turn led on as we are in configuration mode.

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //ESPAsync_wifiManager.setConfigPortalTimeout(600);

    // Starts an access point
    if (!ESPAsync_wifiManager.startConfigPortal((const char *) ssid.c_str(), password.c_str()))
      Serial.println(F("Not connected to WiFi but continuing anyway."));
    else
    {
      Serial.println(F("WiFi connected...yeey :)"));
    }

    // Stored  for later usage, from v1.1.0, but clear first
    memset(&WM_config, 0, sizeof(WM_config));
    
    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      String tempSSID = ESPAsync_wifiManager.getSSID(i);
      String tempPW   = ESPAsync_wifiManager.getPW(i);
  
      if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);  

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }
#if USE_ESP_WIFIMANAGER_NTP      
    String tempTZ   = ESPAsync_wifiManager.getTimezoneName();

    if (strlen(tempTZ.c_str()) < sizeof(WM_config.TZ_Name) - 1)
      strcpy(WM_config.TZ_Name, tempTZ.c_str());
    else
      strncpy(WM_config.TZ_Name, tempTZ.c_str(), sizeof(WM_config.TZ_Name) - 1);

    const char * TZ_Result = ESPAsync_wifiManager.getTZ(WM_config.TZ_Name);
    
    if (strlen(TZ_Result) < sizeof(WM_config.TZ) - 1)
      strcpy(WM_config.TZ, TZ_Result);
    else
      strncpy(WM_config.TZ, TZ_Result, sizeof(WM_config.TZ_Name) - 1);
         
    if ( strlen(WM_config.TZ_Name) > 0 )
    {
      LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

#if ESP8266
      configTime(WM_config.TZ, "pool.ntp.org"); 
#else
      //configTzTime(WM_config.TZ, "pool.ntp.org" );
      configTzTime(WM_config.TZ, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
#endif
    }
    else
    {
      LOGERROR(F("Current Timezone Name is not set. Enter Config Portal to set."));
    }
#endif

    // New in v1.4.0
    ESPAsync_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);
    //////
    
    saveConfigData();
  }

  // Getting posted form values and overriding local variables parameters
  // Config file is written regardless the connection state 
  strcpy(azureAccountName, p_azureAccountName.getValue());
  if (strlen(p_azureAccountKey.getValue()) > 1)
  {
    strcpy(azureAccountKey, p_azureAccountKey.getValue());
  }
  strcpy(sSwiThresholdStr, p_soundSwitcherThreshold.getValue());
    
    // Writing JSON config file to flash for next boot

  writeConfigFile();

  digitalWrite(LED_BUILTIN, LED_OFF); // Turn led off as we are not in configuration mode.

  startedAt = millis();

  // Azure Acount must be updated here with eventually changed values from WiFi-Manager
  myCloudStorageAccount.ChangeAccountParams((char *)azureAccountName, (char *)azureAccountKey, UseHttps_State);
  
  #if WORK_WITH_WATCHDOG == 1
    // Start watchdog with 20 seconds
    if (esp_task_wdt_init(20, true) == ESP_OK)
    {
      Serial.println(F("\r\nWatchdog enabled with interval of 20 sec\r\n"));
    }
    else
    {
      Serial.println(F("Failed to enable watchdog"));
    }
    esp_task_wdt_add(NULL);

    //https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/watchdog-und-heartbeat

    //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html
  #endif 

  uint8_t status;
  int i = 0;
  status = wifiMulti.run();
             
  delay(800L);  //(WIFI_MULTI_1ST_CONNECT_WAITING_MS) 

  bool Connected_To_Router = connect_Wifi(Router_SSID.c_str(), Router_Pass.c_str());
  
  /* Alternativ routine to connect (from Khoih-prog example)
  while ( ( i++ < 20 ) && ( status != WL_CONNECTED ) )
  {
    status = WiFi.status();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }
  */

  if ( status == WL_CONNECTED )
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println(ESPAsync_wifiManager.getStatus(WiFi.status()));
    #if WORK_WITH_WATCHDOG == 1
      while (true)  // Wait to be rebooted by watchdog
      {
        delay(100);
      }
    #endif
  }

  /*
  while (true)
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(WiFi.localIP());
    delay (1000);
  }
  */

  soundSwitcher.begin(atoi((char *)sSwiThresholdStr), Hysteresis::Percent_10, soundSwitcherUpdateInterval, soundSwitcherReadDelayTime);
  soundSwitcher.SetCalibrationParams(-20.0);  // optional
  soundSwitcher.SetActive();
  //**************************************************************
  onOffDataContainer.begin(DateTime(), OnOffTableName_1, OnOffTableName_2, OnOffTableName_3, OnOffTableName_4);

  // Initialize State of 4 On/Off-sensor representations 
  // and of the inverter flags (Application specific)
  for (int i = 0; i < 4; i++)
  {
    onOffDataContainer.PresetOnOffState(i, false, true);
    onOffDataContainer.Set_OutInverter(i, true);
    onOffDataContainer.Set_InputInverter(i, false);
  }

  //Initialize OnOffSwitcher (for tests and simulation)
  onOffSwitcherWio.begin(TimeSpan(15 * 60));   // Toggle every 30 min
  //onOffSwitcherWio.SetInactive();
  onOffSwitcherWio.SetActive();

// Setting Daylightsavingtime. Enter values for your zone in file include/config.h
  // Program aborts in some cases of invalid values
  
  int dstWeekday = getDayNum(DST_START_WEEKDAY);
  int dstMonth = getMonNum(DST_START_MONTH);
  int dstWeekOfMonth = getWeekOfMonthNum(DST_START_WEEK_OF_MONTH);

  TimeChangeRule dstStart {DST_ON_NAME, (uint8_t)dstWeekOfMonth, (uint8_t)dstWeekday, (uint8_t)dstMonth, DST_START_HOUR, TIMEZONEOFFSET + DSTOFFSET};
  
  bool firstTimeZoneDef_is_Valid = (dstWeekday == -1 || dstMonth == - 1 || dstWeekOfMonth == -1 || DST_START_HOUR > 23 ? true : DST_START_HOUR < 0 ? true : false) ? false : true;
  
  dstWeekday = getDayNum(DST_STOP_WEEKDAY);
  dstMonth = getMonNum(DST_STOP_MONTH);
  dstWeekOfMonth = getWeekOfMonthNum(DST_STOP_WEEK_OF_MONTH);

  TimeChangeRule stdStart {DST_OFF_NAME, (uint8_t)dstWeekOfMonth, (uint8_t)dstWeekday, (uint8_t)dstMonth, (uint8_t)DST_START_HOUR, (int)TIMEZONEOFFSET};

  bool secondTimeZoneDef_is_Valid = (dstWeekday == -1 || dstMonth == - 1 || dstWeekOfMonth == -1 || DST_STOP_HOUR > 23 ? true : DST_STOP_HOUR < 0 ? true : false) ? false : true;
  
  if (firstTimeZoneDef_is_Valid && secondTimeZoneDef_is_Valid)
  {
    myTimezone.setRules(dstStart, stdStart);
  }
  else
  {
    // If timezonesettings are not valid: -> take UTC and wait for ever  
    TimeChangeRule stdStart {DST_OFF_NAME, (uint8_t)dstWeekOfMonth, (uint8_t)dstWeekday, (uint8_t)dstMonth, (uint8_t)DST_START_HOUR, (int)0};
    myTimezone.setRules(stdStart, stdStart);
    while (true)
    {
      Serial.println("Invalid DST Timezonesettings");
      delay(5000);
    }
  }
  
  
  Serial.println(F("Starting timeClient"));
  timeClient.begin();
  timeClient.setUpdateInterval((NTP_UPDATE_INTERVAL_MINUTES < 1 ? 1 : NTP_UPDATE_INTERVAL_MINUTES) * 60 * 1000);
  // 'setRetryInterval' should not be too short, may be that short intervals lead to malfunction 
  timeClient.setRetryInterval(20000);  // Try to read from NTP Server not more often than every 20 seconds
  Serial.println("Using NTP Server " + timeClient.getPoolServerName());
  
  timeClient.update();
  uint32_t counter = 0;
  uint32_t maxCounter = 10;
  
  while(!timeClient.updated() &&  counter++ <= maxCounter)
  {
    Serial.println(F("NTP FAILED: Trying again"));
    delay(1000);
    #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();
    #endif
    timeClient.update();
  }

  if (counter >= maxCounter)
  {
    while(true)
    {
      delay(500); //Wait for ever, could not get NTP time, eventually reboot by Watchdog
    }
  }

  Serial.println("\r\n********UPDATED********");
    
  Serial.println("UTC : " + timeClient.getFormattedUTCTime());
  Serial.println("UTC : " + timeClient.getFormattedUTCDateTime());
  Serial.println("LOC : " + timeClient.getFormattedTime());
  Serial.println("LOC : " + timeClient.getFormattedDateTime());
  Serial.println("UTC EPOCH : " + String(timeClient.getUTCEpochTime()));
  Serial.println("LOC EPOCH : " + String(timeClient.getEpochTime()));

  unsigned long utcTime = timeClient.getUTCEpochTime();  // Seconds since 1. Jan. 1970
  
  dateTimeUTCNow =  utcTime;

  Serial.printf("%s %i %02d %02d %02d %02d", (char *)"UTC-Time is  :", dateTimeUTCNow.year(), 
                                        dateTimeUTCNow.month() , dateTimeUTCNow.day(),
                                        dateTimeUTCNow.hour() , dateTimeUTCNow.minute());
  Serial.println("");
  
  DateTime localTime = myTimezone.toLocal(dateTimeUTCNow.unixtime());
  
  Serial.printf("%s %i %02d %02d %02d %02d", (char *)"Local-Time is:", localTime.year(), 
                                        localTime.month() , localTime.day(),
                                        localTime.hour() , localTime.minute());
  Serial.println("");

  analogSensorMgr.SetReadInterval(ANALOG_SENSOR_READ_INTERVAL_SECONDS);

}

void loop()
{
  check_status();
  // put your main code here, to run repeatedly:
  if (++loopCounter % 100000 == 0)   // Make decisions to send data every 100000 th round and toggle Led to signal that App is running
  {
    
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);    // toggle LED to signal that App is running

    #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();
    #endif
    
      // Update RTC from Ntp when ntpUpdateInterval has expired, retry when RetryInterval has expired       
      if (timeClient.update())
      {                                                                      
        dateTimeUTCNow = timeClient.getUTCEpochTime();
        
        timeNtpUpdateCounter++;

        #if SERIAL_PRINT == 1
          // Indicate that NTP time was updated         
          char buffer[] = "NTP-Utc: YYYY-MM-DD hh:mm:ss";           
          dateTimeUTCNow.toString(buffer);
          Serial.println(buffer);
        #endif
      }  // End NTP stuff
          
      dateTimeUTCNow = timeClient.getUTCEpochTime();
      
      // Get offset in minutes between UTC and local time with consideration of DST
      int timeZoneOffsetUTC = myTimezone.utcIsDST(dateTimeUTCNow.unixtime()) ? TIMEZONEOFFSET + DSTOFFSET : TIMEZONEOFFSET;
      
      DateTime localTime = myTimezone.toLocal(dateTimeUTCNow.unixtime());

      // In the last 15 sec of each day we set a pulse to Off-State when we had On-State before
      bool isLast15SecondsOfDay = (localTime.hour() == 23 && localTime.minute() == 59 &&  localTime.second() > 45) ? true : false;
      
      // Get readings from 4 differend analog sensors and store the values in a container     
       
      dataContainer.SetNewValue(0, dateTimeUTCNow, ReadAnalogSensor(0));
      dataContainer.SetNewValue(1, dateTimeUTCNow, ReadAnalogSensor(1));
      dataContainer.SetNewValue(2, dateTimeUTCNow, ReadAnalogSensor(2));
      dataContainer.SetNewValue(3, dateTimeUTCNow, ReadAnalogSensor(3));

      

      // Check if automatic OnOfSwitcher has toggled (used to simulate on/off changes)
      // and accordingly change the state of one representation (here index 0 and 1) in onOffDataContainer
      if (onOffSwitcherWio.hasToggled(dateTimeUTCNow))
      {
        bool state = onOffSwitcherWio.GetState();
        onOffDataContainer.SetNewOnOffValue(2, state, dateTimeUTCNow, timeZoneOffsetUTC);
        onOffDataContainer.SetNewOnOffValue(3, !state, dateTimeUTCNow, timeZoneOffsetUTC);    
      }
        
        
        if (feedResult.isValid && (feedResult.hasToggled || feedResult.analogToSend))
        {
            if (feedResult.hasToggled)
            {
              onOffDataContainer.SetNewOnOffValue(0, feedResult.state, dateTimeUTCNow, timeZoneOffsetUTC);
              Serial.print("\r\nHas toggled, new state is: ");
              Serial.println(feedResult.state == true ? "High" : "Low");
              Serial.println();
            }
            // if toogled activate make hasToBeSent flag
            // so that the analog values have to be updated in the cloud
            if (feedResult.analogToSend)
            {
              dataContainer.setHasToBeSentFlag();
              Serial.print("Average is: ");
              Serial.println(feedResult.avValue);             
              Serial.println();
            }           
        }
        
      // Check if something is to do: send analog data ? send On/Off-Data ? Handle EndOfDay stuff ?
      if (dataContainer.hasToBeSent() || onOffDataContainer.One_hasToBeBeSent(localTime) || isLast15SecondsOfDay)
      {    
        //Create some buffer
        char sampleTime[25] {0};    // Buffer to hold sampletime        
        char strData[100] {0};          // Buffer to hold display message
        
        char EtagBuffer[50] {0};    // Buffer to hold returned Etag

        // Create az_span to hold partitionkey
        char partKeySpan[25] {0};
        size_t partitionKeyLength = 0;
        az_span partitionKey = AZ_SPAN_FROM_BUFFER(partKeySpan);
        
        // Create az_span to hold rowkey
        char rowKeySpan[25] {0};
        size_t rowKeyLength = 0;
        az_span rowKey = AZ_SPAN_FROM_BUFFER(rowKeySpan);

        if (dataContainer.hasToBeSent())       // have to send analog values ?
        {    
          // Retrieve edited sample values from container
          SampleValueSet sampleValueSet = dataContainer.getCheckedSampleValues(dateTimeUTCNow);
                  
          createSampleTime(sampleValueSet.LastUpdateTime, timeZoneOffsetUTC, (char *)sampleTime);

          // Define name of the table (arbitrary name + actual year, like: AnalogTestValues2020)
          String augmentedAnalogTableName = analogTableName; 
          if (augmentTableNameWithYear)
          {
            // RoSchmi changed 10.07.2024 to resolve issue 1         
            //augmentedAnalogTableName += (dateTimeUTCNow.year());
            augmentedAnalogTableName += (localTime.year());  
          }
          
          // Create Azure Storage Table if table doesn't exist
          if (localTime.year() != dataContainer.Year)    // if new year
          {  
            az_http_status_code theResult = createTable(myCloudStorageAccountPtr, myX509Certificate, (char *)augmentedAnalogTableName.c_str());
                     
            if ((theResult == AZ_HTTP_STATUS_CODE_CONFLICT) || (theResult == AZ_HTTP_STATUS_CODE_CREATED))
            {
              dataContainer.Set_Year(localTime.year());                   
            }
            else
            {
              // Reset board if not successful
             
             //SCB_AIRCR = 0x05FA0004;             
            }                     
          }         
          // Create an Array of (here) 5 Properties
          // Each Property consists of the Name, the Value and the Type (here only Edm.String is supported)

          // Besides PartitionKey and RowKey we have 5 properties to be stored in a table row
          // (SampleTime and 4 samplevalues)
          size_t analogPropertyCount = 5;
          EntityProperty AnalogPropertiesArray[5];
          AnalogPropertiesArray[0] = (EntityProperty)TableEntityProperty((char *)"SampleTime", (char *) sampleTime, (char *)"Edm.String");
          AnalogPropertiesArray[1] = (EntityProperty)TableEntityProperty((char *)"T_1", (char *)floToStr(sampleValueSet.SampleValues[0].Value).c_str(), (char *)"Edm.String");
          AnalogPropertiesArray[2] = (EntityProperty)TableEntityProperty((char *)"T_2", (char *)floToStr(sampleValueSet.SampleValues[1].Value).c_str(), (char *)"Edm.String");
          AnalogPropertiesArray[3] = (EntityProperty)TableEntityProperty((char *)"T_3", (char *)floToStr(sampleValueSet.SampleValues[2].Value).c_str(), (char *)"Edm.String");
          AnalogPropertiesArray[4] = (EntityProperty)TableEntityProperty((char *)"T_4", (char *)floToStr(sampleValueSet.SampleValues[3].Value).c_str(), (char *)"Edm.String");
  
          // Create the PartitionKey (special format)
          makePartitionKey(analogTablePartPrefix, augmentPartitionKey, localTime, partitionKey, &partitionKeyLength);
          partitionKey = az_span_slice(partitionKey, 0, partitionKeyLength);

          // Create the RowKey (special format)        
          makeRowKey(localTime, rowKey, &rowKeyLength);
          
          rowKey = az_span_slice(rowKey, 0, rowKeyLength);
  
          // Create TableEntity consisting of PartitionKey, RowKey and the properties named 'SampleTime', 'T_1', 'T_2', 'T_3' and 'T_4'
          AnalogTableEntity analogTableEntity(partitionKey, rowKey, az_span_create_from_str((char *)sampleTime),  AnalogPropertiesArray, analogPropertyCount);
          
          #if SERIAL_PRINT == 1
            Serial.printf("Trying to insert %u \r\n", insertCounterAnalogTable);
          #endif  
             
          // Keep track of tries to insert and check for memory leak
          insertCounterAnalogTable++;

          // RoSchmi, Todo: event. include code to check for memory leaks here

          // Store Entity to Azure Cloud   
          __unused az_http_status_code insertResult =  insertTableEntity(myCloudStorageAccountPtr, myX509Certificate, (char *)augmentedAnalogTableName.c_str(), analogTableEntity, (char *)EtagBuffer);
                 
        }
        // Now test if Send On/Off values or End of day stuff?
        if (onOffDataContainer.One_hasToBeBeSent(localTime) || isLast15SecondsOfDay)
        {        
          OnOffSampleValueSet onOffValueSet = onOffDataContainer.GetOnOffValueSet();

          for (int i = 0; i < 4; i++)    // Do for 4 OnOff-Tables  
          {
            DateTime lastSwitchTimeDate = DateTime(onOffValueSet.OnOffSampleValues[i].LastSwitchTime.year(), 
                                                onOffValueSet.OnOffSampleValues[i].LastSwitchTime.month(), 
                                                onOffValueSet.OnOffSampleValues[i].LastSwitchTime.day());

            DateTime actTimeDate = DateTime(localTime.year(), localTime.month(), localTime.day());

            if (onOffValueSet.OnOffSampleValues[i].hasToBeSent || ((onOffValueSet.OnOffSampleValues[i].actState == true) &&  (lastSwitchTimeDate.operator!=(actTimeDate))))
            {
              if (onOffValueSet.OnOffSampleValues[i].hasToBeSent)
              {
                onOffDataContainer.Reset_hasToBeSent(i);     
                EntityProperty OnOffPropertiesArray[5];

                // RoSchmi
                TimeSpan  onTime = onOffValueSet.OnOffSampleValues[i].OnTimeDay;
                if (lastSwitchTimeDate.operator!=(actTimeDate))
                {
                  onTime = TimeSpan(0);                 
                  onOffDataContainer.Set_OnTimeDay(i, onTime);

                  if (onOffValueSet.OnOffSampleValues[i].actState == true)
                  {
                    onOffDataContainer.Set_LastSwitchTime(i, actTimeDate);
                  }
                }
                          
              char OnTimeDay[15] = {0};
              sprintf(OnTimeDay, "%03i-%02i:%02i:%02i", onTime.days(), onTime.hours(), onTime.minutes(), onTime.seconds());
              createSampleTime(dateTimeUTCNow, timeZoneOffsetUTC, (char *)sampleTime);

              // Tablenames come from the onOffValueSet, here usually the tablename is augmented with the actual year
              String augmentedOnOffTableName = onOffValueSet.OnOffSampleValues[i].tableName;
              if (augmentTableNameWithYear)
              {               
                augmentedOnOffTableName += (localTime.year()); 
              }

              // Create table if table doesn't exist
              if (localTime.year() != onOffValueSet.OnOffSampleValues[i].Year)
              {
                 az_http_status_code theResult = createTable(myCloudStorageAccountPtr, myX509Certificate, (char *)augmentedOnOffTableName.c_str());
                 
                 if ((theResult == AZ_HTTP_STATUS_CODE_CONFLICT) || (theResult == AZ_HTTP_STATUS_CODE_CREATED))
                 {
                    onOffDataContainer.Set_Year(i, localTime.year());
                 }
                 else
                 {
                    delay(3000);
                     //Reset Teensy 4.1
                    //SCB_AIRCR = 0x05FA0004;      
                 }
              }
              
              TimeSpan TimeFromLast = onOffValueSet.OnOffSampleValues[i].TimeFromLast;

              char timefromLast[15] = {0};
              sprintf(timefromLast, "%03i-%02i:%02i:%02i", TimeFromLast.days(), TimeFromLast.hours(), TimeFromLast.minutes(), TimeFromLast.seconds());
                         
              size_t onOffPropertyCount = 5;
              OnOffPropertiesArray[0] = (EntityProperty)TableEntityProperty((char *)"ActStatus", onOffValueSet.OnOffSampleValues[i].outInverter ? (char *)(onOffValueSet.OnOffSampleValues[i].actState ? "On" : "Off") : (char *)(onOffValueSet.OnOffSampleValues[i].actState ? "Off" : "On"), (char *)"Edm.String");
              OnOffPropertiesArray[1] = (EntityProperty)TableEntityProperty((char *)"LastStatus", onOffValueSet.OnOffSampleValues[i].outInverter ? (char *)(onOffValueSet.OnOffSampleValues[i].lastState ? "On" : "Off") : (char *)(onOffValueSet.OnOffSampleValues[i].lastState ? "Off" : "On"), (char *)"Edm.String");
              OnOffPropertiesArray[2] = (EntityProperty)TableEntityProperty((char *)"OnTimeDay", (char *) OnTimeDay, (char *)"Edm.String");
              OnOffPropertiesArray[3] = (EntityProperty)TableEntityProperty((char *)"SampleTime", (char *) sampleTime, (char *)"Edm.String");
              OnOffPropertiesArray[4] = (EntityProperty)TableEntityProperty((char *)"TimeFromLast", (char *) timefromLast, (char *)"Edm.String");
          
              // Create the PartitionKey (special format)
              makePartitionKey(onOffTablePartPrefix, augmentPartitionKey, localTime, partitionKey, &partitionKeyLength);
              partitionKey = az_span_slice(partitionKey, 0, partitionKeyLength);
              
              // Create the RowKey (special format)            
              makeRowKey(localTime, rowKey, &rowKeyLength);
              
              rowKey = az_span_slice(rowKey, 0, rowKeyLength);
  
              // Create TableEntity consisting of PartitionKey, RowKey and the properties named 'SampleTime', 'T_1', 'T_2', 'T_3' and 'T_4'
              OnOffTableEntity onOffTableEntity(partitionKey, rowKey, az_span_create_from_str((char *)sampleTime),  OnOffPropertiesArray, onOffPropertyCount);
          
              onOffValueSet.OnOffSampleValues[i].insertCounter++;
              
              // Store Entity to Azure Cloud   
             __unused az_http_status_code insertResult =  insertTableEntity(myCloudStorageAccountPtr, myX509Certificate, (char *)augmentedOnOffTableName.c_str(), onOffTableEntity, (char *)EtagBuffer);
              
              delay(1000);     // wait at least 1 sec so that two uploads cannot have the same RowKey

              break;          // Send only one in each round of loop
              }
              else
              {
                if (isLast15SecondsOfDay && !onOffValueSet.OnOffSampleValues[i].dayIsLocked)
                {
                  if (onOffValueSet.OnOffSampleValues[i].actState == true)              
                  {               
                    onOffDataContainer.Set_ResetToOnIsNeededFlag(i, true);                 
                    onOffDataContainer.SetNewOnOffValue(i, onOffValueSet.OnOffSampleValues[i].inputInverter ? true : false, dateTimeUTCNow, timeZoneOffsetUTC);
                    delay(1000);   // because we don't want to send twice in the same second 
                    break;
                  }
                else
                {              
                  if (onOffValueSet.OnOffSampleValues[i].resetToOnIsNeeded)
                  {                  
                    onOffDataContainer.Set_DayIsLockedFlag(i, true);
                    onOffDataContainer.Set_ResetToOnIsNeededFlag(i, false);
                    onOffDataContainer.SetNewOnOffValue(i, onOffValueSet.OnOffSampleValues[i].inputInverter ? false : true, dateTimeUTCNow, timeZoneOffsetUTC);
                    break;
                  }                 
                }              
              }
              } 
            }                        
          }         
        } 
      }     
  } 
}

uint8_t connectMultiWiFi()
{
#if ESP32
  // For ESP32, this better be 0 to shorten the connect time.
  // For ESP32-S2/C3, must be > 500
  #if ( USING_ESP32_S2 || USING_ESP32_C3 )
    #define WIFI_MULTI_1ST_CONNECT_WAITING_MS           500L
  #else
    // For ESP32 core v1.0.6, must be >= 500
    #define WIFI_MULTI_1ST_CONNECT_WAITING_MS           800L
  #endif
#else
  // For ESP8266, this better be 2200 to enable connect the 1st time
  #define WIFI_MULTI_1ST_CONNECT_WAITING_MS             2200L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS                   500L

  uint8_t status;

  //WiFi.mode(WIFI_STA);

  LOGERROR(F("ConnectMultiWiFi with :"));

  if ( (Router_SSID != "") && (Router_Pass != "") )
  {
    LOGERROR3(F("* Flash-stored Router_SSID = "), Router_SSID, F(", Router_Pass = "), Router_Pass );
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass );
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
  }

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
  {
    // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
    if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
    {
      LOGERROR3(F("* Additional SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
    }
  }

  LOGERROR(F("Connecting MultiWifi..."));

  //WiFi.mode(WIFI_STA);

#if !USE_DHCP_IP
  // New in v1.4.0
  configWiFi(WM_STA_IPconfig);
  //////
#endif

  int i = 0;
  status = wifiMulti.run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);
  
  while ( ( i++ < 20 ) && ( status != WL_CONNECTED ) )
  {
    status = WiFi.status();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }
  
  if ( status == WL_CONNECTED )
  {
    LOGERROR1(F("WiFi connected after time: "), i);
    LOGERROR3(F("SSID:"), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
    LOGERROR3(F("Channel:"), WiFi.channel(), F(",IP address:"), WiFi.localIP() );
  }
  else
  {
    LOGERROR(F("WiFi not connected"));
 
#if ESP8266      
    ESP.reset();
#else
    ESP.restart();
#endif  
  }

  return status;
}

// To manage daylightsavingstime stuff convert input ("Last", "First", "Second", "Third", "Fourth") to int equivalent
int getWeekOfMonthNum(const char * weekOfMonth)
{
  for (int i = 0; i < 5; i++)
  {  
    if (strcmp((char *)timeNameHelper.weekOfMonth[i], weekOfMonth) == 0)
    {
      return i;
    }   
  }
  return -1;
}

int getMonNum(const char * month)
{
  for (int i = 0; i < 12; i++)
  {  
    if (strcmp((char *)timeNameHelper.monthsOfTheYear[i], month) == 0)
    {
      return i + 1;
    }   
  }
  return -1;
}

int getDayNum(const char * day)
{
  for (int i = 0; i < 7; i++)
  {  
    if (strcmp((char *)timeNameHelper.daysOfTheWeek[i], day) == 0)
    {
      return i + 1;
    }   
  }
  return -1;
}

// Scan for available Wifi networks
// print result als simple list
void scan_WIFI() 
{
      Serial.println("WiFi scan ...");
      // WiFi.scanNetworks returns the number of networks found
      int n = WiFi.scanNetworks();
      if (n == 0) {
          Serial.println("[ERR] no networks found");
      } else {
          
          Serial.printf("[OK] %i networks found:\n", n);       
          for (int i = 0; i < n; ++i) {
              // Print SSID for each network found
              Serial.printf("  %i: ",i+1);
              Serial.println(WiFi.SSID(i));
              delay(10);
          }
      }
}

// establish the connection to an Wifi Access point
//boolean connect_Wifi(const char * ssid, const char * password)
boolean connect_Wifi(const char *ssid, const char * password)
{
  // Establish connection to the specified network until success.
  // Important to disconnect in case that there is a valid connection
  WiFi.disconnect();
  Serial.println("Connecting to ");
  Serial.println(ssid);
  delay(500);
  //Start connecting (done by the ESP in the background)
  
  #if USE_WIFI_STATIC_IP == 1
  IPAddress presetIp(192, 168, 1, 83);
  IPAddress presetGateWay(192, 168, 1, 1);
  IPAddress presetSubnet(255, 255, 255, 0);
  IPAddress presetDnsServer1(8,8,8,8);
  IPAddress presetDnsServer2(8,8,4,4);

  WiFi.config(presetIp, presetGateWay, presetDnsServer1, presetDnsServer2);
  #endif

  WiFi.begin(ssid, password, 6);
  // read wifi Status
  wl_status_t wifi_Status = WiFi.status();  
  int n_trials = 0;
  // loop while waiting for Wifi connection
  // run only for 5 trials.
  while (wifi_Status != WL_CONNECTED && n_trials < 5) {
    // Check periodicaly the connection status using WiFi.status()
    // Keep checking until ESP has successfuly connected
    // or maximum number of trials is reached
    wifi_Status = WiFi.status();
    n_trials++;
    switch(wifi_Status){
      case WL_NO_SSID_AVAIL:
          Serial.println("[ERR] SSID not available");
          break;
      case WL_CONNECT_FAILED:
          Serial.println("[ERR] Connection failed");
          break;
      case WL_CONNECTION_LOST:
          Serial.println("[ERR] Connection lost");
          break;
      case WL_DISCONNECTED:
          Serial.println("[ERR] WiFi disconnected");
          break;
      case WL_IDLE_STATUS:
          Serial.println("[ERR] WiFi idle status");
          break;
      case WL_SCAN_COMPLETED:
          Serial.println("[OK] WiFi scan completed");
          break;
      case WL_CONNECTED:
          Serial.println("[OK] WiFi connected");
          break;
      default:
          Serial.println("[ERR] unknown Status");
          break;
    }
    delay(500);
  }
  if(wifi_Status == WL_CONNECTED){
    // connected
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    // not connected
    Serial.println("");
    Serial.println("[ERR] unable to connect Wifi");
    return false;
  }
}

String floToStr(float value)
{
  char buf[10];
  sprintf(buf, "%.1f", (roundf(value * 10.0))/10.0);
  return String(buf);
}

float ReadAnalogSensor(int pSensorIndex)
{
#ifndef USE_SIMULATED_SENSORVALUES
            // Use values read from an analog source
            // Change the function for each sensor to your needs

            double theRead = MAGIC_NUMBER_INVALID;

            if (analogSensorMgr.HasToBeRead(pSensorIndex, dateTimeUTCNow))
            {                     
              switch (pSensorIndex)
              {
                case 0:
                    {
                      // must be called in the first case
                      feedResult = soundSwitcher.feed();
                      if (feedResult.isValid)  
                      {
                        float soundValues[2] = {0};
                        soundValues[0] = feedResult.avValue;
                        soundValues[1] = feedResult.highAvValue;
                        analogSensorMgr.SetReadTimeAndValues(pSensorIndex, dateTimeUTCNow, soundValues[1], soundValues[0], MAGIC_NUMBER_INVALID);
                        
                        //theRead = feedResult.highAvValue / 10;

                        // Function not used in this App, can be used to display another sensor
                        theRead = MAGIC_NUMBER_INVALID;

                        // Take theRead (nearly) 0.0 as invalid
                        // (if no sensor is connected the function returns 0)                        
                        if (theRead > - 0.00001 && theRead < 0.00001)
                        {
                          theRead = MAGIC_NUMBER_INVALID;
                        }
                      }                                                                               
                    }
                    break;

                case 1:
                    {
                      if (feedResult.isValid)  
                      {
                        float soundValues[2] = {0};
                        soundValues[0] = feedResult.avValue;
                        soundValues[1] = feedResult.highAvValue;
                        
                        // Here we look if the sound sensor was updated in this loop
                        // If yes, we can get the average value from the index 0 sensor
                        AnalogSensor tempSensor = analogSensorMgr.GetSensorDates(0);
                        if (tempSensor.LastReadTime.operator==(dateTimeUTCNow))
                        {
                          analogSensorMgr.SetReadTimeAndValues(pSensorIndex, dateTimeUTCNow, soundValues[1], soundValues[0], MAGIC_NUMBER_INVALID);
                          theRead = feedResult.avValue / 10;
                          // is limited to be not more than 100
                          theRead = theRead <= 100 ? theRead : 100.0;
                            // Take theRead (nearly) 0.0 as invalid
                            // (if no sensor is connected the function returns 0)                        
                            if ((theRead > - 0.00001 && theRead < 0.00001))
                            {      
                                  theRead = MAGIC_NUMBER_INVALID;                                                  
                            }
                        }                             
                      }                
                    }
                    break;
                case 2:
                    {  
                        // Show ascending lines from 0 to 5, so re-boots of the board are indicated                                                
                        theRead = ((double)(insertCounterAnalogTable % 50)) / 10;
                        return theRead;                                                                   
                    }
                    break;
                case 3:
                    {
                      // Line for the switch threshold
                      theRead = atoi((char *)sSwiThresholdStr) / 10; // dummy
                    }                   
                    break;
              }
            }                    
            return theRead ;
#endif

#ifdef USE_SIMULATED_SENSORVALUES
      #ifdef USE_TEST_VALUES
            // Here you can select that diagnostic values (for debugging)
            // are sent to your storage table
            double theRead = MAGIC_NUMBER_INVALID;
            switch (pSensorIndex)
            {
                case 0:
                    {
                        theRead = timeNtpUpdateCounter;
                        theRead = theRead / 10; 
                    }
                    break;

                case 1:
                    {                       
                        theRead = sysTimeNtpDelta > 140 ? 140 : sysTimeNtpDelta < - 40 ? -40 : (double)sysTimeNtpDelta;                      
                    }
                    break;
                case 2:
                    {
                        theRead = insertCounterAnalogTable;
                        theRead = theRead / 10;                      
                    }
                    break;
                case 3:
                    {
                        theRead = lastResetCause;                       
                    }
                    break;
            }

            return theRead ;

  

        #endif
            
            onOffSwitcherWio.SetActive();
            // Only as an example we here return values which draw a sinus curve            
            int frequDeterminer = 4;
            int y_offset = 1;
            // different frequency and y_offset for aIn_0 to aIn_3
            if (pSensorIndex == 0)
            { frequDeterminer = 4; y_offset = 1; }
            if (pSensorIndex == 1)
            { frequDeterminer = 8; y_offset = 10; }
            if (pSensorIndex == 2)
            { frequDeterminer = 12; y_offset = 20; }
            if (pSensorIndex == 3)
            { frequDeterminer = 16; y_offset = 30; }
             
            int secondsOnDayElapsed = dateTimeUTCNow.second() + dateTimeUTCNow.minute() * 60 + dateTimeUTCNow.hour() *60 *60;

            // RoSchmi
            switch (pSensorIndex)
            {
              case 3:
              {

                //return (double)9.9;   // just something for now
                return lastResetCause;
              }
              break;
            
              case 2:
              { 
                uint32_t showInsertCounter = insertCounterAnalogTable % 50;               
                double theRead = ((double)showInsertCounter) / 10;
                return theRead;
              }
              break;
              case 0:
              case 1:
              {
                return roundf((float)25.0 * (float)sin(PI / 2.0 + (secondsOnDayElapsed * ((frequDeterminer * PI) / (float)86400)))) / 10  + y_offset;          
              }
              break;
              default:
              {
                return 0;
              }
            }
  #endif
}
void createSampleTime(DateTime dateTimeUTCNow, int timeZoneOffsetUTC, char * sampleTime)
{
  int hoursOffset = timeZoneOffsetUTC / 60;
  int minutesOffset = timeZoneOffsetUTC % 60;
  char sign = timeZoneOffsetUTC < 0 ? '-' : '+';
  char TimeOffsetUTCString[10];
  sprintf(TimeOffsetUTCString, " %c%03i", sign, timeZoneOffsetUTC);
  TimeSpan timespanOffsetToUTC = TimeSpan(0, hoursOffset, minutesOffset, 0);
  DateTime newDateTime = dateTimeUTCNow + timespanOffsetToUTC;
  sprintf(sampleTime, "%02i/%02i/%04i %02i:%02i:%02i%s",newDateTime.month(), newDateTime.day(), newDateTime.year(), newDateTime.hour(), newDateTime.minute(), newDateTime.second(), TimeOffsetUTCString);
}
 
void makeRowKey(DateTime actDate,  az_span outSpan, size_t *outSpanLength)
{
  // formatting the RowKey (= reverseDate) this way to have the tables sorted with last added row upmost
  char rowKeyBuf[20] {0};

  sprintf(rowKeyBuf, "%4i%02i%02i%02i%02i%02i", (10000 - actDate.year()), (12 - actDate.month()), (31 - actDate.day()), (23 - actDate.hour()), (59 - actDate.minute()), (59 - actDate.second()));
  az_span retValue = az_span_create_from_str((char *)rowKeyBuf);
  az_span_copy(outSpan, retValue);
  *outSpanLength = retValue._internal.size;         
}

void makePartitionKey(const char * partitionKeyprefix, bool augmentWithYear, DateTime dateTime, az_span outSpan, size_t *outSpanLength)
{
  // if wanted, augment with year and month (12 - month for right order)                    
  char dateBuf[20] {0};
  sprintf(dateBuf, "%s%d-%02d", partitionKeyprefix, (dateTime.year()), (12 - dateTime.month()));                  
  az_span ret_1 = az_span_create_from_str((char *)dateBuf);
  az_span ret_2 = az_span_create_from_str((char *)partitionKeyprefix);                       
  if (augmentWithYear == true)
  {
    az_span_copy(outSpan, ret_1);            
    *outSpanLength = ret_1._internal.size; 
  }
    else
  {
    az_span_copy(outSpan, ret_2);
    *outSpanLength = ret_2._internal.size;
  }    
}

az_http_status_code createTable(CloudStorageAccount *pAccountPtr, X509Certificate pCaCert, const char * pTableName)
{ 

  #if TRANSPORT_PROTOCOL == 1
    static WiFiClientSecure wifi_client;
  #else
    static WiFiClient wifi_client;
  #endif

    #if TRANSPORT_PROTOCOL == 1
    wifi_client.setCACert(myX509Certificate);
    //wifi_client.setCACert(baltimore_corrupt_root_ca);
  #endif

  #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();
  #endif
  
  // RoSchmi
  TableClient table(pAccountPtr, pCaCert,  httpPtr, &wifi_client, bufferStorePtr);

  // Create Table
  az_http_status_code statusCode = table.CreateTable(pTableName, dateTimeUTCNow, ContType::contApplicationIatomIxml, AcceptType::acceptApplicationIjson, returnContent, false);
  
   // RoSchmi for tests: to simulate failed upload
   //az_http_status_code   statusCode = AZ_HTTP_STATUS_CODE_UNAUTHORIZED;

  char codeString[35] {0};
  if ((statusCode == AZ_HTTP_STATUS_CODE_CONFLICT) || (statusCode == AZ_HTTP_STATUS_CODE_CREATED))
  {
    #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();
    #endif
   
      sprintf(codeString, "%s %i", "Table available: ", az_http_status_code(statusCode));
      #if SERIAL_PRINT == 1
        Serial.println((char *)codeString);
      #endif
  
  }
  else
  {
    
    
      sprintf(codeString, "%s %i", "Table Creation failed: ", az_http_status_code(statusCode));
      #if SERIAL_PRINT == 1   
        Serial.println((char *)codeString);
      #endif
 
    delay(1000);

    ESP.restart();
    
  }
  

return statusCode;
}

az_http_status_code insertTableEntity(CloudStorageAccount *pAccountPtr,  X509Certificate pCaCert, const char * pTableName, TableEntity pTableEntity, char * outInsertETag)
{ 
  #if TRANSPORT_PROTOCOL == 1
    static WiFiClientSecure wifi_client;
  #else
    static WiFiClient wifi_client;
  #endif
  
  #if TRANSPORT_PROTOCOL == 1
    wifi_client.setCACert(myX509Certificate); 
  #endif
  
  /*
  // For tests: Try second upload with corrupted certificate to provoke failure
  #if TRANSPORT_PROTOCOL == 1
    wifi_client.setCACert(myX509Certificate);
    if (insertCounterAnalogTable == 2)
    {
      wifi_client.setCACert(baltimore_corrupt_root_ca);
    }
  #endif
  */

  // RoSchmi
  TableClient table(pAccountPtr, pCaCert,  httpPtr, &wifi_client, bufferStorePtr);

  #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();
  #endif
  
  DateTime responseHeaderDateTime = DateTime();   // Will be filled with DateTime value of the resonse from Azure Service

  // Insert Entity
  az_http_status_code statusCode = table.InsertTableEntity(pTableName, dateTimeUTCNow, pTableEntity, (char *)outInsertETag, &responseHeaderDateTime, ContType::contApplicationIatomIxml, AcceptType::acceptApplicationIjson, ResponseType::dont_returnContent, false);
  
  #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();
  #endif

  lastResetCause = 0;
  tryUploadCounter++;

   // RoSchmi for tests: to simulate failed upload
  //az_http_status_code   statusCode = AZ_HTTP_STATUS_CODE_UNAUTHORIZED;
  
  if ((statusCode == AZ_HTTP_STATUS_CODE_NO_CONTENT) || (statusCode == AZ_HTTP_STATUS_CODE_CREATED))
  {
      char codeString[35] {0};
      sprintf(codeString, "%s %i", "Entity inserted: ", az_http_status_code(statusCode));
      #if SERIAL_PRINT == 1
        Serial.println((char *)codeString);
      #endif
    
    #if UPDATE_TIME_FROM_AZURE_RESPONSE == 1    // System time shall be updated from the DateTime value of the response ?
    
    dateTimeUTCNow = responseHeaderDateTime;
    
    char buffer[] = "Azure-Utc: YYYY-MM-DD hh:mm:ss";
    dateTimeUTCNow.toString(buffer);
    #if SERIAL_PRINT == 1
      Serial.println((char *)buffer);
      Serial.println("");
    #endif
    
    #endif   
  }
  else            // request failed
  {               // note: internal error codes from -1 to -11 were converted for tests to error codes 401 to 411 since
                  // negative values cannot be returned as 'az_http_status_code' 

    failedUploadCounter++;
    //sendResultState = false;
    lastResetCause = 100;      // Set lastResetCause to arbitrary value of 100 to signal that post request failed
    
    
      char codeString[35] {0};
      sprintf(codeString, "%s %i", "Insertion failed: ", az_http_status_code(statusCode));
      #if SERIAL_PRINT == 1
        Serial.println((char *)codeString);
      #endif
  
    
    #if REBOOT_AFTER_FAILED_UPLOAD == 1   // When selected in config.h -> Reboot through SystemReset after failed uoload

        #if TRANSPORT_PROTOCOL == 1         
          ESP.restart();        
        #endif
        #if TRANSPORT_PROTOCOL == 0     // for http requests reboot after the second, not the first, failed request
          if(failedUploadCounter > 1)
          {
            ESP.restart();
          }
    #endif

    #endif

    #if WORK_WITH_WATCHDOG == 1
      esp_task_wdt_reset();  
    #endif
    delay(1000);
  }
  
  return statusCode;
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