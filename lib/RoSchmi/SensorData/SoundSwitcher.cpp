#include "SoundSwitcher.h"



//DateTime  lastSwitchTime = DateTime();
//TimeSpan  switchInterval = TimeSpan(60);



static const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number
static const i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
     .sample_rate = 22050,
     .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
     .intr_alloc_flags = 0, // default interrupt priority
     .dma_buf_count = 8,
     .dma_buf_len = 64,
     .use_apll = false
};

// For Adafruit Huzzah Esp32
/*
static const i2s_pin_config_t pin_config = {
    .bck_io_num = 14,                   // BCKL
    .ws_io_num = 15,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 32                   // DOUT
};
*/

// For some other Esp32 board
/*
static const i2s_pin_config_t pin_config = {
    .bck_io_num = 26,                   // BCKL
    .ws_io_num = 25,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 22                   // DOUT
};
*/

SoundSwitcher::SoundSwitcher(i2s_pin_config_t config)
{
    pin_config = config;
}

void SoundSwitcher::begin(uint16_t switchThreshold, Hysteresis pHysteresis, uint32_t updateIntervalMs, uint32_t delayTimeMs)
{ 
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);   //install and start i2s driver
  REG_SET_BIT(  I2S_TIMING_REG(i2s_num),BIT(9));   /*  #include "soc/i2s_reg.h"   I2S_NUM -> 0 or 1*/
  REG_SET_BIT( I2S_CONF_REG(i2s_num), I2S_RX_MSB_SHIFT);
  i2s_set_pin(i2s_num, &pin_config);
  
  lastFeedTimeMillis = millis();
  lastBorderNarrowTimeMillis = millis();
  feedIntervalMs = updateIntervalMs;
  threshold = (float)switchThreshold;
  hysteresis = pHysteresis;
  readDelayTimeMs = delayTimeMs;
  /*
  if (pHysteresis == Hysteresis::Percent_20)
  {
      hysteresis = 20;
  }
  else
  {
      if (pHysteresis == Hysteresis::Percent_10)
      {
          hysteresis = 10;
      }
      else
      {
          if (pHysteresis == Hysteresis::Percent_5)
          {
            hysteresis = 5;
          }
          else
          {
              if (pHysteresis == Hysteresis::Percent_2)
              {
                 hysteresis = 2;
              }
          }
      }
  }
  */
  
}

bool SoundSwitcher::hasToggled()
{
    return hasSwitched;
}

FeedResponse SoundSwitcher::feed()
{     
     FeedResponse feedResponse;
     feedResponse.isValid = false;
     feedResponse.hasToggled = false;
     feedResponse.analogToSend = false;
     feedResponse.state = false;
     feedResponse.avValue = 0.0;
     feedResponse.lowAvValue = 0.0;
     feedResponse.highAvValue = 0.0;

    if (isActive)
    {
        if (millis() - lastFeedTimeMillis > feedIntervalMs)
        { 
            lastFeedTimeMillis = millis(); 
            soundVolume = getSoundFromMicro();
            if (bufferIsFilled)
            {
                // limit the effect of short very high sound levels
                volBuffer[bufIdx] = soundVolume < (average * 4) ? soundVolume : average * 4;
            }
            else
            {
                volBuffer[bufIdx] = soundVolume;
            }
            
            bufIdx++;
            if (bufIdx >= buflen)
            {
                bufIdx = 0;
                bufferIsFilled = true;
            }
            if (bufferIsFilled)
            {   
                float mean = 0.0;
                for (int i = 0; i < buflen; i++)
                {
                    mean += volBuffer[i];
                }
                average = mean / buflen;
                if (state == false)
                {
                    if (average > threshold)
                    {
                        hasSwitched = true;
                        lastSwitchTimeMillis = millis();
                        analogSendIsPending = true;                                             
                        state = true;
                    }
                    
                }
                else     // state == true
                {
                    if (average < threshold - threshold / 100 * hysteresis)
                    {
                        hasSwitched = true;
                        lastSwitchTimeMillis = millis();
                        analogSendIsPending = true;                         
                        state = false;
                    }
                    
                }
                lowAvBorder = average < lowAvBorder ? average : lowAvBorder;
                highAvBorder = average > highAvBorder ? average : highAvBorder;

                // every 30 minutes narrow range between low and high boarder
                if (millis()- lastBorderNarrowTimeMillis > (30 * 60 * 1000))
                {
                    lastBorderNarrowTimeMillis = millis();
                    float spanPercent = (highAvBorder - lowAvBorder) / 100;
                    lowAvBorder += spanPercent;
                    highAvBorder -= spanPercent; 
                }
                feedResponse.isValid = true;
                feedResponse.hasToggled = hasSwitched;
                // RoSchmi to do
                if (analogSendIsPending)
                {
                    if (millis() - lastSwitchTimeMillis >= readDelayTimeMs)
                    {
                        feedResponse.analogToSend = true;
                        analogSendIsPending = false;
                    }    
                }
                
                feedResponse.state = state;
                feedResponse.avValue = average;
                feedResponse.lowAvValue = lowAvBorder;
                feedResponse.highAvValue = highAvBorder;

            }
        }       
    }
    hasSwitched = false;
    return feedResponse;
    
}
AverageValue SoundSwitcher::getAverage()
{
    AverageValue averageValue;
    averageValue.isValid = bufferIsFilled ? true : false;
    averageValue.value = bufferIsFilled ? average : 0.0;
    return averageValue; 
}

/*
bool SoundSwitcher::hasToggled(DateTime actUtcTime)
{
    if (isActive)
    {
        if (actUtcTime.operator>=(lastSwitchTime.operator+(switchInterval)))
        {
            lastSwitchTime = actUtcTime;
            state = !state;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}
*/

bool SoundSwitcher::GetState()
{
    return state;
}

void SoundSwitcher::SetInactive()
{
    isActive = false;
}

void SoundSwitcher::SetActive()
{
    isActive = true;
}

float SoundSwitcher::getSoundFromMicro()
{   
    int32_t audio_buf[BUFLEN];
    int bytes_read = i2s_read_bytes(i2s_num, audio_buf, sizeof(audio_buf), portMAX_DELAY);
    int32_t cleanBuf[BUFLEN / 2] {0};
    int cleanBufIdx = 0;
    for (int i = 0; i < BUFLEN; i++)
    {
      if (audio_buf[i] != 0)    // Exclude values from other channel
      {
          cleanBuf[cleanBufIdx] = audio_buf[i] >> 14;
          cleanBufIdx++;
      }
    }
    float meanval = 0;
    int volCount = 0;
    for (int i=0; i < BUFLEN / 2; i++) 
    {
         if (cleanBuf[i] != 0)
         {
          meanval += cleanBuf[i];
          volCount++;
         }
    }
    meanval /= volCount;

    // subtract it from all sapmles to get a 'normalized' output
    for (int i=0; i< volCount; i++) 
    {
        cleanBuf[i] -= meanval;
    }

    // find the 'peak to peak' max
    float maxsample, minsample;
    minsample = 100000;
    maxsample = -100000;
    for (int i=0; i<volCount; i++) {
      minsample = _min(minsample, cleanBuf[i]);
      maxsample = _max(maxsample, cleanBuf[i]);
    }
    Serial.print("Volume: ");
    Serial.println(maxsample - minsample);
    return maxsample - minsample;
}