// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// The code of this files provided the following functions which are called by TableClient.cpp to set up
// a request and receive data from the response
//
// az_storage_tables_client_init(...);
// az_storage_tables_client_options_default(...);
// az_result az_storage_tables_upload(...);

#include <roschmi_az_storage_tables.h>

#include <stddef.h>

#include <Arduino.h>

enum
{
  _az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE = 14 * sizeof(_az_http_request_header),
};

static az_span const AZ_STORAGE_TABLES_HEADER_ACCEPT_TYPE
    = AZ_SPAN_LITERAL_FROM_STR("Accept");  

__unused static az_span const AZ_STORAGE_TABLES_HEADER_EXPECT_TYPE
    = AZ_SPAN_LITERAL_FROM_STR("Expect");

__unused  static az_span const AZ_STORAGE_TABLES_HEADER_EXPECT_100_CONTINUE
    = AZ_SPAN_LITERAL_FROM_STR("100-continue");  
 
static az_span const AZ_STORAGE_TABLES_HEADER_XMS_DATE
    = AZ_SPAN_LITERAL_FROM_STR("x-ms-date");

__unused  static az_span const AZ_STORAGE_TABLES_HEADER_API_VERSION
    = AZ_SPAN_LITERAL_FROM_STR("x-ms-version");

static az_span const AZ_STORAGE_TABLES_HEADER_AUTHORIZATION
    = AZ_SPAN_LITERAL_FROM_STR("Authorization");

static az_span const AZ_STORAGE_TABLES_HEADER_CONTENT_MD5
    = AZ_SPAN_LITERAL_FROM_STR("Content-MD5");

static az_span const AZ_STORAGE_TABLES_HEADER_PREFERE
    = AZ_SPAN_LITERAL_FROM_STR("Prefer");

static az_span const AZ_STORAGE_TABLES_HEADER_ACCEPT_CHARSET
    = AZ_SPAN_LITERAL_FROM_STR("Accept-Charset");

static az_span const AZ_STORAGE_TABLES_CHARSET_UTF8 = AZ_SPAN_LITERAL_FROM_STR("UTF-8");

static az_span const AZ_STORAGE_TABLES_HEADER_DATASERVICE_VERSION
    = AZ_SPAN_LITERAL_FROM_STR("DataServiceVersion");

static az_span const AZ_STORAGE_TABLES_DATASERVICE_VERS_3_0 = AZ_SPAN_LITERAL_FROM_STR("3.0");

static az_span const AZ_STORAGE_TABLES_HEADER_MAX_DATASERVICE_VERSION
    = AZ_SPAN_LITERAL_FROM_STR("MaxDataServiceVersion");

static az_span const AZ_STORAGE_TABLES_MAX_DATASERVICE_VERS_3_0_NETFX = AZ_SPAN_LITERAL_FROM_STR("3.0;NetFx");

static az_span const AZ_HTTP_HEADER_CONTENT_LENGTH = AZ_SPAN_LITERAL_FROM_STR("Content-Length");
static az_span const AZ_HTTP_HEADER_CONTENT_TYPE = AZ_SPAN_LITERAL_FROM_STR("Content-Type");

static az_span const AZ_HTTP_HEADER_ACCEPT_ENCODING
= AZ_SPAN_LITERAL_FROM_STR("Accept-Encoding");

__unused  static az_span const AZ_HTTP_ACCEPT_ENCODING_IDENTITY 
= AZ_SPAN_LITERAL_FROM_STR("identity");

static az_span const AZ_HTTP_ACCEPT_ENCODING_CHUNKED
    = AZ_SPAN_LITERAL_FROM_STR("chunked"); 

__unused  static az_span const AZ_HTTP_HEADER_CONNECTION
    = AZ_SPAN_LITERAL_FROM_STR("Connection");

__unused static az_span const AZ_HTTP_CONNECTION_CLOSE = AZ_SPAN_LITERAL_FROM_STR("Close");

__unused static az_span const AZ_HTTP_HEADER_HOST
    = AZ_SPAN_LITERAL_FROM_STR("Host");

AZ_NODISCARD az_storage_tables_client_options az_storage_tables_client_options_default()
{
  az_storage_tables_client_options options = (az_storage_tables_client_options) {
    ._internal = {
      .api_version = { 
        ._internal = { 
          .option_location = _az_http_policy_apiversion_option_location_header,
          .name = AZ_SPAN_FROM_STR("x-ms-version"),
          .version = AZ_STORAGE_API_VERSION,
        },
      },
      .telemetry_options = _az_http_policy_telemetry_options_default(),
    },
    .retry_options = _az_http_policy_retry_options_default(),
  };

  options.retry_options.max_retries = 5;
  options.retry_options.retry_delay_msec = 1 * _az_TIME_MILLISECONDS_PER_SECOND;
  options.retry_options.max_retry_delay_msec = 30 * _az_TIME_MILLISECONDS_PER_SECOND;

  return options;
}

