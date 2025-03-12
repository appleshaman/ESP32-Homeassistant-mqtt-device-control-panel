#ifndef UTILS_MQTT_DEVICE_STORE_H
#define UTILS_MQTT_DEVICE_STORE_H

#include <Arduino.h>
#include <LittleFS.h>
#include <set>
#include "struct_mqtt_device.h"
#include <functional>


#define DEBUG_FUNC_TEST 1  
#define DEBUG_FUNC_PRINT_WARNING 1  
#define DEBUG_FUNC_INITIALIZATION 0  
#define DEBUG_FUNC_READ_ALL_DEVICES 1  
#define DEBUG_FUNC_CLEAR_ALL_DEVICES 1  
#define DEBUG_FUNC_GET_ALL_DEVICES 0  

#if DEBUG_FUNC_TEST
    #define DEBUG_TEST() test()
#else
    #define DEBUG_TEST()
#endif

#if DEBUG_FUNC_PRINT_WARNING
    #define DEBUG_PRINT_WARNING(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_WARNING(...) Serial.println(__VA_ARGS__)
    #define DEBUG_SNPRINTF_WARNING(...) snprintf(__VA_ARGS__)
#else
    #define DEBUG_PRINT_WARNING(...)
    #define DEBUG_PRINTLN_WARNING(...)
    #define DEBUG_SNPRINTF_WARNING(...)
#endif

#if DEBUG_FUNC_PRINT_INITIALIZATION
    #define DEBUG_PRINT_INIT(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_INIT(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_INIT(...)
    #define DEBUG_PRINTLN_INIT(...)
#endif

#if DEBUG_FUNC_READ_ALL_DEVICES
    #define DEBUG_PRINT_READ_ALL_DEVICES(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_READ_ALL_DEVICES(...) Serial.println(__VA_ARGS__)
    #define DEBUG_PRINTF_READ_ALL_DEVICES(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT_READ_ALL_DEVICES(...)
    #define DEBUG_PRINTLN_READ_ALL_DEVICES(...)
    #define DEBUG_PRINTF_READ_ALL_DEVICES(...)
#endif

#if DEBUG_FUNC_CLEAR_ALL_DEVICES
    #define DEBUG_PRINT_CLEAR_ALL_DEVICES(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_CLEAR_ALL_DEVICES(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_CLEAR_ALL_DEVICES(...)
    #define DEBUG_PRINTLN_CLEAR_ALL_DEVICES(...)
#endif

#if DEBUG_FUNC_GET_ALL_DEVICES
    #define DEBUG_PRINT_GET_ALL_DEVICES(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_GET_ALL_DEVICES(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_GET_ALL_DEVICES(...)
    #define DEBUG_PRINTLN_GET_ALL_DEVICES(...)
#endif

#if DEBUG_FUNC2
    #define DEBUG_PRINT_F2(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_F2(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_F2(...)
    #define DEBUG_PRINTLN_F2(...)
#endif

#if DEBUG_FUNC2
    #define DEBUG_PRINT_F2(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_F2(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_F2(...)
    #define DEBUG_PRINTLN_F2(...)
#endif

#if DEBUG_FUNC2
    #define DEBUG_PRINT_F2(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_F2(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_F2(...)
    #define DEBUG_PRINTLN_F2(...)
#endif

#if DEBUG_FUNC2
    #define DEBUG_PRINT_F2(...) Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN_F2(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT_F2(...)
    #define DEBUG_PRINTLN_F2(...)
#endif




// typedef enum {
//     NONE = 0,
//     EQUIPMENT_TYPE = 1,
//     NAME = 2,
//     STATE_TOPIC = 3,
//     COMMAND_TOPIC = 4,
//     STATE_TOPIC_BRIGHTNESS = 5,
//     COMMAND_TOPIC_BRIGHTNESS = 6,
//     STATE_TOPIC_RGB = 7,
//     COMMAND_TOPIC_RGB = 8,
//     AVAILABILITY_TOPIC = 9,
//     CONFIG_TOPIC = 10,
//     STATE_TOPIC_BATTERY = 11
// } AttributeName;






void init_mqtt_device_store();
void test();

std::set<uint8_t> get_all_device_ids();
static void update_all_device_ids();

static void file_system_task(void *pvParameters);
void serialise_equipment(File &file, const mqtt_device_t &mqttDevice);
bool deserialize_equipment(File &file, mqtt_device_t &mqttDevice);

void create_new_devices();
void clear_all_device_files();

mqtt_device_t get_device();
bool update_device(mqtt_device_t mqttDevice);
bool remove_device(mqtt_device_t mqttDevice);
bool store_device(mqtt_device_t mqttDevice);
std::vector<mqtt_device_t> get_all_devices();
std::vector<std::vector<mqtt_device_t>> get_added_deivces();





#endif // UTILS_MQTT_DEVICE_STORE_H