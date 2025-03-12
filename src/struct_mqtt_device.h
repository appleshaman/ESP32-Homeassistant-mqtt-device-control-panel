#ifndef STRUCT_MQTT_DEVICE_H
#define STRUCT_MQTT_DEVICE_H

#include "Arduino.h"
#include <string>


enum MqttDeviceType
{
    SENSOR_BINARY = 0,
    SENSOR_NON_BINARY = 1,
    DEVICE_CONTROLLABLE_BINARY = 2,
    LIGHT_BRIGHTNESS_CONTROL = 3,
    LIGHT_RGB_CONTROL = 4
};

enum MqttDeviceMessageType
{
    TRADITIONAL = 0,
    JSON = 1,
    RAW_JSON = 2,
};

enum ColorMode{
    RGB = 0,
    RGBW = 1,
};

typedef struct 
{
    MqttDeviceType type;
    MqttDeviceMessageType messageType = TRADITIONAL;
    uint8_t id = 0; //0 means id has not been set
    std::string name = "default_name";
    std::string stateTopic = "";
    std::string commandTopic = "";
    std::string stateTopicBrightness = "";
    std::string commandTopicBrightness = "";
    std::string stateTopicRgb = "";
    std::string commandTopicRgb = "";
    std::string availabilityTopic = "";
    std::string configTopic = "";
    std::string stateTopicBattery = "";
    std::string stateTopicLinkQuality = "";

    bool isAdded = false;
    // For JSON only, because some JSON device use different names for the topics
    std::string friendlyNameState = "state";
    std::string friendlyNameBrightness = "brightness";
    ColorMode friendlyNameColorMode = RGB;
    std::string friendlyNameBattery = "battery";
    std::string friendlyLinkQuality = "linkquality";
    // Rest of them are not saved
    bool isSubscribed = false;
    std::string basicMessage = "OFF"; // Corresponds to the stateTopic
    std::string brightnessMessage = "0"; // Corresponds to the stateTopicBrightness
    std::string redMessage = "255"; // Corresponds to the stateTopicRgb
    std::string greenMessage = "255"; // Corresponds to the stateTopicRgb
    std::string blueMessage = "255"; // Corresponds to the stateTopicRgb
    std::string batteryMessage = "100"; // Corresponds to the stateTopicBattery
    std::string linkQualityMessage = "100"; // Corresponds to the stateTopicLinkQuality
    

} mqtt_device_t;

#endif // STRUCT_MQTT_DEVICE_H