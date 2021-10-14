#include <Arduino.h>
#include <Datetime.h>

#ifndef _ON_OFF_SWITCHER_H_
#define _ON_OFF_SWITCHER_H_

class OnOffSwitcherWio
{
public:
    OnOffSwitcherWio();
    
    void begin(TimeSpan interval);

    void SetInactive();
    void SetActive();
    bool hasToggled(DateTime actUtcTime);
    bool GetState();

private:
    bool state = false;
    bool isActive = false;
};

#endif  // _ON_OFF_SWITCHER_H_