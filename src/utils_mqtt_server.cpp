#include "utils_mqtt_server.h"
#include "wifi_utils.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Preferences.h>
#include <memory>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

const char *myName = "mqtt-device-controller";

static char *mqttServerIp;
static char *mqttServerPort;
static char *mqttUser;
static char *mqttPassword;
static char *mqttClientId;

static SemaphoreHandle_t mutexPreferenceEdit = xSemaphoreCreateBinary();

static SemaphoreHandle_t mqttConnectionMutex = xSemaphoreCreateBinary();
static Preferences preferences;

static QueueHandle_t mqttRequestQueue;

static bool autoConnectToMqttWhenStart = true;
static bool mqttStatus = false; // has to be gloabal other wise will lose information when thread is blocked

static bool isInitial = false;

static SubscriptionCallback add_device_to_subscription_list = nullptr;
static SubscriptionCallback remove_device_from_subscription_list = nullptr;

static std::function<void(const std::string &, const std::string &)> mqtt_message_callback = nullptr;

void registerMqttMessageCallback(std::function<void(const std::string &, const std::string &)> callback)
{
    mqtt_message_callback = callback;
}

void registerAddCallback(SubscriptionCallback callback)
{
    add_device_to_subscription_list = callback;
}

void registerRemoveCallback(SubscriptionCallback callback)
{
    remove_device_from_subscription_list = callback;
}

namespace
{
    enum OperationRequestType
    {
        MQTT_REQUEST_CONNECT,
        MQTT_REQUEST_DISCONNECT,
        MQTT_REQUEST_PUBLISH,
        MQTT_REQUEST_SUBSCRIBE_ALL,
        MQTT_REQUEST_ADD_DEVICE,
        MQTT_REQUEST_REMOVE_DEVICE,
        MQTT_REQUEST_SUBSCRIBE_DEVICE,
        MQTT_REQUEST_UNSUBSCRIBE_DEVICE,
        MQTT_REQUEST_CHECK_STATUS
    };

    typedef struct
    {
        OperationRequestType requestType;
        QueueHandle_t resultQueue;
        void *param;
    } operation_request_t;

    typedef struct
    {
        std::string topic;
        std::string payload;
        const mqtt_device_t &device;
    } mqtt_publish_request_t;

    typedef struct
    {
        int result_code;
    } result_t;
}

void init_mqtt_server()
{
    xSemaphoreGive(mqttConnectionMutex);

    xSemaphoreGive(mutexPreferenceEdit);

    mqttRequestQueue = xQueueCreate(10, sizeof(operation_request_t));
    if (mqttRequestQueue == NULL)
    {
        Serial.println("Failed to create queue");
        return;
    }

    store_mqtt_server_param(MQTT_PARAM_SERVER_IP, "192.168.31.119");
    store_mqtt_server_param(MQTT_PARAM_SERVER_PORT, "1883");
    store_mqtt_server_param(MQTT_PARAM_USER, "mqtt_sender");
    store_mqtt_server_param(MQTT_PARAM_PASSWORD, "1234");
    store_mqtt_server_param(MQTT_PARAM_CLIENT_ID, "esp-temperature-sensor");

    xTaskCreatePinnedToCore(mqtt_server_task, "MQTTTask", 4096, NULL, 1, NULL, 0);

    get_mqtt_server_param(MQTT_PARAM_SERVER_IP, mqttServerIp);
    get_mqtt_server_param(MQTT_PARAM_SERVER_PORT, mqttServerPort);

    client.setServer(mqttServerIp, atoi(mqttServerPort));
    client.setCallback(mqtt_callback);

    isInitial = true;

    if (autoConnectToMqttWhenStart)
    {
        xTaskCreatePinnedToCore(connect_to_mqtt_task, "ConnectToMqttTask", 1024, NULL, 1, NULL, 0);
    }
    Serial.println("inited mqtt server");
}

static void mqtt_server_task(void *pvParameters)
{
    operation_request_t request;

    while (true)
    {

        client.loop();
        if (xQueueReceive(mqttRequestQueue, &request, portMAX_DELAY) == pdTRUE)
        {
            result_t *result = new result_t();
            if (isInitial)
            {
                switch (request.requestType)
                {
                case MQTT_REQUEST_CONNECT:
                {
                    int max_retries = 3;
                    int retries = 0;
                    bool connected = false;
                    get_mqtt_server_param(MQTT_PARAM_CLIENT_ID, mqttClientId);
                    get_mqtt_server_param(MQTT_PARAM_USER, mqttUser);
                    get_mqtt_server_param(MQTT_PARAM_PASSWORD, mqttPassword);

                    while (retries < max_retries && !connected)
                    {
                        if (client.connect(mqttClientId, mqttUser, mqttPassword))
                        {
                            connected = true;
                        }
                        else
                        {
                            retries++;
                            vTaskDelay(pdMS_TO_TICKS(1000)); // retry after 1 sec
                        }
                    }
                    result->result_code = connected ? 0 : 1;
                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }

                case MQTT_REQUEST_DISCONNECT:
                {
                    client.disconnect();
                    result->result_code = 0;
                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }

                case MQTT_REQUEST_PUBLISH:
                {
                    mqtt_publish_request_t *publishRequest = static_cast<mqtt_publish_request_t *>(request.param);
                    if (!publishRequest || publishRequest->payload == "" || publishRequest->topic == "")
                    {
                        result->result_code = 1;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        break;
                    }

                    std::string formattedPayload;
                    switch (publishRequest->device.messageType)
                    {
                    case TRADITIONAL:
                        formattedPayload = publishRequest->payload;
                        break;

                    case JSON: // NOT TESTED YET
                    {
                        JsonDocument jsonDoc;

                        if (publishRequest->topic == publishRequest->device.commandTopic)
                        {
                            jsonDoc[publishRequest->device.friendlyNameState] = publishRequest->payload;
                        }
                        else if (publishRequest->topic == publishRequest->device.commandTopicBrightness)
                        {
                            jsonDoc[publishRequest->device.friendlyNameBrightness] = std::stoi(publishRequest->payload);
                        }
                        else if (publishRequest->topic == publishRequest->device.commandTopicRgb)
                        {
                            if (publishRequest->device.friendlyNameColorMode == RGB)
                            {
                                JsonObject color = jsonDoc["color"].to<JsonObject>(); // Create a nested object named "color"
                                color["r"] = std::stoi(publishRequest->device.redMessage);
                                color["g"] = std::stoi(publishRequest->device.greenMessage);
                                color["b"] = std::stoi(publishRequest->device.blueMessage);
                                jsonDoc["color_mode"] = "rgb";
                            }
                        }
                        else
                        {
                            Serial.printf("Unknown topic: %s, skipping JSON formatting\n", publishRequest->topic.c_str());
                            formattedPayload = publishRequest->payload; // Using the original payload
                            break;
                        }

                        serializeJson(jsonDoc, formattedPayload);
                        break;
                    }

                    case RAW_JSON:
                        formattedPayload = publishRequest->payload;
                        break;

                    default:
                        Serial.println("Unknown message type, publish failed.");
                        result->result_code = 1;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        break;
                    }

                    if (client.publish(publishRequest->topic.c_str(), formattedPayload.c_str()))
                    {
                        Serial.printf("Published to %s: %s\n", publishRequest->topic.c_str(), formattedPayload.c_str());
                        result->result_code = 0;
                    }
                    else
                    {
                        Serial.printf("Failed to publish to %s\n", publishRequest->topic.c_str());
                        result->result_code = 1;
                    }

                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }

                case MQTT_REQUEST_SUBSCRIBE_DEVICE:
                {
                    mqtt_device_t *equipment = static_cast<mqtt_device_t *>(request.param);
                    uint16_t subscribeResult = 0; // 9 bits

                    if (!equipment->stateTopic.empty() && client.subscribe(equipment->stateTopic.c_str()))
                    {
                        subscribeResult |= (1 << 0); // stateTopic
                    }
                    if (!equipment->stateTopicBrightness.empty() && client.subscribe(equipment->stateTopicBrightness.c_str()))
                    {
                        subscribeResult |= (1 << 1); // stateTopicBrightness
                    }
                    if (!equipment->stateTopicRgb.empty() && client.subscribe(equipment->stateTopicRgb.c_str()))
                    {
                        subscribeResult |= (1 << 2); // stateTopicRgb
                    }
                    if (!equipment->availabilityTopic.empty() && client.subscribe(equipment->availabilityTopic.c_str()))
                    {
                        subscribeResult |= (1 << 3); // availabilityTopic
                    }
                    if (!equipment->configTopic.empty() && client.subscribe(equipment->configTopic.c_str()))
                    {
                        subscribeResult |= (1 << 4); // configTopic
                    }
                    if (!equipment->stateTopicBattery.empty() && client.subscribe(equipment->stateTopicBattery.c_str()))
                    {
                        subscribeResult |= (1 << 5); // stateTopicBattery
                    }

                    result->result_code = subscribeResult;
                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }

                case MQTT_REQUEST_UNSUBSCRIBE_DEVICE:
                {
                    mqtt_device_t *equipment = static_cast<mqtt_device_t *>(request.param);
                    uint16_t unsubscribeResult = 0; // 9 bits

                    if (client.unsubscribe(equipment->stateTopic.c_str()))
                    {
                        unsubscribeResult |= (1 << 0); // stateTopic
                    }
                    if (!equipment->commandTopic.empty() && client.unsubscribe(equipment->commandTopic.c_str()))
                    {
                        unsubscribeResult |= (1 << 1); // commandTopic
                    }
                    if (!equipment->stateTopicBrightness.empty() && client.unsubscribe(equipment->stateTopicBrightness.c_str()))
                    {
                        unsubscribeResult |= (1 << 2); // stateTopicBrightness
                    }
                    if (!equipment->commandTopicBrightness.empty() && client.unsubscribe(equipment->commandTopicBrightness.c_str()))
                    {
                        unsubscribeResult |= (1 << 3); // commandTopicBrightness
                    }
                    if (!equipment->stateTopicRgb.empty() && client.unsubscribe(equipment->stateTopicRgb.c_str()))
                    {
                        unsubscribeResult |= (1 << 4); // stateTopicRgb
                    }
                    if (!equipment->commandTopicRgb.empty() && client.unsubscribe(equipment->commandTopicRgb.c_str()))
                    {
                        unsubscribeResult |= (1 << 5); // commandTopicRgb
                    }
                    if (!equipment->availabilityTopic.empty() && client.unsubscribe(equipment->availabilityTopic.c_str()))
                    {
                        unsubscribeResult |= (1 << 6); // availabilityTopic
                    }
                    if (!equipment->configTopic.empty() && client.unsubscribe(equipment->configTopic.c_str()))
                    {
                        unsubscribeResult |= (1 << 7); // configTopic
                    }
                    if (!equipment->stateTopicBattery.empty() && client.unsubscribe(equipment->stateTopicBattery.c_str()))
                    {
                        unsubscribeResult |= (1 << 8); // stateTopicBattery
                    }

                    result->result_code = unsubscribeResult;
                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }

                case MQTT_REQUEST_CHECK_STATUS:
                {
                    if (client.connected())
                    {
                        result->result_code = 0;
                        mqttStatus = true;
                    }
                    else
                    {
                        result->result_code = 1;
                        mqttStatus = false;
                    }

                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }
                }
            }
            else
            {
                result->result_code = 0xFFFF;
                xQueueSend(request.resultQueue, &result, portMAX_DELAY);
            }
        }
    }
}

