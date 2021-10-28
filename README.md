# Esp32_WiFiManager_HeatingSurvey

Monitor action of a heating burner by reacting on the sound of the running burner

This App for Esp32 monitors the activity of the burner of an oil-heating
(or any other noisy thing) by measuring the sound of e.g. the heating burner 
using an Adafruit I2S microphone
The on/off states are transferred to the Cloud (Azure Storage Tables)
via WLAN and can be visulized graphically through the iPhone- or Android-App
Charts4Azure.

The WiFi Credentials, Azure table-name and -key and the sound threshold can be entered 
via a Portal page which is provided for one minute by the Esp32 after powering up the device. 
After having entered the credentials one time they stay permanently on the Esp32 and need not to be entered every time.

This App uses an adaption of the 'Async_ConfigOnStartup.ino' and 'Async_ConfigOnSwitchFS'as WiFi Mangager
from: -https://github.com/khoih-prog/ESPAsync_WiFiManager 

More infos about the functions and Licenses see:

-https://github.com/khoih-prog/ESPAsync_WiFiManager/tree/master/examples/Async_ConfigOnStartup

-https://github.com/khoih-prog/ESPAsync_WiFiManager/tree/master/examples/Async_ConfigOnSwitchFS

![Gallery](https://github.com/RoSchmi/Esp32_WiFiManager_HeatingSurvey/blob/master/pictures/Esp32_Huzzah_I2S_Microphone.png)
