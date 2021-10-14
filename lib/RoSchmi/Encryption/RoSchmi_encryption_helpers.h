#include <Arduino.h>

#include "mbedtls/md.h"
#include "mbedtls/md5.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"

#ifndef _ROSCHMI_ENCRYPTION_HELPERS_H_
#define _ROSCHMI_ENCRYPTION_HELPERS_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

int createSHA256Hash(char * output32Bytes, size_t outputLength, const char * input, size_t inputLength, const char * key, const size_t keyLength);

int createMd5Hash(char * output16Bytes, size_t md5HashStrLenght, const char * input);

void stringToHexString(char * output, const char * input, const char * delimiter); 

int base64_decodeRoSchmi(const char * input, char * output);

int base64_encodeRoSchmi(const char * input, const size_t inputLength, char * output, const size_t outputLength);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif




