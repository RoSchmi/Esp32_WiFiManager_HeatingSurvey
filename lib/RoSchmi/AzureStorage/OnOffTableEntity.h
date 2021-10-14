#include <azure/core/az_span.h>
#include <azure/core/internal/az_span_internal.h>
#include "TableEntityProperty.h"
#include "TableEntity.h"


class OnOffTableEntity : public TableEntity
{
    public:
        OnOffTableEntity();
        OnOffTableEntity(az_span partitionKey, az_span rowKey, az_span sampleTime, EntityProperty[], size_t PropertyCount);
              
        class PropertyClass {
           public:
           az_span RowKey;
           az_span PartitionKey;
           
           az_span Act_Status;
           az_span Last_Status;
           az_span OnTimeDay;
           az_span SampleTime;          
           az_span TimeFromLast;
            
           PropertyClass();   

        };
        
};