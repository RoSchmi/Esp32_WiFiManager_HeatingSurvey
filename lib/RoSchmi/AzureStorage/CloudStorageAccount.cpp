#include <CloudStorageAccount.h>

/**
 * constructor
 */
CloudStorageAccount::CloudStorageAccount(String accountName, String accountKey, bool useHttps )
{
    AccountName = (accountName.length() <= MAX_ACCOUNTNAME_LENGTH) ? accountName : accountName.substring(0, MAX_ACCOUNTNAME_LENGTH);
    AccountKey = accountKey;
    char strData[accountName.length() + 30];
    const char * insert = (char *)useHttps ? "s" : "";

    sprintf(strData, "http%s://%s.table.core.windows.net", insert, accountName.c_str());
    UriEndPointTable = String(strData);
    //sprintf(strData, "http%s://%s.blob.core.windows.net", insert, accountName.c_str());
    //UriEndPointBlob = String(strData);
    //sprintf(strData, "http%s://%s.queue.core.windows.net", insert, accountName.c_str());
    //UriEndPointQueue = String(strData);

    sprintf(strData, "%s.table.core.windows.net", accountName.c_str());
    HostNameTable = String(strData);
    //sprintf(strData, "%s.blob.core.windows.net", accountName.c_str());
    //HostNameBlob = String(strData);
    //sprintf(strData, "%s.queue.core.windows.net", accountName.c_str());
    //HostNameQueue = String(strData);
}

void CloudStorageAccount::ChangeAccountParams(String accountName, String accountKey, bool useHttps)
{
    AccountName = (accountName.length() <= MAX_ACCOUNTNAME_LENGTH) ? accountName : accountName.substring(0, MAX_ACCOUNTNAME_LENGTH);
    AccountKey = accountKey;
    char strData[accountName.length() + 30];
    const char * insert = (char *)useHttps ? "s" : "";
    sprintf(strData, "http%s://%s.table.core.windows.net", insert, accountName.c_str());
    UriEndPointTable = String(strData);
    
    sprintf(strData, "%s.table.core.windows.net", accountName.c_str());
    HostNameTable = String(strData);
}



/**
 * destructor
 */
CloudStorageAccount::~CloudStorageAccount()
{}