# ESP32-Homeassistant-mqtt-device-control-panel
A control panel that can control Homeassistant Mqtt devices, based on ESP32
# ESP32 LVGL Control Panel  

This control panel is built using Waveshare's 4.3-inch display, which comes with an ESP32 microcontroller. It is like a milestone project in my journey of learning and using the LVGL library. The development process took about six months from the initial stages to the first release. Due to the messy code during my learning phase, I chose not to publish the previous repository but instead started a new one for this release. Even now, the project is still a work in progress and has many bugs.  

## LVGL Configuration  
The project is configured using Espressif’s newly released LVGL display driver:  
[ESP32_Display_Panel](https://github.com/esp-arduino-libs/ESP32_Display_Panel)  
The configuration files are directly taken from the examples provided in this repository.  

## Features  
- Connects to a local MQTT server  
- Allows users to add and control MQTT devices (e.g., lights, smart plugs)  
- Supports reading MQTT device states (e.g., temperature sensors)  
- MQTT messages can be sent and received in plain text or JSON format  

## Known Issues & Future Updates  
Some functions run in different threads, and multithreading handling is not yet complete. Rapid operations may cause system crashes, which will be fixed in future updates.  

### Planned Features:  
- Multi-language support  
- Custom UI themes  
- Import/export device settings or user infomation via SD card  
- Web UI for easier device management (due to performance issues when adding devices directly on the ESP32)  

Currently, the ESP32 struggles with rendering this display smoothly, even in the official examples, the CPU usage always jumps to a very high level, so for easier to use, a web-based UI will be implemented to improve the user experience in the future.  
