#include <Arduino.h>
#include <azure/core/az_span.h>


#ifndef _TABLE_ENTITY_PROPERTY_H_
#define _TABLE_ENTITY_PROPERTY_H_

#define MAX_ENTITYPROPERTY_NAME_LENGTH   25
#define MAX_ENTITYPROPERTY_VALUE_LENGTH  30
#define MAX_ENTITYPROPERTY_TYPE_LENGTH   13

struct EntityProperty
{   
    char Name[MAX_ENTITYPROPERTY_NAME_LENGTH];
    char Value[MAX_ENTITYPROPERTY_VALUE_LENGTH];
    char Type[MAX_ENTITYPROPERTY_TYPE_LENGTH];
};

EntityProperty TableEntityProperty(char * Name, char * Value, char * Type);

#endif