AZ_NODISCARD az_result az_storage_tables_client_init(
    az_storage_tables_client* out_client,
    az_span endpoint,
    void* credential,
    az_storage_tables_client_options const* options)
{
  _az_PRECONDITION_NOT_NULL(out_client);
  _az_PRECONDITION_NOT_NULL(options);

  _az_credential* const cred = (_az_credential*)credential;

  *out_client = (az_storage_tables_client) {
    ._internal = {
      .endpoint = AZ_SPAN_FROM_BUFFER(out_client->_internal.endpoint_buffer),
      .options = *options,
      .credential = cred,
      .pipeline = (_az_http_pipeline){
        ._internal = {
          .policies = {
            {
              ._internal = {
                .process = az_http_pipeline_policy_apiversion,
                .options= &out_client->_internal.options._internal.api_version,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_telemetry,
                .options = &out_client->_internal.options._internal.telemetry_options,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_retry,
                .options = &out_client->_internal.options.retry_options,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_credential,
                .options = cred,
              },
            },
#ifndef AZ_NO_LOGGING
            {
              ._internal = {
                .process = az_http_pipeline_policy_logging,
                .options = NULL,
              },
            },
#endif // AZ_NO_LOGGING
            {
              ._internal = {
                .process = az_http_pipeline_policy_transport,
                .options = NULL,
              },
            },
          },
        }
      }
    }
  };

  // Copy url to client buffer so customer can re-use buffer on his/her side
  int32_t const uri_size = az_span_size(endpoint);
  
  
  _az_RETURN_IF_NOT_ENOUGH_SIZE(out_client->_internal.endpoint, uri_size);
  az_span_copy(out_client->_internal.endpoint, endpoint);
  out_client->_internal.endpoint = az_span_slice(out_client->_internal.endpoint, 0, uri_size);

  _az_RETURN_IF_FAILED(
      _az_credential_set_scopes(cred, AZ_SPAN_FROM_STR("https://storage.azure.com/.default")));

  return AZ_OK;
}
/*
AZ_NODISCARD az_result az_storage_tables_upload(
    az_storage_tables_client* ref_client,
    az_span content, // Buffer of content
    az_span contentMd5,  // Md5 hash of content
    az_span authorizationHeader,
    az_span timestamp,
    az_storage_tables_upload_options const* options,
    az_http_response* ref_response,
    uint8_t * reqPreparePtr)
*/
AZ_NODISCARD az_result az_storage_tables_upload(
    az_storage_tables_client* ref_client,
    az_span content, // Buffer of content
    az_span contentMd5,  // Md5 hash of content
    az_span authorizationHeader,
    az_span timestamp,
    az_storage_tables_upload_options const* options,
    az_http_response* ref_response)
{

  az_storage_tables_upload_options opt;
  if (options == NULL)
  {
    opt = az_storage_tables_upload_options_default();
  }
  else
  {
    opt = *options;
  }

  // Request buffer
  // create request buffer TODO: define size for a blob upload
  

  uint8_t url_buffer[ROSCHMI_AZ_HTTP_REQUEST_URL_BUFFER_SIZE];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  // copy url from client
  int32_t uri_size = az_span_size(ref_client->_internal.endpoint);
  _az_RETURN_IF_NOT_ENOUGH_SIZE(request_url_span, uri_size);
  //az_span_copy(request_url_span, ref_client->_internal.endpoint);
  
  // RoSchmi
  request_url_span = az_span_slice(ref_client->_internal.endpoint, 0, uri_size);
  
  uint8_t headers_buffer[_az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE] = {0};
  
  az_span request_headers_span = az_span_create(headers_buffer, _az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE);

  // create request
  az_http_request request;
  _az_RETURN_IF_FAILED(az_http_request_init(
      &request,
      opt.context,
      az_http_method_post(),
      request_url_span,
      uri_size,
      request_headers_span,
      content));
 
 _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_ACCEPT_TYPE, options->_internal.acceptType));

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_XMS_DATE, timestamp));

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_AUTHORIZATION, authorizationHeader));

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_CONTENT_MD5, contentMd5));

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_PREFERE, options->_internal.perferType));

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_ACCEPT_CHARSET, AZ_STORAGE_TABLES_CHARSET_UTF8));

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_HTTP_HEADER_CONTENT_TYPE, options->_internal.contentType));

 _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_DATASERVICE_VERSION, AZ_STORAGE_TABLES_DATASERVICE_VERS_3_0));         

