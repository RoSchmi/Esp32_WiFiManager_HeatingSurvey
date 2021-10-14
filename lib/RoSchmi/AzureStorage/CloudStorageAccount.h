
#include <Arduino.h>

#ifndef _CLOUDSTORAGEACCOUNT_H_
#define _CLOUDSTORAGEACCOUNT_H_

#define MAX_ACCOUNTNAME_LENGTH 50
#define ACCOUNT_KEY_LENGTH     88

class CloudStorageAccount
{
public:
    CloudStorageAccount();
    CloudStorageAccount(String accountName, String accountKey, bool useHttps);
    ~CloudStorageAccount();

    String AccountName;
    String AccountKey;
    //String UriEndPointBlob;
    //String UriEndPointQueue;
    String UriEndPointTable;
    //String HostNameBlob;
    //String HostNameQueue;
    String HostNameTable;
};


#endif  // _CLOUDSTORAGEACCOUNT_H_