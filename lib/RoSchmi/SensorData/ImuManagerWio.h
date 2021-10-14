#include <Arduino.h>
#include <Datetime.h>

#ifndef _IMU_MANAGER_WIO_H_
#define _IMU_MANAGER_WIO_H_

#define IMU_ARRAY_ELEMENT_COUNT 5

typedef struct
{
    float X_Read = 0;
    float Y_Read = 0;
    float Z_Read = 0;
}
ImuSampleValues; 

typedef struct
{    
    ImuSampleValues ImuSampleSet[IMU_ARRAY_ELEMENT_COUNT];
}
ImuSampleValueSet;


class ImuManagerWio
{
public:
    ImuManagerWio();
    
    void begin();

    void SetInactive();
    void SetActive();
    void SetNewImuReadings(ImuSampleValues imuReadings);
    float GetVibrationValue();
    ImuSampleValues GetLastImuReadings();
    bool hasToggled(DateTime actUtcTime);  

private:
    bool isActive = false;
    int currentIndex = 0;
    bool averageIsReady = false;
    float floatingAverage(ImuSampleValueSet sampleSet, int arrayElementCnt, char axis);

};

#endif