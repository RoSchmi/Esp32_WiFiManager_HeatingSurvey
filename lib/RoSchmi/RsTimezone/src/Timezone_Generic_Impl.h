/*********************************************************************************************************************************
  Timezone_Generic_Impl.h
  This is a modification of the original file by Khoi Hoang
  This modification is for use with ESP32 dev board, will perhaps work with ESP8266 too
  Version: 1.10.1
  Modified by RoSchmi May 2024
  https://github.com/RoSchmi/Esp32_WiFiManager_HeatingSurvey/tree/test
  **********************************************************************************************************************************/

/*********************************************************************************************************************************
  Timezone_Generic_Impl.h
  
  For AVR, ESP8266/ESP32, SAMD21/SAMD51, nRF52, STM32, WT32_ETH01 boards

  Based on and modified from Arduino Timezone Library (https://github.com/JChristensen/Timezone)
  to support other boards such as ESP8266/ESP32, SAMD21, SAMD51, Adafruit's nRF52 boards, etc.

  Copyright (C) 2018 by Jack Christensen and licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

  Built by Khoi Hoang https://github.com/khoih-prog/Timezone_Generic
  Licensed under MIT license
  
  Version: 1.10.1

  Version Modified By  Date      Comments
  ------- -----------  ---------- -----------
  1.2.4   K Hoang      17/10/2020 Initial porting to support SAM DUE, SAMD21, SAMD51, nRF52, ESP32/ESP8266, STM32, etc. boards
                                  using SPIFFS, LittleFS, EEPROM, FlashStorage, DueFlashStorage.
  1.2.5   K Hoang      28/10/2020 Add examples to use STM32 Built-In RTC.
  1.2.6   K Hoang      01/11/2020 Allow un-initialized TZ then use begin() method to set the actual TZ (Credit of 6v6gt)
  1.3.0   K Hoang      09/01/2021 Add support to ESP32/ESP8266 using LittleFS/SPIFFS, and to AVR, UNO WiFi Rev2, etc.
                                  Fix compiler warnings.
  1.4.0   K Hoang      04/06/2021 Add support to RP2040-based boards using RP2040 Arduino-mbed or arduino-pico core
  1.5.0   K Hoang      13/06/2021 Add support to ESP32-S2 and ESP32-C3. Fix bug
  1.6.0   K Hoang      16/07/2021 Add support to WT32_ETH01
  1.7.0   K Hoang      10/08/2021 Add support to Ameba Realtek RTL8720DN, RTL8722DM and RTM8722CSM
  1.7.1   K Hoang      10/10/2021 Update `platform.ini` and `library.json`
  1.7.2   K Hoang      02/11/2021 Fix crashing issue for new cleared flash
  1.7.3   K Hoang      01/12/2021 Auto detect ESP32 core for LittleFS. Fix bug in examples for WT32_ETH01
  1.8.0   K Hoang      31/12/2021 Fix `multiple-definitions` linker error
  1.9.0   K Hoang      20/01/2022 Make compatible to old code
  1.9.1   K Hoang      26/01/2022 Update to be compatible with new FlashStorage libraries. Add support to more SAMD/STM32 boards
  1.10.0  K Hoang      06/04/2022 Use Ethernet_Generic library as default. Add support to Portenta_H7 Ethernet and WiFi
  1.10.1  K Hoang      25/09/2022 Add support to `RP2040W` using `CYW43439 WiFi` with `arduino-pico` core
 **********************************************************************************************************************************/

#pragma once

#ifndef TIMEZONE_GENERIC_IMPL_H
#define TIMEZONE_GENERIC_IMPL_H

#define  TZ_FILENAME      "/timezone.dat"
#define  TZ_DATA_OFFSET   0

#define TZ_USE_EEPROM      false

/////////////////////////////

// To eliminate warnings with [-Wundef]
#define TZ_USE_ESP32        true
#define TZ_USE_ESP8266      false
#define TZ_USE_SAMD         false
#define TZ_USE_SAM_DUE      false
#define TZ_USE_NRF52        false
#define TZ_USE_STM32        false
#define TZ_USE_RP2040       false
#define TZ_USE_MBED_RP2040  false


#ifndef TZ_DEBUG
  #define TZ_DEBUG       true
#endif


