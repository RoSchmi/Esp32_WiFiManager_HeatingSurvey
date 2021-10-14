#include "OnOffTableEntity.h"

      
// Your entity type must expose a parameter-less constructor
        OnOffTableEntity::OnOffTableEntity() { }


        // Define the PK and RK
        OnOffTableEntity::OnOffTableEntity(az_span partitionKey, az_span rowKey, az_span sampleTime, EntityProperty *pProperties, size_t propertyCount)
            : TableEntity(partitionKey, rowKey, sampleTime)
        {
            Properties = pProperties;    // store the ArrayList

            PropertyCount = propertyCount;
        
            PropertyClass myProperties;           
            myProperties.PartitionKey = partitionKey;           
            myProperties.RowKey = rowKey;
              
            myProperties.SampleTime = SampleTime;
            myProperties.Act_Status = az_span_create_from_str(pProperties[1].Value);
            myProperties.Last_Status = az_span_create_from_str(pProperties[2].Value);
            myProperties.OnTimeDay = az_span_create_from_str(pProperties[3].Value);
            myProperties.SampleTime = az_span_create_from_str(pProperties[4].Value);
            myProperties.TimeFromLast = az_span_create_from_str(pProperties[4].Value);         

        //  this.JsonString = JsonConverter.Serialize(myProperties).ToString();
        }

        OnOffTableEntity::PropertyClass::PropertyClass(){};
        
       