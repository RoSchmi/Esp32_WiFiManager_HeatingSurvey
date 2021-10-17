#include <Arduino.h>
#include <Datetime.h>
#include <driver/i2s.h>
#include "soc/i2s_reg.h"

#ifndef _SOUND_SWITCHER_H_
#define _SOUND_SWITCHER_H_



typedef struct
    {
        bool isValid = false;
        float value = 0.0; 
    }
  AverageValue;

  typedef struct 
    {
        bool isValid = false;
        bool hasToggled = false;
        bool state = false;
        float avValue = 0.0;
        float lowAvValue = 100000;
        float highAvValue = -100000;
    }
  FeedResponse;

typedef  enum 
{
    Percent_0 = 0,
    Percent_2 = 2,
    Percent_5 = 5,
    Percent_10 = 10,
    Percent_20 = 20
}
Hysteresis;


class SoundSwitcher
{
    
public:
    SoundSwitcher(i2s_pin_config_t config);
    
    void begin(uint16_t switchThreshold, Hysteresis hysteresis, uint32_t updateIntervalMs);
    FeedResponse feed();
    AverageValue getAverage();
    void SetInactive();
    void SetActive();
    bool hasToggled();
    bool GetState();

private:

     i2s_pin_config_t pin_config = {
    .bck_io_num = 2,                   // BCKL
    .ws_io_num = 2,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 2                  // DOUT
};
    
    uint32_t lastFeedTimeMillis = millis();
    uint32_t lastBorderNarrowTimeMillis = millis();
    size_t feedIntervalMs = 100;
    bool hasSwitched = false;
    float getSoundFromMicro();
    float soundVolume;
    float threshold;
    int hysteresis;
    bool state = false;
    bool isActive = false;
    bool bufferIsFilled = false;
    uint32_t bufIdx = 0;
    const static int buflen = 10;
    float volBuffer[buflen] {0.0};
    float average = 0;
    float lowAvBorder = 10000;
    float highAvBorder = 0;

    #define BUFLEN 256

//static const i2s_port_t i2s_num; = I2S_NUM_0; // i2s port number
/*
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
*/

// For Adafruit Huzzah Esp32
/*
static const i2s_pin_config_t pin_config = {
    .bck_io_num = 14,                   // BCKL
    .ws_io_num = 15,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 32                   // DOUT
};
*/
};

#endif  // _SOUND_SWITCHER_H_