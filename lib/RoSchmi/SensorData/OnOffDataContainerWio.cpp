#include "OnOffDataContainerWio.h"

OnOffSampleValueSet onOffSampleValueSet;

OnOffDataContainerWio::OnOffDataContainerWio()
{}


void OnOffDataContainerWio::begin(DateTime lastSwitchTime, const char * tableName_1, 
const char * tableName_2, const char * tableName_3, const char * tableName_4)
{ 
    strcpy(onOffSampleValueSet.OnOffSampleValues[0].tableName, tableName_1);
    strcpy(onOffSampleValueSet.OnOffSampleValues[1].tableName, tableName_2);
    strcpy(onOffSampleValueSet.OnOffSampleValues[2].tableName, tableName_3);
    strcpy(onOffSampleValueSet.OnOffSampleValues[3].tableName, tableName_4);

    for (int i = 0; i < PROPERTY_COUNT; i++)
    {
        onOffSampleValueSet.OnOffSampleValues[i].actState = false;
        onOffSampleValueSet.OnOffSampleValues[i].lastState = true;
        onOffSampleValueSet.OnOffSampleValues[i].hasToBeSent = false;
        onOffSampleValueSet.OnOffSampleValues[i].LastSwitchTime = lastSwitchTime;
    }
}



// If state is different to the value it had before, actstate is changed and lastState
// is set to the value which actstate was before
// LastSwitchTime is set to the value passed in parameter time
// The hasToBeSent flag is set
// If we have a new day (local time), a new OnTimeDay is calculated
// If we have a new day the 'dayIsLocked' flag is cleared 
void OnOffDataContainerWio::SetNewOnOffValue(int sensorIndex, bool state, DateTime pTimeUtc, int offsetUtcMinutes)
{
    // change incoming state if inputInverter is active
    bool _state = onOffSampleValueSet.OnOffSampleValues[sensorIndex].inputInverter ? !state : state;

    DateTime _localTime = pTimeUtc.operator+(TimeSpan(offsetUtcMinutes * 60));   
    DateTime _localTimeOfLastSwitch = onOffSampleValueSet.OnOffSampleValues[sensorIndex].LastSwitchTime;  
    TimeSpan _timeFromLast = _localTime.operator-(_localTimeOfLastSwitch);

    DateTime lastSwitchTimeDate = DateTime(_localTimeOfLastSwitch.year(), _localTimeOfLastSwitch.month(), _localTimeOfLastSwitch.day());
        
    DateTime actLocalTimeDate = DateTime(_localTime.year(), _localTime.month(), _localTime.day());
      
    // if the day has chanced
    if (lastSwitchTimeDate.operator!=(actLocalTimeDate))
    {
            // if we switch from true (on) to off (false) we calculate new OnTimeDay
        if (onOffSampleValueSet.OnOffSampleValues[sensorIndex].actState == true)
        {
            onOffSampleValueSet.OnOffSampleValues[sensorIndex].OnTimeDay = _localTime.operator-(DateTime(_localTime.year(), _localTime.month(), _localTime.day(), 0, 0, 0));
        }
        else
        {
            onOffSampleValueSet.OnOffSampleValues[sensorIndex].OnTimeDay = TimeSpan(0);
        }

        onOffSampleValueSet.OnOffSampleValues[sensorIndex].dayIsLocked = false;
        onOffSampleValueSet.OnOffSampleValues[sensorIndex].resetToOnIsNeeded = false;
    }
    else    // not a new day
    {
            if (onOffSampleValueSet.OnOffSampleValues[sensorIndex].actState == true)
        {
                onOffSampleValueSet.OnOffSampleValues[sensorIndex].OnTimeDay = 
                onOffSampleValueSet.OnOffSampleValues[sensorIndex].OnTimeDay.operator+(_localTime.operator-(onOffSampleValueSet.OnOffSampleValues[sensorIndex].LastSwitchTime));
        }          
    }
        
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].lastState = onOffSampleValueSet.OnOffSampleValues[sensorIndex].actState;               
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].actState = _state;
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].TimeFromLast = _timeFromLast.days() < 100 ? _timeFromLast : TimeSpan(0);
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].LastSwitchTime = _localTime;
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].hasToBeSent = true;
   
}

void OnOffDataContainerWio::PresetOnOffState(int sensorIndex, bool state, bool lastState, DateTime time)
{ 
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].lastState = lastState;  
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].actState = state;    
    if(time != DateTime())
    { 
        onOffSampleValueSet.OnOffSampleValues[sensorIndex].LastSwitchTime = time;
    }   
}

bool OnOffDataContainerWio::ReadOnOffState(int sensorIndex)
{
    return onOffSampleValueSet.OnOffSampleValues[sensorIndex].actState;
}

OnOffSampleValueSet OnOffDataContainerWio::GetOnOffValueSet()
{
    return onOffSampleValueSet;
}

void OnOffDataContainerWio::Reset_hasToBeSent(int sensorIndex)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].hasToBeSent = false;  
}


void OnOffDataContainerWio::Set_OutInverter(int sensorIndex, bool invertState)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].outInverter = invertState;  
}

void OnOffDataContainerWio::Set_InputInverter(int sensorIndex, bool invertState)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].inputInverter = invertState;
}


void OnOffDataContainerWio::Set_DayIsLockedFlag(int sensorIndex, bool isLockedState)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].dayIsLocked = isLockedState;
}

void OnOffDataContainerWio::Set_ResetToOnIsNeededFlag(int sensorIndex, bool state)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].resetToOnIsNeeded = state;
}

void OnOffDataContainerWio::Set_Year(int sensorIndex, int year)

{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].Year = year;
}

bool OnOffDataContainerWio::One_hasToBeBeSent(DateTime pLocalNow)
{
    // A sensor value has to be sent if either the flag is set or if a new day has arrived
    // and the actual state is 'on'
    bool ret = false;
    
    for (int i = 0; i < PROPERTY_COUNT; i++)
    { 
        ret = onOffSampleValueSet.OnOffSampleValues[i].hasToBeSent == true ? true : ret;
                     
        DateTime lastSwitchTimeDate = DateTime( onOffSampleValueSet.OnOffSampleValues[i].LastSwitchTime.year(), 
                                                onOffSampleValueSet.OnOffSampleValues[i].LastSwitchTime.month(), 
                                                onOffSampleValueSet.OnOffSampleValues[i].LastSwitchTime.day());
        
        DateTime actTimeDate = DateTime(pLocalNow.year(), pLocalNow.month(), pLocalNow.day());

        // Check if it is a new day and sensor is actually 'on'
        if ((lastSwitchTimeDate.operator!=(actTimeDate)) && (onOffSampleValueSet.OnOffSampleValues[i].actState == true))    // we have new day
        {
            ret = true;           
        }
    }
    return ret;
}

void OnOffDataContainerWio::Set_LastSwitchTime(int sensorIndex, DateTime pSwitchDateTime)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].LastSwitchTime = pSwitchDateTime;  
}

void OnOffDataContainerWio::Set_OnTimeDay(int sensorIndex, TimeSpan pOnTimeDay)
{
    onOffSampleValueSet.OnOffSampleValues[sensorIndex].OnTimeDay = pOnTimeDay;  
}