#if defined(ESP32)

  #if ( ARDUINO_ESP32C3_DEV )
    // https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-gpio.c
    #warning ESP32-C3 boards not fully supported yet. Only SPIFFS and EEPROM OK. Tempo esp32_adc2gpio to be replaced
    const int8_t esp32_adc2gpio[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    #warning Using ESP32-C3
  #endif

  #if ( ARDUINO_ESP32C3_DEV )
    // Currently, ESP32-C3 only supporting SPIFFS and EEPROM. Will fix to support LittleFS
    #ifdef USE_LITTLEFS
      #undef USE_LITTLEFS
    #endif
    #define USE_LITTLEFS          false
  #endif

  #if defined(TZ_USE_ESP32)
    #undef TZ_USE_ESP32
  #endif
  #define TZ_USE_ESP32     true
  
  #if defined(TZ_USE_EEPROM)
    #undef TZ_USE_EEPROM
  #endif
  #define TZ_USE_EEPROM    false
  
  #ifndef USE_LITTLEFS
    #define USE_LITTLEFS   true
  #endif
  
  #if USE_LITTLEFS
    // Use LittleFS
    #include "FS.h"
    
    // Check cores/esp32/esp_arduino_version.h and cores/esp32/core_version.h
    //#if ( ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0) )  //(ESP_ARDUINO_VERSION_MAJOR >= 2)
    #if ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) )
      #if (_TZ_LOGLEVEL_ > 2)
        #warning Using ESP32 Core 1.0.6 or 2.0.0+
      #endif
      
      
      
      #include <LittleFS.h>
      #define FileFS   LittleFS
      
    #else
      #if (_TZ_LOGLEVEL_ > 2)
        #warning Using ESP32 Core 1.0.5-. You must install LITTLEFS library
      #endif
      
      // The library has been merged into esp32 core from release 1.0.6
      #include <LITTLEFS.h>             // https://github.com/lorol/LITTLEFS
      
      #define FileFS        LITTLEFS
    #endif

  #else
    // Use SPIFFS
   #include "FS.h"
    #include <SPIFFS.h>

    #define FileFS        SPIFFS
  #endif

//////////////////////////////////////////////////////////////

#elif defined(ESP8266)
  #if defined(TZ_USE_ESP8266)
    #undef TZ_USE_ESP8266
  #endif
  #define TZ_USE_ESP8266     true
  
  #if defined(TZ_USE_EEPROM)
    #undef TZ_USE_EEPROM
  #endif
  #define TZ_USE_EEPROM    false
  
  #ifndef USE_LITTLEFS
    #define USE_LITTLEFS   true
  #endif
  
  #include <FS.h>
  #include <LittleFS.h>
  
   #if USE_LITTLEFS   
    #define FileFS        LittleFS 
  #else
    #define FileFS        SPIFFS
  #endif

#elif ( defined(ARDUINO_SAM_DUE) || defined(__SAM3X8E__) )
  #warning Use Arduino_Sam_Due
#else
    #warning Use Unknown board and EEPROM
  #endif  

#ifndef TZ_DEBUG
  #define TZ_DEBUG       true
#endif

/*----------------------------------------------------------------------*
   Create a Timezone object from the given time change rules.
  ----------------------------------------------------------------------*/
Timezone::Timezone(const TimeChangeRule& dstStart, const TimeChangeRule& stdStart, uint32_t address)
  : m_dst(dstStart), m_std(stdStart), TZ_DATA_START(address)
{ 
  initStorage(address);
  initTimeChanges();
}

/*----------------------------------------------------------------------*
   Create a Timezone object for a zone that does not observe
   daylight time.
  ----------------------------------------------------------------------*/
Timezone::Timezone(const TimeChangeRule& stdTime, uint32_t address)
  : m_dst(stdTime), m_std(stdTime), TZ_DATA_START(address)
{ 
  // RoSchmi
  //initStorage(address);
  //initTimeChanges();
}

//////

/*----------------------------------------------------------------------*
   initialise (used where void constructor is called)  6v6gt
  ----------------------------------------------------------------------*/
void Timezone::init(const TimeChangeRule& dstStart, const TimeChangeRule& stdStart)
{
  //m_dst = dstStart;
  //m_std = stdStart;
  memcpy(&m_dst, &dstStart, sizeof(m_dst));
  memcpy(&m_std, &stdStart, sizeof(m_std));
}

// Allow a "blank" TZ object then use begin() method to set the actual TZ.
// Feature added by 6v6gt (https://forum.arduino.cc/index.php?topic=711259)
/*----------------------------------------------------------------------*
   Create a Timezone object from time change rules stored in EEPROM
   at the given address.
  ----------------------------------------------------------------------*/
Timezone::Timezone(uint32_t address)
{
  // RoSchmi
  //initStorage(address);
  
  //initTimeChanges();
  
  // RoSchmi readRules is modified, it might crash
  readRules();
}



/*----------------------------------------------------------------------*
   Calculate the DST and standard time change points for the given
   given year as local and UTC time_t values.
  ----------------------------------------------------------------------*/
void Timezone::calcTimeChanges(int yr)
{
  m_dstLoc = toTime_t(m_dst, yr);
  m_stdLoc = toTime_t(m_std, yr);
  m_dstUTC = m_dstLoc - m_std.offset * SECS_PER_MIN;
  m_stdUTC = m_stdLoc - m_dst.offset * SECS_PER_MIN;
}

/*----------------------------------------------------------------------*
   Determine whether the given UTC time_t is within the DST interval
   or the Standard time interval.
  ----------------------------------------------------------------------*/
bool Timezone::utcIsDST(const time_t& utc)
{
  // recalculate the time change points if needed
  if (year(utc) != year(m_dstUTC)) 
    calcTimeChanges(year(utc));

  if (m_stdUTC == m_dstUTC)       // daylight time not observed in this tz
    return false;
  else if (m_stdUTC > m_dstUTC)   // northern hemisphere
    return (utc >= m_dstUTC && utc < m_stdUTC);
  else                            // southern hemisphere
    return !(utc >= m_stdUTC && utc < m_dstUTC);
}

/*----------------------------------------------------------------------*
   Initialize the DST and standard time change points.
  ----------------------------------------------------------------------*/
void Timezone::initTimeChanges()
{
  m_dstLoc = 0;
  m_stdLoc = 0;
  m_dstUTC = 0;
  m_stdUTC = 0;
}

/*----------------------------------------------------------------------*
   Convert the given time change rule to a time_t value
   for the given year.
  ----------------------------------------------------------------------*/
time_t Timezone::toTime_t(const TimeChangeRule& r, int yr)
{
  uint8_t m = r.month;     // temp copies of r.month and r.week
  uint8_t w = r.week;
  
  if (w == 0)              // is this a "Last week" rule?
  {
    if (++m > 12)        // yes, for "Last", go to the next month
    {
      m = 1;
      ++yr;
    }
    
    w = 1;               // and treat as first week of next month, subtract 7 days later
  }

  // calculate first day of the month, or for "Last" rules, first day of the next month
  tmElements_t tm;
  
  tm.Hour   = r.hour;
  tm.Minute = 0;
  tm.Second = 0;
  tm.Day    = 1;
  tm.Month  = m;
  tm.Year   = yr - 1970;
  time_t t  = makeTime(tm);

  // add offset from the first of the month to r.dow, and offset for the given week
  t += ( (r.dow - weekday(t) + 7) % 7 + (w - 1) * 7 ) * SECS_PER_DAY;
  
  // back up a week if this is a "Last" rule
  if (r.week == 0) 
    t -= 7 * SECS_PER_DAY;
    
  return t;
}


/************************************************/



void Timezone::initStorage(uint32_t address)
{
  this->TZ_DATA_START = address;
  
#if TZ_USE_EEPROM
  EEPROM.begin();

  TZ_LOGDEBUG3("Read from EEPROM, size = ", TZ_EEPROM_SIZE, ", offset = ", TZ_DATA_START);

/////////////////////////////    
#elif TZ_USE_SAMD
  // Do something to init FlashStorage
  
/////////////////////////////    
#elif TZ_USE_STM32
  // Do something to init FlashStorage  
  

/////////////////////////////    
#elif TZ_USE_SAM_DUE
  // Do something to init DueFlashStorage

/////////////////////////////  
#elif TZ_USE_NRF52
  // Do something to init LittleFS / InternalFS
  // Initialize Internal File System
  InternalFS.begin();
  
/////////////////////////////    
#elif TZ_USE_ESP32
  // Do something to init LittleFS / InternalFS
  // Initialize Internal File System
  if (!FileFS.begin(true))
  {
    TZ_LOGDEBUG("SPIFFS/LittleFS failed! Already tried formatting.");
  
    if (!FileFS.begin())
    {     
      // prevents debug info from the library to hide err message.
      delay(100);
      
      TZ_LOGERROR("LittleFS failed!");
    }
  }
  
/////////////////////////////    
#elif TZ_USE_ESP8266
  // Do something to init LittleFS / InternalFS
  // Initialize Internal File System
  // Do something to init LittleFS / InternalFS
  // Initialize Internal File System
  FileFS.format();
   
  if (!FileFS.begin())
  {
    TZ_LOGDEBUG("SPIFFS/LittleFS failed! Already tried formatting.");
  
    if (!FileFS.begin())
    {     
      // prevents debug info from the library to hide err message.
      delay(100);
      
      TZ_LOGERROR("LittleFS failed!");
    }
  }
  
/////////////////////////////
#elif TZ_USE_RP2040

      bool beginOK = FileFS.begin();
  
      if (!beginOK)
      {
        TZ_LOGERROR("LittleFS error");
      }
  
/////////////////////////////

#elif TZ_USE_MBED_RP2040
 
      TZ_LOGDEBUG1("LittleFS size (KB) = ", RP2040_FS_SIZE_KB);
      
      int err = fs.mount(&bd);
      
      if (err)
      {       
        // Reformat if we can't mount the filesystem
        TZ_LOGERROR("LittleFS Mount Fail. Formatting... ");
 
        err = fs.reformat(&bd);
      }
      else
      {
        TZ_LOGDEBUG("LittleFS Mount OK");
      }
     
      if (err)
      {
        TZ_LOGERROR("LittleFS error");
      }

/////////////////////////////

#elif TZ_USE_MBED_PORTENTA

      if (blockDevicePtr != nullptr)
        return;

      // Get limits of the the internal flash of the microcontroller
      _flashIAPLimits = getFlashIAPLimits();
      
      TZ_LOGDEBUG1("Flash Size: (KB) = ", _flashIAPLimits.flash_size / 1024.0);
      TZ_HEXLOGDEBUG1("FlashIAP Start Address: = 0x", _flashIAPLimits.start_address);
      TZ_LOGDEBUG1("LittleFS size (KB) = ", _flashIAPLimits.available_size / 1024.0);
      
      blockDevicePtr = new FlashIAPBlockDevice(_flashIAPLimits.start_address, _flashIAPLimits.available_size);
      
      if (!blockDevicePtr)
      {    
        TZ_LOGERROR("Error init FlashIAPBlockDevice");

        return;
      }
      
  #if FORCE_REFORMAT
      fs.reformat(blockDevicePtr);
  #endif

      int err = fs.mount(blockDevicePtr);
      
      TZ_LOGDEBUG(err ? "LittleFS Mount Fail" : "LittleFS Mount OK");
  
      if (err)
      {
        // Reformat if we can't mount the filesystem
        TZ_LOGDEBUG("Formatting... ");
  
        err = fs.reformat(blockDevicePtr);
      }
  
      bool beginOK = (err == 0);
  
      if (!beginOK)
      {
        TZ_LOGERROR("\nLittleFS error");
      }
      
/////////////////////////////  
 
#elif TZ_USE_RTL8720
  // Do something to init FlashStorage_RTL8720
    
/////////////////////////////  
#else
  #error Un-identifiable board selected. Please check your Tools->Board setting.
#endif
/////////////////////////////  

  storageSystemInit = true;
}



/************************************************/




/*----------------------------------------------------------------------*
   Convert the given UTC time to local time, standard or
   daylight time, as appropriate.
  ----------------------------------------------------------------------*/
time_t Timezone::toLocal(const time_t& utc)
{
  // recalculate the time change points if needed
  if (year(utc) != year(m_dstUTC)) 
    calcTimeChanges(year(utc));

  if (utcIsDST(utc))
    return utc + m_dst.offset * SECS_PER_MIN;
  else
    return utc + m_std.offset * SECS_PER_MIN;
}
/*----------------------------------------------------------------------*
   Read or update the daylight and standard time rules from RAM.
  ----------------------------------------------------------------------*/
void Timezone::setRules(const TimeChangeRule& dstStart, const TimeChangeRule& stdStart)
{
  m_dst = dstStart;
  m_std = stdStart;
  initTimeChanges();  // force calcTimeChanges() at next conversion call
}

void Timezone::display_DST_Rule()
{
  //TZ_LOGERROR("DST rule");
  //TZ_LOGERROR3("abbrev :",  m_dst.abbrev, ", week :",   m_dst.week);
  //TZ_LOGERROR3("dow :",     m_dst.dow,    ", month :",  m_dst.month);
  //TZ_LOGERROR3("hour :",    m_dst.hour,   ", offset :", m_dst.offset);
}

void Timezone::display_STD_Rule()
{
  //TZ_LOGERROR("DST rule");
  //TZ_LOGERROR3("abbrev :",  m_std.abbrev, ", week :",   m_std.week);
  //TZ_LOGERROR3("dow :",     m_std.dow,    ", month :",  m_std.month);
  //TZ_LOGERROR3("hour :",    m_std.hour,   ", offset :", m_std.offset);
}


/*----------------------------------------------------------------------*
   Read the daylight and standard time rules from EEPROM/storage at
   the TZ_DATA_START offset.
  ----------------------------------------------------------------------*/
void Timezone::readRules()
{
  // RoSchmi: The following lines make the code crash
     readTZData();
     initTimeChanges();  // force calcTimeChanges() at next conversion call
}

/*----------------------------------------------------------------------*
   Write the daylight and standard time rules to EEPROM/storage at
   the TZ_DATA_START offset.
  ----------------------------------------------------------------------*/
void Timezone::writeRules(int address)
{
  this->TZ_DATA_START = address;
  
  writeTZData(address);
  initTimeChanges();  // force calcTimeChanges() at next conversion call
}


#if (TZ_USE_ESP32)

#if USE_LITTLEFS
  #warning Using ESP32 LittleFS in Timezone_Generic
#else
  #warning Using ESP32 SPIFFS in Timezone_Generic
#endif

 // ESP32 code    
/*----------------------------------------------------------------------*
   Read the daylight and standard time rules from LittleFS at
   the given offset.
  ----------------------------------------------------------------------*/
void Timezone::readTZData()
{

  // This code will crash the application, why ?
  /*
  if (!storageSystemInit)
  {
    // Format SPIFFS/LittleFS if not yet
    if (!FileFS.begin(true))
    {
      TZ_LOGERROR(F("SPIFFS/LittleFS failed! Formatting."));
      
      if (!FileFS.begin())
      {
        TZ_LOGERROR(F("SPIFFS/LittleFS failed! Pls use EEPROM."));
        return;
      }
    }    
    storageSystemInit = true;
    
  }
  */
  // ESP32 code
  File file = FileFS.open(TZ_FILENAME, "r");
  
  TZ_LOGDEBUG3(F("Reading m_dst & m_std from TZ_file :"), TZ_FILENAME, F(", data offset ="), TZ_DATA_OFFSET);
  
  if (file)
  {
    memset(&m_dst, 0, TZ_DATA_SIZE);
    memset(&m_std, 0, TZ_DATA_SIZE);
    
    file.seek(TZ_DATA_OFFSET);
    
    file.readBytes((char *) &m_dst, TZ_DATA_SIZE);
    
    // Seek to be sure
    file.seek(TZ_DATA_OFFSET + TZ_DATA_SIZE);
    file.readBytes((char *) &m_std, TZ_DATA_SIZE);

    TZ_LOGDEBUG(F("Reading from TZ_file OK"));

    file.close(); 
  }
  else
  {
    TZ_LOGERROR(F("Reading from TZ_file failed"));
  }
  
}

/*----------------------------------------------------------------------*
   Write the daylight and standard time rules to LittleFS at
   the given offset.
  ----------------------------------------------------------------------*/
void Timezone::writeTZData(int address)
{ 
  (void) address;
  
  if (!storageSystemInit)
  {
    // Format SPIFFS/LittleFS if not yet
    if (!FileFS.begin(true))
    {
      TZ_LOGERROR(F("SPIFFS/LittleFS failed! Formatting."));
      
      if (!FileFS.begin())
      {
        TZ_LOGERROR(F("SPIFFS/LittleFS failed!"));
        return;
      }
    }
    
    storageSystemInit = true;
  }
  // ESP32 code
  File file = FileFS.open(TZ_FILENAME, "w");

  TZ_LOGDEBUG3(F("Saving m_dst & m_std to TZ_file :"), TZ_FILENAME, F(", data offset ="), TZ_DATA_OFFSET);

  if (file)
  {
    file.seek(TZ_DATA_OFFSET);
    file.write((uint8_t *) &m_dst, TZ_DATA_SIZE);

    // Seek to be sure
    file.seek(TZ_DATA_OFFSET + TZ_DATA_SIZE);
    file.write((uint8_t *) &m_std, TZ_DATA_SIZE);
    
    file.close();

    TZ_LOGDEBUG("Saving to TZ_file OK");
  }
  else
  {
    TZ_LOGERROR("Saving to TZ_file failed");
  }
    
}

/////////////////////////////////////////////

#elif (TZ_USE_ESP8266)
  
  #if USE_LITTLEFS
  #warning Using ESP8266 LittleFS in Timezone_Generic
#else
  #warning Using ESP8266 SPIFFS in Timezone_Generic
#endif

  // ESP8266 code    
/*----------------------------------------------------------------------*
   Read the daylight and standard time rules from LittleFS at
   the given offset.
  ----------------------------------------------------------------------*/
void Timezone::readTZData()
{
  if (!storageSystemInit)
  {
    // Format SPIFFS/LittleFS if not yet
    if (!FileFS.begin())
    {
      TZ_LOGERROR(F("SPIFFS/LittleFS failed! Formatting."));
      FileFS.format();
      
      if (!FileFS.begin())
      {
        TZ_LOGERROR(F("SPIFFS/LittleFS failed"));
        return;
      }
    }
    
    storageSystemInit = true;
  }
  
  // ESP8266 code
  File file = FileFS.open(TZ_FILENAME, "r");
  
  TZ_LOGDEBUG3(F("Reading m_dst & m_std from TZ_file :"), TZ_FILENAME, F(", data offset ="), TZ_DATA_OFFSET);

  if (file)
  {
    memset(&m_dst, 0, TZ_DATA_SIZE);
    memset(&m_std, 0, TZ_DATA_SIZE);
    
    file.seek(TZ_DATA_OFFSET);
    
    file.readBytes((char *) &m_dst, TZ_DATA_SIZE);
    
    // Seek to be sure
    file.seek(TZ_DATA_OFFSET + TZ_DATA_SIZE);
    file.readBytes((char *) &m_std, TZ_DATA_SIZE);

    TZ_LOGDEBUG(F("Reading from TZ_file OK"));

    file.close(); 
  }
  else
  {
    TZ_LOGERROR(F("Reading from TZ_file failed"));
  }
}

/*----------------------------------------------------------------------*
   Write the daylight and standard time rules to LittleFS at
   the given offset.
  ----------------------------------------------------------------------*/
void Timezone::writeTZData(int address)
{ 
  (void) address;
  
  if (!storageSystemInit)
  {
    // Format SPIFFS/LittleFS if not yet
    if (!FileFS.begin())
    {
      TZ_LOGERROR(F("SPIFFS/LittleFS failed! Formatting."));
      FileFS.format();
      
      if (!FileFS.begin())
      {
        TZ_LOGERROR(F("SPIFFS/LittleFS failed! Pls use EEPROM."));
        return;
      }
    }
    
    storageSystemInit = true;
  }
  
  // ESP8266 code
  File file = FileFS.open(TZ_FILENAME, "w");

  TZ_LOGDEBUG3(F("Saving m_dst & m_std to TZ_file :"), TZ_FILENAME, F(", data offset ="), TZ_DATA_OFFSET);

  if (file)
  {
    file.seek(TZ_DATA_OFFSET);
    file.write((uint8_t *) &m_dst, TZ_DATA_SIZE);

    // Seek to be sure
    file.seek(TZ_DATA_OFFSET + TZ_DATA_SIZE);
    file.write((uint8_t *) &m_std, TZ_DATA_SIZE);
    
    file.close();

    TZ_LOGDEBUG("Saving to TZ_file OK");
  }
  else
  {
    TZ_LOGERROR("Saving to TZ_file failed");
  }   
}

#endif
#endif    // TIMEZONE_GENERIC