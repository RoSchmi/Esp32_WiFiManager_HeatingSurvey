// Copyright of template
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Copyright (c) RoSchmi. All rights reserved.
// SPDX-License-Identifier: MIT

// The code of this file provides Esp32 specific solutions for the 'generic' function calls from
// the azure-sdk-for-c

// These functions are:
//    az_http_client_send_request(...);
//    az_platform_clock_msec(...);
//    az_platform_sleep_msec(...)


#include <az_esp32_roschmi.h>
#include "config.h"


HTTPClient *  devHttp = NULL;
WiFiClient * devWifiClient = NULL;  

const char * _caCertificate;

const char * PROGMEM mess1 = "-1 Connection refused\r\n\0";
const char * PROGMEM mess2 = "-2 Send Header failed\r\n\0";
const char * PROGMEM mess3 = "-3 Send Payload failed\r\n\0";
const char * PROGMEM mess4 = "-4 Not connected\r\n\0";
const char * PROGMEM mess5 = "-5 Connection lost\r\n\0";
const char * PROGMEM mess6 = "-6 No Stream\r\n\0";
const char * PROGMEM mess7 = "-7 No Http Server\r\n\0";
const char * PROGMEM mess8 = "-8 Too less Ram\r\n\0";
const char * PROGMEM mess9 = "-9 Error Encoding\r\n\0";
const char * PROGMEM mess10 = "-10 Stream Write\r\n\0";
const char * PROGMEM mess11 = "-11 Read Timeout\r\n\0";
const char * PROGMEM mess12 = "-12 unspecified\r\n\0";



/**
 * @brief uses AZ_HTTP_BUILDER to set up request and perform it.
 *
 * @param request an internal http builder with data to build and send http request
 * @param ref_response pre-allocated buffer where http response will be written
 * @return az_result
 */
