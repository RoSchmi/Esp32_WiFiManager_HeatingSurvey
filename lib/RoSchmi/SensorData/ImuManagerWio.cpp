#include "ImuManagerWio.h"

ImuSampleValueSet sampleSet;

float averageX = 0;
float averageY = 0;
float averageZ = 0;

ImuManagerWio::ImuManagerWio()
{}

void ImuManagerWio::begin()
{}

void ImuManagerWio::SetNewImuReadings(ImuSampleValues imuReadings)
{
    if (currentIndex == IMU_ARRAY_ELEMENT_COUNT - 1)
    {
         averageIsReady = true;
         currentIndex = 0;
    }
    else
    {
        currentIndex++;
    }
    sampleSet.ImuSampleSet[currentIndex].X_Read = imuReadings.X_Read;
    sampleSet.ImuSampleSet[currentIndex].Y_Read = imuReadings.Y_Read;
    sampleSet.ImuSampleSet[currentIndex].Z_Read = imuReadings.Z_Read;

    averageX =  floatingAverage(sampleSet, IMU_ARRAY_ELEMENT_COUNT, 'X');
    averageY =  floatingAverage(sampleSet, IMU_ARRAY_ELEMENT_COUNT, 'Y');
    averageZ =  floatingAverage(sampleSet, IMU_ARRAY_ELEMENT_COUNT, 'Z');

    //Serial.println(averageX);
    //Serial.println(averageY);
    //Serial.println(averageZ);
    //Serial.println("");  
}

float ImuManagerWio::floatingAverage(ImuSampleValueSet sampleSet, int arrayElementCnt, char axis)
{
    float sum = 0;

    for (int i = 0; i < arrayElementCnt; i++)
    {
        switch (axis)
        {
            case 'X':
                {
                    sum += sampleSet.ImuSampleSet[i].X_Read;
                }
                break;
                case 'Y':
                {
                    sum += sampleSet.ImuSampleSet[i].Y_Read;
                }
                break;
                case 'Z':
                {
                    sum += sampleSet.ImuSampleSet[i].Z_Read;
                }
                break;
                default:
                {
                    while (true)
                    {
                        Serial.println("Error in ImuManagerWio");
                        delay(1000);
                    }                                   
            }
        }
    }   
    return sum / arrayElementCnt;
}
    
ImuSampleValues ImuManagerWio::GetLastImuReadings()
{
    ImuSampleValues retValues;
    retValues.X_Read = 0;
    retValues.Y_Read = 0;
    retValues.Z_Read = 0;

    return isActive ? sampleSet.ImuSampleSet[currentIndex] : retValues;
}

float ImuManagerWio::GetVibrationValue()
{
    // First tests -- doesn't work well
    // Have to find a better way to measure vibration

    ImuSampleValueSet averagedSampleSet;

    volatile float retValue = 0;

    float summedSqDev_X = 0;
    float summedSqDev_Y = 0;
    float summedSqDev_Z = 0;

    if (!isActive)
    {
        return 0;
    }
    else
    {
        for (int i = 0; i < IMU_ARRAY_ELEMENT_COUNT; i++)
        {   
            // deviation squares
            averagedSampleSet.ImuSampleSet[i].X_Read = sq((sampleSet.ImuSampleSet[i].X_Read - averageX) * 10 );
            summedSqDev_X += averagedSampleSet.ImuSampleSet[i].X_Read;
            averagedSampleSet.ImuSampleSet[i].Y_Read = sq((sampleSet.ImuSampleSet[i].Y_Read - averageY) + 10);
            summedSqDev_Y += averagedSampleSet.ImuSampleSet[i].Y_Read;
            averagedSampleSet.ImuSampleSet[i].Z_Read = sq((sampleSet.ImuSampleSet[i].Z_Read - averageZ) + 10);
            summedSqDev_Z += averagedSampleSet.ImuSampleSet[i].Z_Read;      
        }
        retValue = (summedSqDev_X + summedSqDev_Y + summedSqDev_Z) / (IMU_ARRAY_ELEMENT_COUNT * 3);
        return retValue;
    } 
}

void ImuManagerWio::SetInactive()
{
    isActive = false;
}

void ImuManagerWio::SetActive()
{
    isActive = true;
}
