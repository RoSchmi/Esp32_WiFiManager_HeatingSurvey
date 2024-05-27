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

#define TZ_USE_EEPROM      true

/////////////////////////////

// To eliminate warnings with [-Wundef]
#define TZ_USE_ESP32        false
#define TZ_USE_ESP8266      false
#define TZ_USE_SAMD         false
#define TZ_USE_SAM_DUE      false
#define TZ_USE_NRF52        false
#define TZ_USE_STM32        false
#define TZ_USE_RP2040       false
#define TZ_USE_MBED_RP2040  false

/*----------------------------------------------------------------------*
   Create a Timezone object from the given time change rules.
  ----------------------------------------------------------------------*/
Timezone::Timezone(const TimeChangeRule& dstStart, const TimeChangeRule& stdStart, uint32_t address)
  : m_dst(dstStart), m_std(stdStart), TZ_DATA_START(address)
{ 
  //initStorage(address);
  //initTimeChanges();
}

/*----------------------------------------------------------------------*
   Create a Timezone object for a zone that does not observe
   daylight time.
  ----------------------------------------------------------------------*/
Timezone::Timezone(const TimeChangeRule& stdTime, uint32_t address)
  : m_dst(stdTime), m_std(stdTime), TZ_DATA_START(address)
{ 
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
  //initStorage(address);
  
  //initTimeChanges();

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
  //readTZData();
  //initTimeChanges();  // force calcTimeChanges() at next conversion call
}

/*----------------------------------------------------------------------*
   Write the daylight and standard time rules to EEPROM/storage at
   the TZ_DATA_START offset.
  ----------------------------------------------------------------------*/
void Timezone::writeRules(int address)
{
  this->TZ_DATA_START = address;
  
  writeTZData(address);
 // initTimeChanges();  // force calcTimeChanges() at next conversion call
}

#endif    // TIMEZONE_GENERIC