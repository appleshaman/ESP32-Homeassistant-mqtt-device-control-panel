#ifndef UTILS_MQTT_SERVER_H
#define UTILS_MQTT_SERVER_H

#include "Arduino.h"
#include "struct_mqtt_device.h"


enum MqttParam
{
    MQTT_PARAM_SERVER_IP,
    MQTT_PARAM_SERVER_PORT,
    MQTT_PARAM_USER,
    MQTT_PARAM_PASSWORD,
    MQTT_PARAM_CLIENT_ID
};

typedef void (*SubscriptionCallback)(uint8_t id);
void registerMqttMessageCallback(std::function<void(const std::string &, const std::string &)> callback);
void registerAddCallback(SubscriptionCallback callback);
void registerRemoveCallback(SubscriptionCallback callback);
void init_mqtt_server();
static void mqtt_server_task(void *param);
bool connect_to_mqtt();
bool disconnect_from_mqtt();
static void connect_to_mqtt_task(void *param);
bool publish_to_mqtt(const std::string &topic, const std::string &payload,  const mqtt_device_t &device);
bool is_connected_to_mqtt_server();
bool subscribe_mqtt_device(const mqtt_device_t &device);
bool unsubscribe_mqtt_device(const mqtt_device_t &device);
void mqtt_callback(char *p_topic, byte *p_payload, unsigned int p_length);

bool store_mqtt_server_param(MqttParam param, const char *value);
void get_mqtt_server_param(MqttParam param, char *&value);
String get_mqtt_server_param(MqttParam param);

#endif // UTILS_MQTT_SERVER_H