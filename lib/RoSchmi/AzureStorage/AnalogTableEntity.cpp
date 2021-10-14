#include "AnalogTableEntity.h"
       
// Your entity type must expose a parameter-less constructor
        AnalogTableEntity::AnalogTableEntity() { }


        // Define the PK and RK
        AnalogTableEntity::AnalogTableEntity(az_span partitionKey, az_span rowKey, az_span sampleTime, EntityProperty *pProperties, size_t propertyCount)
            : TableEntity(partitionKey, rowKey, sampleTime)
        {
            Properties = pProperties;    // store the ArrayList

            PropertyCount = propertyCount;
        
            PropertyClass myProperties;           
            myProperties.PartitionKey = partitionKey;           
            myProperties.RowKey = rowKey;
              
            myProperties.SampleTime = SampleTime;
            myProperties.T_1 = az_span_create_from_str(pProperties[1].Value);
            myProperties.T_2 = az_span_create_from_str(pProperties[2].Value);
            myProperties.T_3 = az_span_create_from_str(pProperties[3].Value);
            myProperties.T_4 = az_span_create_from_str(pProperties[4].Value);
            
        //  this.JsonString = JsonConverter.Serialize(myProperties).ToString();
        }

        AnalogTableEntity::PropertyClass::PropertyClass(){};
             