AZ_NODISCARD az_result
az_http_client_send_request(az_http_request const* request, az_http_response* ref_response)
{
  _az_PRECONDITION_NOT_NULL(request);
  _az_PRECONDITION_NOT_NULL(ref_response);

   //Serial.println(F("az_http_client_send_request"));

  // Working with spans
  //https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/core#working-with-spans
  
  az_http_method requMethod = request->_internal.method;
  int32_t max_header_count = request->_internal.max_headers;
  size_t headerCount = az_http_request_headers_count(request);

  // Code to copy all headers into one string, actually not needed
  /*
  uint8_t headers_buffer[300] {0};
  az_span headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);
  az_result myResult = dev_az_http_client_build_headers(request, headers_span);
  char* theHeader_str = (char*) az_span_ptr(headers_span);
  volatile size_t headerSize = strlen(theHeader_str);
  */
    
  //az_span_to_str(char* destination, int32_t destination_max_size, az_span source);
    
  az_span urlWorkCopy = request->_internal.url;

  int32_t colonIndex = az_span_find(urlWorkCopy, AZ_SPAN_LITERAL_FROM_STR(":"));
		
  char protocol[6] {0};
  urlWorkCopy = request->_internal.url;
  
  /* bool protocolIsHttpOrHttps = false; */
  int32_t slashIndex = - 1;
  if (colonIndex != -1)
  {
    az_span_to_str(protocol, 6, az_span_slice(urlWorkCopy, 0, colonIndex));
    if ((strcmp(protocol, (const char *)"https") == 0) || (strcmp(protocol, (const char *)"http") == 0))
    {
      /* protocolIsHttpOrHttps = true; */
    }

    slashIndex = az_span_find(az_span_slice_to_end(urlWorkCopy, colonIndex + 3), AZ_SPAN_LITERAL_FROM_STR("/"));
        
    slashIndex = (slashIndex != -1) ? slashIndex + colonIndex + 3 : -1;       
  }
    
  char workBuffer[100] {0};
    
  if (slashIndex == -1)
  {
    az_span_to_str(workBuffer, sizeof(workBuffer), az_span_slice_to_end(urlWorkCopy, colonIndex + 3));
  }
  else
  {
    az_span_to_str(workBuffer, sizeof(workBuffer), az_span_slice_to_end(az_span_slice(urlWorkCopy, 0, slashIndex), colonIndex + 3));
  }

 
  String host = (const char *)workBuffer;

  if (slashIndex != -1)
  {
    memset(workBuffer, 0, sizeof(workBuffer));
    az_span_to_str(workBuffer, sizeof(workBuffer), az_span_slice_to_end(urlWorkCopy, slashIndex));
  }
  String resource = slashIndex != -1 ? (char *)workBuffer : "";
  
  uint16_t port = (strcmp(protocol, (char *)"http") == 0) ? 80 : 443;
  

  devHttp->setReuse(false);

  
  if (port == 80)      // http ?
  { 
      devHttp->begin(* devWifiClient, host, port, resource, false);    
  }
  else                 // https
  {
    devHttp->begin(* devWifiClient, host, port, resource, true);
  }
    
  char name_buffer[MAX_HEADERNAME_LENGTH +2] {0};
  char value_buffer[MAX_HEADERVALUE_LENGTH +2] {0};
  az_span head_name = AZ_SPAN_FROM_BUFFER(name_buffer);
  az_span head_value = AZ_SPAN_FROM_BUFFER(value_buffer);
    
  
  char strInit[2] {0};
  String nameString = (char *)strInit;
  String valueString = (char *)strInit;

  // transfere header to HTTPClient
  for (int32_t offset = (headerCount - 1); offset >= 0; offset--)
  {
    _az_RETURN_IF_FAILED(az_http_request_get_header(request, offset, &head_name, &head_value));

      // For debugging, print header
      /*
      // Serial.println(offset);
      char buf[50] {0};
      az_span_to_str(buf, 49, head_name);
      Serial.printf((char *) buf);
      Serial.print(" : ");
      az_span_to_str(buf, 49, head_value);
      Serial.println((char *)buf);
      */
   
    az_span_to_str((char *)name_buffer, MAX_HEADERNAME_LENGTH -1, head_name);
    az_span_to_str((char *)value_buffer, MAX_HEADERVALUE_LENGTH -1, head_value);
    nameString = (char *)name_buffer;
    valueString = (char *)value_buffer;

    devHttp->addHeader(nameString, valueString, true, true);    
  }

  uint8_t * theBody = request->_internal.body._internal.ptr;

  if (az_span_is_content_equal(requMethod, AZ_SPAN_LITERAL_FROM_STR("POST")))
  {      
    const char * headerKeys[] = {"ETag", "Date", "x-ms-request-id", "x-ms-version", "Content-Type"};       
    devHttp->collectHeaders(headerKeys, 5);
      
    int httpCode = -1;

    /*  Give information about stack size
    UBaseType_t  watermarkEntityInsert_1 = uxTaskGetStackHighWaterMark(NULL);
    Serial.print(F("Watermark for core_1 before POST is: "));
    Serial.println(watermarkEntityInsert_1);
    */

    //void* pPost = NULL;
    //Serial.printf("\r\nFree Stack before POST is: %d \r\n", (uint32_t)&pPost - 0x3ffb0050);
  
    httpCode = devHttp->POST((char *)theBody);
 
    delay(1);
             
    int indexCtr = 0;
    int pageWidth = 50;
        
    
        
    az_result appendResult;
    char httpStatusLine[40] {0};
    if (httpCode > 0)  // Request was successful
    {      
          sprintf((char *)httpStatusLine, "%s%i%s", "HTTP/1.1 ", httpCode, " ***\r\n");

          #if SERIAL_PRINT == 1
          Serial.println(httpStatusLine);
          #endif

          appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)httpStatusLine));

          size_t respHeaderCount = devHttp->headers();

          //Serial.printf("Header Count: %d", respHeaderCount);

          for (size_t i = 0; i < respHeaderCount; i++)
          {       
            appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)devHttp->headerName(i).c_str()));          
            appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)": "));
            appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)devHttp->header(i).c_str()));
            appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)"\r\n"));
          }
          appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)"\r\n"));
          appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)devHttp->getString().c_str()));
        }
        else
        {
          int httpCodeCopy = httpCode;
          char messageBuffer[30] {0};
          
          // Hack: Convert negative return codes from post request into http codes 401 - 412, so that they can be handeled
          // (returned) as az_http_status_code
                 
          switch (httpCodeCopy)
          {
            case -1: {
              httpCode = 401;
              strcpy(messageBuffer, mess1);
              break;
            }
            case -2: {
              httpCode = 402;
              strcpy(messageBuffer, mess2);
              break;
            }
            case -3: {
              httpCode = 403;
              strcpy(messageBuffer, mess3);
              break;
            }
            case -4: {
              httpCode = 404;
              strcpy(messageBuffer, mess4);
              break;
            }
            case -5: {
              httpCode = 405;
              strcpy(messageBuffer, mess5);
              break;
            }
            case -6: {
              httpCode = 406;
              strcpy(messageBuffer, mess6);
              break;
            }
            case -7: {
              httpCode = 407;
              strcpy(messageBuffer, mess7);
              break;
            }
            case -8: {
              httpCode = 408;
              strcpy(messageBuffer, mess8);
              break;
            }
            case -9: {
              httpCode = 409;
              strcpy(messageBuffer, mess9);
              break;
            }
            case -10: {
              httpCode = 410;
              strcpy(messageBuffer, mess10);
              break;
            }
            case -11: {
              httpCode = 411;
              strcpy(messageBuffer, mess11);
              break;
            }       
            default: {
              httpCode = 412;
              strcpy(messageBuffer, mess12);
            }
          }     
          
          // Request failed because of internal http-client error
          sprintf((char *)httpStatusLine, "%s%i%s%i%s", "HTTP/1.1 ", httpCode, " Http-Client error ", httpCode, " \r\n");
          appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)httpStatusLine));
          appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)"\r\n"));
          appendResult = az_http_response_append(ref_response, az_span_create_from_str((char *)messageBuffer));
        }

        devHttp->end();
        
        // For debugging
        /*
        char buffer[1000];
        az_span content = AZ_SPAN_FROM_BUFFER(buffer);

        az_http_response_get_body(ref_response, &content);

        volatile int dummy349 = 1;
        */

        //  Here you can see the received payload in chunks 
        // if you set a breakpoint in the loop 
        
        /*
        String partMessage;
        while (indexCtr < length)
        {
          partMessage = payload.substring(indexCtr, indexCtr + pageWidth);
          indexCtr += pageWidth;
        } 
        */
    }
    else
    {

      Serial.println("Before GET routine");
    while (true)
   {
     Serial.println("Halted in Loop");
     delay(5000);
   }   


      if (az_span_is_content_equal(requMethod, AZ_SPAN_LITERAL_FROM_STR("GET")))
      {
        // Not used
      }
      else
      {
        // Not used
      }
      
    }
    
    
    //delay(500);
   return AZ_OK;
}

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  
  int64_t clPerMico = clockCyclesPerMicrosecond() * 1000;
  out_clock_msec = &clPerMico;
  return AZ_OK;
} 

