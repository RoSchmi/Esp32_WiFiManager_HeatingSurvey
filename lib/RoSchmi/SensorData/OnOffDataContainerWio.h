#include <Arduino.h>
#include "DateTime.h"

#ifndef _ON_OFF_DATACONTAINERWIO_H_
#define _ON_OFF_DATACONTAINERWIO_H_

#define PROPERTY_COUNT 4

typedef struct
{
    bool inputInverter = false;
    bool outInverter = false;
    bool actState = false; 
    bool lastState = true;
    bool hasToBeSent = false;
    bool dayIsLocked = false;
    bool resetToOnIsNeeded = false;
    DateTime LastSwitchTime = DateTime();
    TimeSpan OnTimeDay = TimeSpan(0);
    TimeSpan TimeFromLast = TimeSpan(0);
    char tableName[50];
    uint32_t insertCounter = 0;
    uint16_t Year = 1900;
}
OnOffSampleValue; 
    
typedef struct
{    
    OnOffSampleValue OnOffSampleValues[PROPERTY_COUNT];
}
OnOffSampleValueSet;

class OnOffDataContainerWio
{

public:
    OnOffDataContainerWio();

    //void begin();
    
    void begin(DateTime time, const char * tableName_1, const char * tableName_2, const char * tableName_3, const char * tableName_4);
    //void begin(uint32_t time, const char * tableName_1, const char * tableName_2, const char * tableName_3, const char * tableName_4);

/**
 * @brief Sets the 'state' variable if the new value is different to the value before.
 *        If the 'state' is changed, 'lastState' is set to the former value of 'state'
 *        The 'hasToBeSent' flag is set
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] state Sets the 'state'-variable of the selected OnOff-Sensor representation
 * @param[in] time Sets the 'LastSwitchTime'-variable of the selected OnOff-Sensor representation.
 *              If time = nullptr or time is not passed, 'LastSwitchTime' is not changed
 */
    void SetNewOnOffValue(int sensorIndex, bool state, DateTime time, int offsetUtcMinutes);

/**
 * @brief Sets State and LastState without affecting 'hasToBeSent"-State.
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] state Sets the 'state'-variable of the selected OnOff-Sensor representation
 * @param[in] lastState Sets the 'lastState'-variable of the selected OnOff-Sensor representation
 * @param[in] time Sets the 'LastSwitchTime'-variable of the selected OnOff-Sensor representation.
 *              If time = nullptr or time is not passed, 'LastSwitchTime' is not changed
 * 
 */
    void PresetOnOffState(int sensorIndex, bool state, bool lastState, DateTime time = DateTime());
    //void PresetOnOffState(int sensorIndex, bool state, bool lastState, uint32_t);

/**
 * @brief Reads the On/Off-State.
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 */   
    bool ReadOnOffState(int sensorIndex);

/**
 * @brief Resets the 'hasToBeSent"-flag of the selected sensor representation.
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 */    
    void Reset_hasToBeSent(int sensorIndex);

/**
 * @brief Sets a flag which indicates whether the state has to be inverted for use.
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] invertState True = shall be inverted, False = shall not be inverted
 * 
 */  
    void Set_OutInverter(int sensorIndex, bool invertState);

/**
 * @brief Sets a flag which indicates whether the input state is inverted.
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] invertState Indicates whether the input state is inverted
 * 
 */  
    void Set_InputInverter(int sensorIndex, bool invertState);
    
/**
 * @brief Sets a flag which indicates whether the dayIsLocked state is set.
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] isLockedState Indicates whether the dayIsLocked state is set
 * 
 */  

    void Set_DayIsLockedFlag(int sensorIndex, bool isLockedState);

/**
 * @brief Sets a flag which indicates whether the state has to be reset to on (end of day spike).
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] state Indicates whether the resetToOnIsNeeded shall be set
 * 
 */ 

    void Set_ResetToOnIsNeededFlag(int sensorIndex, bool state);


/**
 * @brief Sets the Year (of the last upload)
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] year The year 
 * 
 */  
    void Set_Year(int sensorIndex, int year);

  /**
 * @brief Sets the DateTime (of the last upload)
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] pSwitchDateTime The dateTime 
 * 
 */  
    void Set_LastSwitchTime(int sensorIndex, DateTime pSwitchDateTime);

    /**
 * @brief Sets the onTimeDay Timespan
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @param[in] pOnTimeDay The timespan the sensor was in 'on' state on this day
 * 
 */  
    void Set_OnTimeDay(int sensorIndex, TimeSpan pOnTimeDay); 

/**
 * @brief Returns true if at least in one of the sensor representations the 'hasToBeSent'-flag is set
 *
 * @param[in] sensorIndex The index of 4 OnOff-Tables (0 - 3)
 * @return Indicating if at least for one Sensor representation the 'hasToBeSent'-flag is set.
 */  
    bool One_hasToBeBeSent(DateTime localNow);
     
    OnOffSampleValueSet GetOnOffValueSet();   
};

#endif  // _ON_OFF_DATACONTAINERWIO_H_