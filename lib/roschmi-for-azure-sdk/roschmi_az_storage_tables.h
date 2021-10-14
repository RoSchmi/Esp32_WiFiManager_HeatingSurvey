
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition for the Azure Storage Tables client.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ROSCHMI_AZ_STORAGE_TABLES_H
#define _ROSCHMI_AZ_STORAGE_TABLES_H

#include <azure/core/az_config.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/az_context.h>
#include <azure/core/az_credentials.h>
#include <azure/core/internal/az_credentials_internal.h>
#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/Internal/az_retry_internal.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/core/internal/az_http_internal.h>
#include <azure/core/az_precondition.h>
#include <azure/core/internal/az_precondition_internal.h>


#include <stdint.h>
#include <Arduino.h>

#include <azure/core/_az_cfg_prefix.h>

#define ROSCHMI_AZ_HTTP_REQUEST_URL_BUFFER_SIZE 100


/**
 * @brief Client is fixed to a specific version of the Azure Storage Tables service.
 */
static az_span const AZ_STORAGE_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("2015-04-05");

/**
 * @brief Allows customization of the table client.
 */
typedef struct
{
  /// Optional values used to override the default retry policy options.
  az_http_policy_retry_options retry_options;

  struct
  {
    /// Services pass API versions in the header or in query parameters used by the API Version
    /// policy.
    _az_http_policy_apiversion_options api_version;
    
    /// Options for the telemetry policy.
    _az_http_policy_telemetry_options telemetry_options;
  } _internal;
} az_storage_tables_client_options;

/**
 * @brief Azure Storage Blobs Blob Client.
 */
typedef struct
{
  struct
  {
    // buffer to copy customer url. Then it stays immutable
    uint8_t endpoint_buffer[ROSCHMI_AZ_HTTP_REQUEST_URL_BUFFER_SIZE];
    // this url will point to endpoint_buffer
    az_span endpoint;
    _az_http_pipeline pipeline;
    az_storage_tables_client_options options;
    _az_credential* credential;
  } _internal;
} az_storage_tables_client;


/**
 * @brief Initialize a client with default options.
 *
 * @param[out] out_client The blob client instance to initialize.
 * @param[in] endpoint A URL to a table storage account.
 * @param credential The object used for authentication. #AZ_CREDENTIAL_ANONYMOUS should be
 * used for SAS.
 * @param[in] options A reference to an #az_storage_tables_client_options structure which
 * defines custom behavior of the client.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result az_storage_tables_client_init(
    az_storage_tables_client* out_client,
    az_span endpoint,
    void* credential,
    az_storage_tables_client_options const* options);

/**
 * @brief Allows customization of the upload operation.
 */
typedef struct
{
  az_context* context; ///< Operation context.
  struct
  {
    /// Currently, this is unused, but needed as a placeholder since we can't have an empty struct.
    bool unused;
    az_span acceptType;
    az_span contentType;
    az_span perferType;

  } _internal;
} az_storage_tables_upload_options;

/**
 * @brief Gets the default table storage options.
 *
 * @details Call this to obtain an initialized #az_storage_tables_client_options structure that
 * can be modified and passed to #az_storage_tables_client_init().
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this
 * function and then overriding that specific option.
 */
AZ_NODISCARD az_storage_tables_client_options az_storage_tables_client_options_default();

/**
 * @brief Gets the default tables upload options.
 *
 * @details Call this to obtain an initialized #az_storage_tables_upload_options structure.
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this
 * function and then overriding that specific option.
 */
AZ_NODISCARD AZ_INLINE az_storage_tables_upload_options
az_storage_tables_upload_options_default()
{
  return (az_storage_tables_upload_options){ .context = &az_context_application,
                                                 ._internal = { 
                                                   .unused = false,
                                                   .acceptType = AZ_SPAN_LITERAL_FROM_STR("application/json"),
                                                   .contentType = AZ_SPAN_LITERAL_FROM_STR("application/atom+xml"),
                                                   .perferType = AZ_SPAN_LITERAL_FROM_STR("application/json"),
                                                  } };
}

/**
 * @brief Uploads the contents to table storage.
 *
 * @param[in,out] ref_client An #az_storage_table_client structure.
 * @param[in] content The table content to upload.
 * @param[in] options __[nullable]__ A reference to an #az_storage_table_upload_options
 * structure which defines custom behavior for uploading the blob. If `NULL` is passed, the client
 * will use the default options (i.e. #az_storage_tables_upload_options_default()).
 * @param[in,out] ref_response An initialized #az_http_response where to write HTTP response into.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */

AZ_NODISCARD az_result az_storage_tables_upload(
    az_storage_tables_client* ref_client,
    az_span content,
    az_span contentMd5,
    az_span authorizationHeader,
    az_span timestamp,
    az_storage_tables_upload_options const* options,
    az_http_response* ref_response);
    

#include <azure/core/_az_cfg_suffix.h>

#endif // _ROSCHMI_AZ_STORAGE_TABLES_H
