#include <Arduino.h>
#include <Datetime.h>

#ifndef _ANALOGSENSORMGR_H_
#define _ANALOGSENSORMGR_H_

#define SENSOR_COUNT 4

typedef struct
{
    // For Sensors which can read up to three values with one command
    float  Value_1 = 999.9;
    float  Value_2 = 999.9;
    float  Value_3 = 999.9;
    DateTime LastReadTime = DateTime();
    TimeSpan ReadInterval = TimeSpan(0,0,0,1);  // default every 2 seconds
    bool IsActive = true;
}
AnalogSensor;

class AnalogSensorMgr
{
    public:
    
    AnalogSensorMgr(float pMagicNumberInvalid);
    void SetReadInterval(uint32_t pInterval);
    void SetReadInterval(int sensorIndex, uint32_t pInterval);
    bool HasToBeRead(int pSensorIndex, DateTime now);
    void SetReadTimeAndValues(int pSensorIndex, DateTime now, float pReadValue_1, float pReadValue_2, float pReadValue_3);
    AnalogSensor GetSensorDates(int pSensorIndex);
    
    //float ReadSensor(int pSensorIndex);
    
    private:
      
};

#endif  // _ANALOGSENSORMGR_H_

