#include "TableEntity.h"
  
az_span     ETag;
DateTime    TimeStamp;
    
TableEntity::TableEntity() { }

TableEntity::TableEntity(az_span partitionKey, az_span rowKey, az_span sampleTime)
{   
    PartitionKey = partitionKey;
    RowKey = rowKey;
    SampleTime = sampleTime;
}
        
//az_span JsonString = az_span_create_from_str((char *)"");

/*
public string ReadJson()
{
    return JsonString;
}

public string ReadEntity()
    { return "Not implemented"; }

    public void WriteEntity(string Ausgabe)
    { Debug.WriteLine(Ausgabe); }

*/


    