static void connect_to_mqtt_task(void *param)
{
    connect_to_mqtt();
    vTaskDelete(NULL);
}

bool connect_to_mqtt()
{

    if (is_connected_to_mqtt_server())
    {
        return true;
    }
    if (xSemaphoreTake(mqttConnectionMutex, 0) == pdTRUE)
    {
        QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
        operation_request_t request = {MQTT_REQUEST_CONNECT, responseQueue, nullptr};
        xQueueSend(mqttRequestQueue, &request, portMAX_DELAY);
        result_t *result;
        xQueueReceive(responseQueue, &result, portMAX_DELAY);
        auto freeData = [&]()
        {
            delete result;
            vQueueDelete(responseQueue);
        };
        if (result != nullptr)
        {
            if (result->result_code == 0)
            {
                Serial.println("MQTT server connected");
                freeData();
                xSemaphoreGive(mqttConnectionMutex);
                return true;
            }
            if (result->result_code == 1)
            {
                Serial.println("Conect to MQTT server failed");
                freeData();
                xSemaphoreGive(mqttConnectionMutex);
                return false;
            }
            if (result->result_code == 0xFFFF)
            {
                Serial.println("Please initialise the mqtt server functions first");
                freeData();
                xSemaphoreGive(mqttConnectionMutex);
                return false;
            }
        }
        freeData();
        xSemaphoreGive(mqttConnectionMutex);
        return false;
    }
    return false;
}

bool disconnect_from_mqtt()
{
    if (!is_connected_to_mqtt_server())
    {
        return true;
    }
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {MQTT_REQUEST_DISCONNECT, responseQueue, nullptr};
    xQueueSend(mqttRequestQueue, &request, portMAX_DELAY);

    result_t *result;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    auto freeData = [&]()
    {
        delete result;
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        if (result->result_code == 0)
        {
            Serial.println("MQTT server disconnected");
            freeData();
            return true;
        }
        if (result->result_code == 0xFFFF)
        {
            Serial.println("Please initialise the mqtt server functions first");
            freeData();
            return false;
        }
    }
    freeData();
    return false;
}

