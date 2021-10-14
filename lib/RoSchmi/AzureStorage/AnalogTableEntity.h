#include <azure/core/az_span.h>
#include <azure/core/internal/az_config_internal.h>
#include <TableEntityProperty.h>
#include <TableEntity.h>


class AnalogTableEntity : public TableEntity
{
    public:
        AnalogTableEntity();
        AnalogTableEntity(az_span partitionKey, az_span rowKey, az_span sampleTime, EntityProperty[], size_t PropertyCount);
              
        class PropertyClass {
           public:
           az_span RowKey;
           az_span PartitionKey;
           az_span SampleTime;
           az_span T_1;
           az_span T_2;
           az_span T_3;
           az_span T_4;
            
           PropertyClass();
           //PropertyClass(az_span RowKey, az_span PartitionKey);  

        };
        
};