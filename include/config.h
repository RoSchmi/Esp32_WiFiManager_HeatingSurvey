#include <Arduino.h>

#ifndef _CONFIG_H
#define _CONFIG_H

// This file is for 'not secret' user specific configurations
//
// Please set your timezone offset (time difference from your zone 
// to UTC time in units of minutes) and set the time difference
// used for DaylightSaving Time in minutes
// Define begin and end of Daylightsaving 
//
// Please select the transport protocol, http or https (https is recommended)
// For https you must include the Root Certificate of your Azure Account
// like here the baltimore_root_ca
// Select the Sendinterval in minutes
// Select the Invalidate Interval in minutes (Sensor values not actualized
// within this interval are considered to be invalid)
// Define other settings according to your needs

//
// The credentials of your WiFi router and the name and key of your
// Azure Storage Account have to be set in the file config_secret.h 

#define SOUNDSWITCHER_THRESHOLD 250         // The Threshold to toggle High/Low
#define SOUNDSWITCHER_UPDATEINTERVAL 400    // Interval in ms for reading sound level
#define SOUNDSWITCHER_READ_DELAYTIME 4000   // Delay in ms from switch to displayed value


#define SENDINTERVAL_MINUTES   5    // Sendinterval in minutes (10 is recommended), in this interval                                        
                                      // data are sent to the Cloud (is limited to be not below 1 second)

#define SERIAL_PRINT 1                     // 1 = yes, 0 = no. Select if Serial.print messages are printed 
                                           

// Names for Tables in Azure Account, please obey rules for Azure Tablenames (e.g. no underscore allowed)
// regular expression "^[A-Za-z][A-Za-z0-9]{2,62}$".
// max length in this App is 45

#define ANALOG_TABLENAME "AnalogValuesXY"          // Name of the Azure Table to store 4 analog Values max length = 45

#define ANALOG_TABLE_PART_PREFIX "Y2_"            // Prefix for PartitionKey of Analog Tables (default, no need to change)


                                               // Names of tables to be created in your Azure Storage Account
                                               // Per default the names are augmented with the actual year in this App
#define ON_OFF_TABLENAME_01 "OnOffx01x"          // Name of the 1. On/Off Table  max length = 45
#define ON_OFF_TABLENAME_02 "OnOffx02x"          // Name of the 2. On/Off Table  max length = 45
#define ON_OFF_TABLENAME_03 "OnOffx03x"          // Name of the 3. On/Off Table  max length = 45
#define ON_OFF_TABLENAME_04 "OnOffx04x"          // Name of the 4. On/Off Table  max length = 45

#define ON_OFF_TABLE_PART_PREFIX "Y3_"           // Prefix for PartitionKey of On/Off Tables (default, only change if needed)

#define INVALIDATEINTERVAL_MINUTES 10      // Invalidateinterval in minutes 
                                           // (limited to values between 1 - 60)
                                           // (Sensor readings are considered to be invalid if not successsfully
                                           // read within this timespan)

#define NTP_UPDATE_INTERVAL_MINUTES 140000     //  With this interval sytem time is updated via NTP
                                           //  with internet time (is limited to be not below 1 min)

#define UPDATE_TIME_FROM_AZURE_RESPONSE 1  // 1 = yes, 0 = no. SystemTime is updated from the Post response from Azure.
                                           // With this option set, you can set  NTP_UPDATE_INTERVAL_MINUTES to a very
                                           // long time, so that it 'never' happens                                      

#define ANALOG_SENSOR_READ_INTERVAL_SECONDS 0   // Analog sensors are read with this interval (seconds, can be 0)
                                                // Must be 0 in Applikation Esp32_WiFiManager_HeatingSurvey                                  

#define WORK_WITH_WATCHDOG 0              // 1 = yes, 0 = no, Watchdog is used (1) or not used (0)
                                           // should be 1 for normal operation and 0 for testing
                                           
#define REBOOT_AFTER_FAILED_UPLOAD 0       // 1 = yes, 0 = no
                                           // should be 1 for normal operation and 0 for testing
                                            
// Set timezoneoffset and daylightsavingtime settings according to your zone
// https://en.wikipedia.org/wiki/Daylight_saving_time_by_country
// https://en.wikipedia.org/wiki/List_of_time_zone_abbreviations

#define TIMEZONEOFFSET 60           // TimeZone time difference to UTC in minutes
#define DSTOFFSET 60                // Additional DaylightSaving Time offset in minutes

#define  DST_ON_NAME                "CEST"
#define  DST_START_MONTH            "Mar"    // Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov or Dec
#define  DST_START_WEEKDAY          "Sun"    // Sun, Mon, Tue, Wed, Thu, Fri, Sat
#define  DST_START_WEEK_OF_MONTH    "Fourth"   // Last, First, Second, Third, Fourth, 
#define  DST_START_HOUR              2        // 0 - 23

#define  DST_OFF_NAME               "CET"
#define  DST_STOP_MONTH             "Oct"    // Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov or Dec
#define  DST_STOP_WEEKDAY           "Sun"    // Sun, Tue, Wed, Thu, Fri, Sat
#define  DST_STOP_WEEK_OF_MONTH     "Last"   // Last, First, Second, Third, Fourth
#define  DST_STOP_HOUR               3       // 0 - 23
       

#define TRANSPORT_PROTOCOL 1         // 0 = http, 1 = https
                                 

#define USE_STATIC_IP 0                // 1 = use static IpAddress, 0 = use DHCP
                                        // for static IP: Ip-addresses have to be set in the code

#define MIN_DATAVALUE -40.0             // Values below are treated as invalid
#define MAX_DATAVALUE 140.0             // Values above are treated as invalid
#define MAGIC_NUMBER_INVALID 999.9      // Invalid values are replaced with this value (should be 999.9)
                                        // Not sure if it works with other values than 999.9

//#define USE_SIMULATED_SENSORVALUES      // Activates simulated sensor values (sinus curve) or (test values)
//#define USE_TEST_VALUES                 // Activates sending of test values, e.g. counter or last reset cause (see Code in main.cpp)
                                        // if activated we select test values, not sinus curves

#define SENSOR_1_OFFSET     0.0        // Calibration Offset to sensor No 1
#define SENSOR_2_OFFSET     0.0        // Calibration Offset to sensor No 2
#define SENSOR_3_OFFSET     0.0        // Calibration Offset to sensor No 3
#define SENSOR_4_OFFSET     0.0        // Calibration Offset to sensor No 4

// Not needed in version for Esp32
#define SENSOR_1_FAHRENHEIT 0         // 1 = yes, 0 = no - Display in Fahrenheit scale
#define SENSOR_2_FAHRENHEIT 0         // 1 = yes, 0 = no - Display in Fahrenheit scale
#define SENSOR_3_FAHRENHEIT 0         // 1 = yes, 0 = no - Display in Fahrenheit scale
#define SENSOR_4_FAHRENHEIT 0         // 1 = yes, 0 = no - Display in Fahrenheit scale



const char *baltimore_root_ca =
"-----BEGIN CERTIFICATE-----\n"
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
"-----END CERTIFICATE-----";


#endif // _CONFIG_H