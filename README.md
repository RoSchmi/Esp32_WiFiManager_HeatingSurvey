# Esp32_WiFiManager_HeatingSurvey

Monitor action of a heating burner by reacting on the sound of the running burner

This App for Esp32 monitors the activity of the burner of an oil-heating
(or any other noisy thing) by measuring the sound of e.g. the heating burner 
using an Adafruit I2S microphone
The on/off states are transferred to the Cloud (Azure Storage Tables)
via WLAN and can be visulized graphically through the iPhone- or Android-App
Charts4Azure  https://azureiotcharts.home.blog/

The WiFi Credentials, Azure table-name and -key and the sound threshold can be entered 
via a Portal page which is provided for one minute by the Esp32 after powering up the device.
On the first connection to the Portal page a password is needed. 
This is "My" + the SSID-Name of your ESP32 e.g. MyESP32_xxxxxx

After having entered the credentials one time they stay permanently on the Esp32 and need not to be entered every time.

This App uses an adaption of the 'Async_ConfigOnStartup.ino' and 'Async_ConfigOnSwitchFS'as WiFi Mangager
from: -https://github.com/khoih-prog/ESPAsync_WiFiManager 

More infos about the functions and Licenses see:

-https://github.com/khoih-prog/ESPAsync_WiFiManager/tree/master/examples/Async_ConfigOnStartup

-https://github.com/khoih-prog/ESPAsync_WiFiManager/tree/master/examples/Async_ConfigOnSwitchFS

![Gallery](https://github.com/RoSchmi/Esp32_WiFiManager_HeatingSurvey/blob/master/pictures/Esp32_Huzzah_I2S_Microphone.png)

For Adafruit Huzzah Esp32:

static const i2s_pin_config_t pin_config_Adafruit_Huzzah_Esp32 = {
    .bck_io_num = 14,                   // BCKL
    .ws_io_num = 15,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 32                   // DOUT
};
******************************************************************
![Gallery](https://github.com/RoSchmi/Esp32_WiFiManager_HeatingSurvey/blob/develop/pictures/ESP32%20Dev-KitC%20V4.png)

// For Esp32-Dev_KitV V4:

static const i2s_pin_config_t pin_config_Esp32_dev = {
    .bck_io_num = 26,                   // BCKL
    .ws_io_num = 25,                    // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used (only for speakers)
    .data_in_num = 22                   // DOUT
};

![Gallery](https://github.com/RoSchmi/Esp32_WiFiManager_HeatingSurvey/blob/develop/pictures/Heating_Survey.png)