// For Arduino (Wio Terminal):
AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds) 
{ 
  delay(milliseconds);
  return AZ_OK;
}

void setHttpClient(HTTPClient * httpClient)
{
    devHttp = httpClient;
}

void setWiFiClient(WiFiClient * wifiClient)
{
    devWifiClient = wifiClient;
}

void setCaCert(const char * caCert)
{
  _caCertificate = caCert;
}

/**
 * @brief loop all the headers from a HTTP request and combine all headers into one az_span
 *
 * @param request an http builder request reference
 * @param ref_headers list of headers
 * @return az_result
 */
/*
static AZ_NODISCARD az_result
dev_az_http_client_build_headers(az_http_request const* request, az_span ref_headers)
{
  _az_PRECONDITION_NOT_NULL(request);
  
  az_span header_name = { 0 };
  az_span header_value = { 0 };
  uint8_t header_buffer[request->_internal.url_length - 20 + 60] {0};
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buffer);
  az_span separator = AZ_SPAN_LITERAL_FROM_STR(": ");
  

  for (int32_t offset = 0; offset < az_http_request_headers_count(request); ++offset)
  {
    _az_RETURN_IF_FAILED(az_http_request_get_header(request, offset, &header_name, &header_value));   
    _az_RETURN_IF_FAILED(dev_az_span_append_header_to_buffer(header_span, header_name, header_value, separator));
    char* header_str = (char*) az_span_ptr(header_span); // str points to a 0-terminated string
    ref_headers = az_span_copy(ref_headers, az_span_create_from_str(header_str));
    ref_headers = az_span_copy(ref_headers, AZ_SPAN_LITERAL_FROM_STR("\r\n"));
  }
  az_span_copy_u8(ref_headers, 0);
  return AZ_OK;
}
*/

/**
 * @brief writes a header key and value to a buffer as a 0-terminated string and using a separator
 * span in between. Returns error as soon as any of the write operations fails
 *
 * @param writable_buffer pre allocated buffer that will be used to hold header key and value
 * @param header_name header name
 * @param header_value header value
 * @param separator symbol to be used between key and value
 * @return az_result
 */

/*
static AZ_NODISCARD az_result dev_az_span_append_header_to_buffer(
    az_span writable_buffer,
    az_span header_name,
    az_span header_value,
    az_span separator)
{
  int32_t required_length
      = az_span_size(header_name) + az_span_size(separator) + az_span_size(header_value) + 1;

  _az_RETURN_IF_NOT_ENOUGH_SIZE(writable_buffer, required_length);

  writable_buffer = az_span_copy(writable_buffer, header_name);
  writable_buffer = az_span_copy(writable_buffer, separator);
  writable_buffer = az_span_copy(writable_buffer, header_value);
  az_span_copy_u8(writable_buffer, 0);
  return AZ_OK;
}
*/