bool publish_to_mqtt(const std::string &topic, const std::string &payload, const mqtt_device_t &device)
{
    mqtt_publish_request_t publishRequest = {topic, payload, device};
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {MQTT_REQUEST_PUBLISH, responseQueue, reinterpret_cast<void *>(&publishRequest)};
    xQueueSend(mqttRequestQueue, &request, portMAX_DELAY);

    result_t *result;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    auto freeData = [&]()
    {
        delete result;
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        if (result->result_code == 0)
        {
            Serial.println("Message published");
            freeData();
            return true;
        }
        if (result->result_code == 1)
        {
            Serial.println("Message publish failed");
            freeData();
            return false;
        }
        if (result->result_code == 0xFFFF)
        {
            Serial.println("Please initialise the mqtt server functions first");
            freeData();
            return false;
        }
    }
    freeData();
    return false;
}

bool is_connected_to_mqtt_server()
{
    const TickType_t timeout = pdMS_TO_TICKS(300);

    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t));
    operation_request_t request = {MQTT_REQUEST_CHECK_STATUS, responseQueue, nullptr};
    xQueueSend(mqttRequestQueue, &request, timeout);

    result_t *result;
    xQueueReceive(responseQueue, &result, timeout);
    auto freeData = [&]()
    {
        delete result;
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        if (result->result_code == 0)
        {
            // Serial.println("MQTT server connected");
            freeData();
            return true;
        }
        if (result->result_code == 1)
        {
            // Serial.println("MQTT server not connect");
            freeData();
            return false;
        }
        if (result->result_code == 0xFFFF)
        {
            Serial.println("Please initialise the mqtt server functions first");
            freeData();
            return false;
        }
    }
    freeData();
    return false;
}

bool subscribe_mqtt_device(const mqtt_device_t &device)
{
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {MQTT_REQUEST_SUBSCRIBE_DEVICE, responseQueue, (void *)&device};

    // Send Request
    xQueueSend(mqttRequestQueue, &request, portMAX_DELAY);

    // Receive results
    result_t *result;
    if (xQueueReceive(responseQueue, &result, portMAX_DELAY) == pdTRUE && result != nullptr)
    {
        uint16_t subscribeResult = static_cast<uint16_t>(result->result_code);
        delete result;
        vQueueDelete(responseQueue);

        if (result->result_code == 0xFFFF)
        {
            Serial.println("Please initialise the mqtt server functions first");
            return false;
        }
        if (result->result_code == 0)
        {
            Serial.printf("Device [%s] has no topic been subscribed\n", device.name.c_str());
            return false;
        }
        if (!device.stateTopic.empty())
        {
            if (subscribeResult & (1 << 0))
                Serial.printf("Device [%s]: stateTopic subscription success\n", device.name.c_str());
            else
                Serial.printf("Device [%s]: stateTopic subscription failed\n", device.name.c_str());
        }

        if (!device.stateTopicBrightness.empty())
        {
            if (subscribeResult & (1 << 1))
                Serial.printf("Device [%s]: stateTopicBrightness subscription success\n", device.name.c_str());
            else
                Serial.printf("Device [%s]: stateTopicBrightness subscription failed\n", device.name.c_str());
        }

        if (!device.stateTopicRgb.empty())
        {
            if (subscribeResult & (1 << 2))
                Serial.printf("Device [%s]: stateTopicRgb subscription success\n", device.name.c_str());
            else
                Serial.printf("Device [%s]: stateTopicRgb subscription failed\n", device.name.c_str());
        }

        if (!device.availabilityTopic.empty())
        {
            if (subscribeResult & (1 << 3))
                Serial.printf("Device [%s]: availabilityTopic subscription success\n", device.name.c_str());
            else
                Serial.printf("Device [%s]: availabilityTopic subscription failed\n", device.name.c_str());
        }

        if (!device.configTopic.empty())
        {
            if (subscribeResult & (1 << 4))
                Serial.printf("Device [%s]: configTopic subscription success\n", device.name.c_str());
            else
                Serial.printf("Device [%s]: configTopic subscription failed\n", device.name.c_str());
        }

        if (!device.stateTopicBattery.empty())
        {
            if (subscribeResult & (1 << 5))
                Serial.printf("Device [%s]: stateTopicBattery subscription success\n", device.name.c_str());
            else
                Serial.printf("Device [%s]: stateTopicBattery subscription failed\n", device.name.c_str());
        }

        // At least one topic has been subscribed
        if (add_device_to_subscription_list)
        {
            add_device_to_subscription_list(device.id);
            Serial.println("Added to subscribe list");
        }
        return true;
    }

    Serial.println("Failed to receive subscription results");
    vQueueDelete(responseQueue);
    return false;
}