_az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_STORAGE_TABLES_HEADER_MAX_DATASERVICE_VERSION, AZ_STORAGE_TABLES_MAX_DATASERVICE_VERS_3_0_NETFX));

// Add Accept-Encoding header: chunked. Using identity evtl. caused issues
//  _az_RETURN_IF_FAILED(
//     az_http_request_append_header(&request, AZ_HTTP_HEADER_ACCEPT_ENCODING, AZ_HTTP_ACCEPT_ENCODING_CHUNKED));

 _az_RETURN_IF_FAILED(
     az_http_request_append_header(&request, AZ_HTTP_HEADER_ACCEPT_ENCODING, AZ_HTTP_ACCEPT_ENCODING_IDENTITY));

az_span urlWorkCopy = ref_client->_internal.endpoint;
 
int32_t colonIndex = az_span_find(urlWorkCopy, AZ_SPAN_FROM_STR(":"));

char protocol[6] = {0};	
  
/* bool protocolIsHttpOrHttps = false; */
int32_t slashIndex = - 1;
if (colonIndex != -1)
{
  az_span_to_str(protocol, 6, az_span_slice(urlWorkCopy, 0, colonIndex));
  if ((strcmp(protocol, (const char *)"https") == 0) || (strcmp(protocol, (const char *)"http") == 0))
  {
      /* protocolIsHttpOrHttps = true; */
  }

  slashIndex = az_span_find(az_span_slice_to_end(urlWorkCopy, colonIndex + 3), AZ_SPAN_FROM_STR("/"));     
  slashIndex = (slashIndex != -1) ? slashIndex + colonIndex + 3 : -1;       
}
    
char workBuffer[100] = {0};
    
if (slashIndex == -1)
{
  az_span_to_str(workBuffer, sizeof(workBuffer), az_span_slice_to_end(urlWorkCopy, colonIndex + 3));
}
else
{
  az_span_to_str(workBuffer, sizeof(workBuffer), az_span_slice_to_end(az_span_slice(urlWorkCopy, 0, slashIndex), colonIndex + 3));
}

_az_RETURN_IF_FAILED(
        az_http_request_append_header(&request, AZ_HTTP_HEADER_HOST, AZ_SPAN_FROM_BUFFER(workBuffer)));

  
//_az_RETURN_IF_FAILED(az_http_request_append_header(
//      &request, AZ_STORAGE_TABLES_HEADER_EXPECT_TYPE, AZ_STORAGE_TABLES_HEADER_EXPECT_100_CONTINUE));    
    
//Add connection Close header
//_az_RETURN_IF_FAILED(
//     az_http_request_append_header(&request, AZ_HTTP_HEADER_CONNECTION, AZ_HTTP_CONNECTION_CLOSE));

  uint8_t content_length[_az_INT64_AS_STR_BUFFER_SIZE] = { 0 };
  az_span content_length_span = AZ_SPAN_FROM_BUFFER(content_length);

  az_span remainder;
  _az_RETURN_IF_FAILED(az_span_i64toa(content_length_span, az_span_size(content), &remainder));
  content_length_span
      = az_span_slice(content_length_span, 0, _az_span_diff(remainder, content_length_span));

  // add Content-Length to request
  _az_RETURN_IF_FAILED(
      az_http_request_append_header(&request, AZ_HTTP_HEADER_CONTENT_LENGTH, content_length_span));

  //_az_RETURN_IF_FAILED(
  //      az_http_request_append_header(&request, AZ_HTTP_HEADER_HOST, ref_client->_internal.endpoint));


  // add blob type to request
  /*
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_HTTP_HEADER_CONTENT_TYPE, AZ_SPAN_FROM_STR("text/plain")));
  */

  // start pipeline
  
  //RoSchmi
  // show how headers can be accessed
  /*
  __unused  size_t headerCount = az_http_request_headers_count(&request);
  az_span header_name = { 0 };
  az_span header_value = { 0 };
  __unused  az_result getHeaderResult =  az_http_request_get_header(&request, 12, &header_name, &header_value);
  */
  return az_http_pipeline_process(&ref_client->_internal.pipeline, &request, ref_response);
}

