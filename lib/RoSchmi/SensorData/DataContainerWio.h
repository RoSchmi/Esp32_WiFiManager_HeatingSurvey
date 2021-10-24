#include <Arduino.h>
#include <Datetime.h>

#ifndef _DATACONTAINERWIO_H_
#define _DATACONTAINERWIO_H_

#define PROPERTY_COUNT 4

typedef struct
{
    float  Value = 999.9;
    DateTime LastSendTime;  
}
SampleValue; 
    
typedef struct
{  
    DateTime LastSendTime;
    DateTime LastUpdateTime;
    SampleValue SampleValues[PROPERTY_COUNT];
}
SampleValueSet; 

class DataContainerWio
{

public:
    
    DataContainerWio(TimeSpan pSendInterval, TimeSpan pInvalidateInterval, float pLowerLimit, float pUpperLimit, float pMagicNumberInvalid); 
    
    String floToStr(float value);
    SampleValue checkedSampleValue(SampleValue inSampleValue, float lowLimit, float upperLimit, float invalidSubstitute,  DateTime actDateTime, TimeSpan invalidateTime);
    void SetNewValue(uint32_t pIndex, DateTime pActDateTime, float pSampleValue);
    SampleValueSet getCheckedSampleValues(DateTime pActDateTime);
    SampleValueSet getSampleValues(DateTime pActDateTime);
    void setHasToBeSentFlag();
    bool hasToBeSent();
    void setLowerLimit(float pLowerLimit);
    void setUpperLimit(float pUpperLimit);
    void setMagigNumberInvalid(float pMagicNumberInvalid);
    /**
    * @brief Sets the Year (of the last upload)
    *
    * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
    * @param[in] year The year 
    * 
    */  
    void Set_Year(uint16_t year);

    bool _isFirstTransmission = true;

    TimeSpan SendInterval;
    TimeSpan InvalidateInterval;
    float LowerLimit = -40.0;
    float UpperLimit = 140.0;
    float MagicNumberInvalid = 999.9;

    bool _hasToBeSent = false;
    uint16_t Year = 1900;

    DateTime _lastSentTime;
    SampleValue SampleValues[PROPERTY_COUNT];
    SampleValueSet _SampleValuesSet;
};

#endif  // _DATACONTAINERWIO_H_