#include <Arduino.h>
#include "WiFiClientSecure.h"
#include "HTTPClient.h"
#include "DateTime.h"

#include "mbedtls/md.h"
#include "mbedtls/md5.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include "azure/core/az_http.h"
//#include "Time/SysTime.h"
//#include "Time/Rs_time_helpers.h"

#include "CloudStorageAccount.h"

#include "TableEntity.h"
#include "RoSchmi_encryption_helpers.h"

#include "roschmi_az_storage_tables.h"
#include "az_esp32_roschmi.h"

#ifndef _TABLECLIENT_H_
#define _TABLECLIENT_H_

#define MAX_TABLENAME_LENGTH 50
#define RESPONSE_BUFFER_LENGTH 2000
#define REQUEST_BODY_BUFFER_LENGTH 900
#define PROPERTIES_BUFFER_LENGTH 300
#define AUTH_HEADER_BUFFER_LENGTH 100
#define REQUEST_PREPARE_PTR_BUFFER_LENGTH 500

  typedef enum {
    contApplicationIatomIxml,
    contApplicationIjson
    } ContType;

typedef enum 
        {
            acceptApplicationIatomIxml,
            acceptApplicationIjson
        } AcceptType;

typedef enum 
        {
            returnContent,
            dont_returnContent
        } ResponseType;

class TableClient
{
public:
     
    TableClient(CloudStorageAccount *account, const char * caCert, HTTPClient *httpClient, WiFiClient * wifiClient, uint8_t * bufferStore);
    
    ~TableClient();

    az_http_status_code CreateTable(const char * tableName, DateTime pDateTimeUtcNow, ContType pContentType = ContType::contApplicationIatomIxml, AcceptType pAcceptType = AcceptType::acceptApplicationIjson, ResponseType pResponseType = ResponseType::returnContent, bool useSharedKeyLight = false);
    az_http_status_code InsertTableEntity(const char * tableName, DateTime pDateTimeUtcNow, TableEntity pEntity, char* out_ETAG, DateTime * outResonseHeaderDate, ContType pContentType, AcceptType pAcceptType, ResponseType pResponseType, bool useSharedKeyLite = false);   
    void CreateTableAuthorizationHeader(const char * content, const char * canonicalResource, const char * ptimeStamp, const char * pHttpVerb, az_span pConentType, char * pMd5Hash, char pAutorizationHeader[], bool useSharedKeyLite = false);
    int32_t dow(int32_t year, int32_t month, int32_t day);
};
#endif 