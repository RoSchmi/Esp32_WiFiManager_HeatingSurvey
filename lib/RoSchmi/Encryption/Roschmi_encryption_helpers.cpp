#include "RoSchmi_encryption_helpers.h"


// returns 0 if everthing worked o.k. returns 1 in case of some error
int createSHA256Hash(char * output32Bytes, size_t outputLength, const char * input, const size_t inputLength, const char * key, const size_t keyLength)
{
    if (outputLength >= 33)
    {
        uint8_t hmacResult[32];
        mbedtls_md_context_t ctxSHA256;          
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;            
        mbedtls_md_init(&ctxSHA256);
        volatile int setUpResult = mbedtls_md_setup(&ctxSHA256, mbedtls_md_info_from_type(md_type), 1); 
        volatile int startResult = mbedtls_md_hmac_starts( &ctxSHA256, (const unsigned char *) key, keyLength);
        volatile int updateResult = mbedtls_md_hmac_update(&ctxSHA256, (const unsigned char *) input, inputLength);
        volatile int finishResult = mbedtls_md_hmac_finish(&ctxSHA256, hmacResult);
        mbedtls_md_free(&ctxSHA256);

        char * ptr = &output32Bytes[0];
        for (int i = 0; i < 32; i++) {
            ptr[i] = hmacResult[i];
        }
        ptr[32] = '\0';      
        return 0;
    }
    else
    {
         return 1;
    }
}



// returns 0 if everthing worked o.k. returns 1 in case of some error

int createMd5Hash(char * output17Bytes, size_t outputLength, const char * input)
{
    uint8_t md5hash[16];  
           
    if (outputLength >= 17)
    {
        mbedtls_md5_context ctxMd5; 
             
        mbedtls_md5_init(&ctxMd5);
        //RoSchmi
        //mbedtls_md5_starts(&ctxMd5);
        mbedtls_md5_starts_ret(&ctxMd5);   
        mbedtls_md5_update(&ctxMd5, (const uint8_t*) input, strlen(input));           
        mbedtls_md5_finish(&ctxMd5, md5hash);
                
        char * ptr = &output17Bytes[0];
        for (int i = 0; i < 16; i++) {
            ptr[i] = md5hash[i];
        }
        ptr[16] = '\0';

        mbedtls_md5_free(&ctxMd5);
        return 0;
    }
    else
    {
        int dummy34 = 1;
        dummy34++;
        return 1;
    }  
}



// result can be retrieved from output
void stringToHexString(char * output, const char * input, const char * delimiter = "")
{
    const char _hexCharacterTable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };  
    size_t charsCount = (strlen(input) * 2) + 1 + strlen(input - 1) * strlen(delimiter);          
    char chars[charsCount];
    int j = 0;
    for (size_t i = 0; i < strlen(input); i++)
    {
        chars[j++] = (char)_hexCharacterTable[(input[i] & 0xF0) >> 4];
        chars[j++] = (char)_hexCharacterTable[input[i] & 0x0F];
                    
        if (i != (strlen(input) - 1))
        {                       
            for (size_t i = 0; i < strlen(delimiter); i++)
            {                            
                chars[j++] = delimiter[i];                            
            }                     
        }                  
    }
    chars[j] = '\0';

    output[charsCount] = '\0';
    for (int i = 0; i < charsCount; i++) 
    {
        output[i] = chars[i];
    }
}

// returns -1 if something went wrong, otherwise count of bytes written
int base64_decodeRoSchmi(const char * input, char * output)
{

    size_t destLen = strlen(input);  // size of the destination buffer
    char base64DecOut[destLen];     // destination buffer (can be NULL for checking size)
            
    size_t olen = 0;         // number of bytes written 

    mbedtls_base64_decode((unsigned char *)base64DecOut, destLen, &olen, (unsigned char *)input, strlen(input));
            
    if (olen == 0)
    {
        return -1;
    }
    else
    {             
        size_t ctr = olen;
        output[ctr] = '\0';
        for (int i = 0; i < ctr; i++) {
        output[i] = base64DecOut[i];
        }                
        return olen;
    }
}


// returns 0 if everthing worked o.k. returns 1 in case of some error
int base64_encodeRoSchmi(const char * input, const size_t inputLength, char * output, const size_t outputLength)
{
    if (outputLength > inputLength * 1.5 )
    {       
        unsigned char base64EncOut[outputLength];     // destination buffer (can be NULL for checking size)
            
        size_t outPutWritten = 0;         // number of bytes written 
        int Base64EncodeResult = mbedtls_base64_encode((unsigned char *)base64EncOut, outputLength, &outPutWritten,
                   (const unsigned char *)input, inputLength);


        if (outPutWritten == 0)
        {
            return 1;
        }
        else
        {
                // RoSchmi Todo correct calculation of ctr
                //size_t ctr = outPutWritten > sizeof(output) ? (sizeof(output) -1) : outPutWritten;
        size_t ctr = outPutWritten;
        output[ctr] = '\0';
        for (int i = 0; i < ctr; i++) 
        {                  
            output[i] = base64EncOut[i];
        }                
        return 0;
        }
    }
    else
    {
        return 1;
    }
}
