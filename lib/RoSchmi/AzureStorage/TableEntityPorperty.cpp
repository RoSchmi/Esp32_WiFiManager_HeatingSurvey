#include "TableEntityProperty.h"

EntityProperty TableEntityProperty(char * pName, char * pValue, char * pType)
{
   // Limit input parameter to their max length
   char * validName = (char *)pName;
   if (strlen(pName) >  MAX_ENTITYPROPERTY_NAME_LENGTH)
   {
      validName[MAX_ENTITYPROPERTY_NAME_LENGTH] = '\0';
   }

   char * validValue = (char *)pValue;
   if (strlen(pValue) >  MAX_ENTITYPROPERTY_VALUE_LENGTH)
   {
      validValue[MAX_ENTITYPROPERTY_VALUE_LENGTH] = '\0';
   }

   char * validType = (char *)pType;
   if (strlen(pType) >  MAX_ENTITYPROPERTY_TYPE_LENGTH)
   {
      validType[MAX_ENTITYPROPERTY_TYPE_LENGTH] = '\0';
   }

    EntityProperty property;
   
    strcpy(property.Name, validName);
    strcpy(property.Value, validValue);
    strcpy(property.Type, validType); 
    return property;
}