bool unsubscribe_mqtt_device(const mqtt_device_t &device)
{
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {MQTT_REQUEST_UNSUBSCRIBE_DEVICE, responseQueue, (void *)&device};

    // Send Request
    xQueueSend(mqttRequestQueue, &request, portMAX_DELAY);

    // Receive results
    result_t *result;
    if (xQueueReceive(responseQueue, &result, portMAX_DELAY) == pdTRUE && result != nullptr)
    {
        uint16_t subscribeResult = static_cast<uint16_t>(result->result_code); // 9位订阅结果
        delete result;
        vQueueDelete(responseQueue);

        if (result->result_code == 0xFFFF)
        {
            Serial.println("Please initialise the mqtt server functions first");
            return false;
        }

        if (subscribeResult & (1 << 0))
            Serial.printf("Device [%s]: stateTopic unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: stateTopic unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 1))
            Serial.printf("Device [%s]: commandTopic unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: commandTopic unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 2))
            Serial.printf("Device [%s]: stateTopicBrightness unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: stateTopicBrightness unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 3))
            Serial.printf("Device [%s]: commandTopicBrightness unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: commandTopicBrightness unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 4))
            Serial.printf("Device [%s]: stateTopicRgb unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: stateTopicRgb unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 5))
            Serial.printf("Device [%s]: commandTopicRgb unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: commandTopicRgb unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 6))
            Serial.printf("Device [%s]: availabilityTopic unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: availabilityTopic unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 7))
            Serial.printf("Device [%s]: configTopic unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: configTopic unsubscription failed\n", device.name.c_str());

        if (subscribeResult & (1 << 8))
            Serial.printf("Device [%s]: stateTopicBattery unsubscription success\n", device.name.c_str());
        else
            Serial.printf("Device [%s]: stateTopicBattery unsubscription failed\n", device.name.c_str());

        // At least one topic has been unsubscribed
        if (remove_device_from_subscription_list)
        {
            remove_device_from_subscription_list(device.id);
            Serial.println("Removed from subscribe list");
        }
        return true;
    }

    Serial.println("Failed to receive unsubscription results");
    vQueueDelete(responseQueue);
    return false;
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    // Parse the payload to a string
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    if (mqtt_message_callback)
    {
        mqtt_message_callback(topic, message.c_str());
    }
}

bool store_mqtt_server_param(MqttParam param, const char *value)
{
    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        if (!preferences.begin("mqtt", false))
        {
            return false;
        }
        switch (param)
        {
        case MQTT_PARAM_SERVER_IP:
            if (!(strlen(value) ==
                  preferences.putString("serverIp", value)))
            {
                return false;
            };
            break;
        case MQTT_PARAM_SERVER_PORT:
            if (!(strlen(value) ==
                  preferences.putString("serverPort", value)))
            {
                return false;
            };
            break;
        case MQTT_PARAM_USER:
            if (!(strlen(value) ==
                  preferences.putString("user", value)))
            {
                return false;
            };
            break;
        case MQTT_PARAM_PASSWORD:
            if (!(strlen(value) ==
                  preferences.putString("password", value)))
            {
                return false;
            };
            break;
        case MQTT_PARAM_CLIENT_ID:
            if (!(strlen(value) ==
                  preferences.putString("clientId", value)))
            {
                return false;
            };
            break;
        }
        preferences.end();
        xSemaphoreGive(mutexPreferenceEdit);
        return true;
    }
    return false;
}

void get_mqtt_server_param(MqttParam param, char *&value)
{
    String storedValue;
    storedValue = get_mqtt_server_param(param);

    if (storedValue.length() > 0)
    {
        value = strdup(storedValue.c_str());
    }
}

String get_mqtt_server_param(MqttParam param)
{
    String storedValue;

    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        preferences.begin("mqtt", false);
        switch (param)
        {
        case MQTT_PARAM_SERVER_IP:
            storedValue = preferences.getString("serverIp", "");
            break;
        case MQTT_PARAM_SERVER_PORT:
            storedValue = preferences.getString("serverPort", "");
            break;
        case MQTT_PARAM_USER:
            storedValue = preferences.getString("user", "");
            break;
        case MQTT_PARAM_PASSWORD:
            storedValue = preferences.getString("password", "");
            break;
        case MQTT_PARAM_CLIENT_ID:
            storedValue = preferences.getString("clientId", "");
            break;
        }
        preferences.end();
    }
    xSemaphoreGive(mutexPreferenceEdit);

    return storedValue;
}
