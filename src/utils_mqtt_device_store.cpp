#include "utils_mqtt_device_store.h"

QueueHandle_t fileSystemRequestQueue;
static bool isInitial = false;
static std::set<uint8_t> existingIds;
static SemaphoreHandle_t mqttIdReadingMutex = xSemaphoreCreateBinary();
static SemaphoreHandle_t mqttIdReadingManipulationMutex = xSemaphoreCreateBinary(); // prevent device id changes at the same time, e.g. add or remove can cause id changes

namespace
{
    typedef enum
    {
        STORE_DEVICE,
        REMOVE_DEVICE,
        UPDATE_DEVICE,
        GET_DEVICE,
        GET_ALL_DEVICES,
        GET_ADDED_DEVICES
    } OperationRequestType;

    typedef struct
    {
        OperationRequestType requestType;
        mqtt_device_t mqttDevice;
        QueueHandle_t resultQueue;
        void *param;
    } operation_request_t;

    typedef struct
    {
        int result_code;
        std::vector<mqtt_device_t> *devices;
        std::vector<std::vector<mqtt_device_t>> *devices_mutiple_lines;
    } result_t;
}

void init_mqtt_device_store()
{

    if (!LittleFS.begin(true, "/littlefs"))
    {
        DEBUG_PRINTLN_WARNING("Failed to mount file system");
        return;
    }

    if (!LittleFS.exists("/mqtt_devices"))
    {
        if (LittleFS.mkdir("/mqtt_devices"))
        {
            DEBUG_PRINTLN_INIT("/mqtt_devices directory created successfully");
        }
        else
        {
            DEBUG_PRINTLN_WARNING("Failed to create /mqtt_devices directory");
        }
    }

    // Create a queue for file system requests
    fileSystemRequestQueue = xQueueCreate(10, sizeof(operation_request_t));
    if (fileSystemRequestQueue == NULL)
    {
        DEBUG_PRINTLN_WARNING("Failed to create queue");
        return;
    }

    // Release the id reading function mutex
    xSemaphoreGive(mqttIdReadingMutex);
    // Release mutex to allow doing the manipulation of device storing and removing
    xSemaphoreGive(mqttIdReadingManipulationMutex);
    // Get existing device IDs
    update_all_device_ids();
    // Start the file system task
    xTaskCreate(file_system_task, "FileSystemTask", 8192, NULL, 1, NULL);

    isInitial = true;
    Serial.println("mqtt store initialised");
    test();
}

std::set<uint8_t> get_all_device_ids()
{
    if (xSemaphoreTake(mqttIdReadingMutex, 0) == pdTRUE)
    {
        return existingIds;
    }
    else
    {
        DEBUG_PRINTLN_WARNING("Failed to get the id mutex");
    }
    return {};
}

static void update_all_device_ids()
{
    if (xSemaphoreTake(mqttIdReadingMutex, 0) == pdTRUE)
    {
        Serial.println("Updating all device ids");
        std::set<uint8_t> ids;

        // 打开设备目录
        File root = LittleFS.open("/mqtt_devices/");
        if (!root || !root.isDirectory())
        {
            DEBUG_PRINTLN_WARNING("Failed to open /mqtt_devices directory");
            vTaskDelete(NULL);
            return;
        }
        // 遍历目录中的所有文件，读取每个设备的 ID
        File file = root.openNextFile();
        while (file)
        {
            mqtt_device_t mqttDevice;
            if (deserialize_equipment(file, mqttDevice))
            {
                if (mqttDevice.id != 0)
                {
                    ids.insert(mqttDevice.id);
                    DEBUG_PRINTF_READ_ALL_DEVICES("Read device ID: %d\n", (unsigned int)mqttDevice.id);
                }
            }
            file.close();
            file = root.openNextFile();
        }
        root.close();

        // 更新全局 ID 集合
        existingIds = ids;

        // 设置标志表示所有 ID 已读取完毕
        xSemaphoreGive(mqttIdReadingMutex);
        // 删除当前任务
        DEBUG_PRINTLN_READ_ALL_DEVICES("Device id read finished");
    }
    else
    {
        DEBUG_PRINTLN_WARNING("Failed to get the id mutex");
    }
}

static void file_system_task(void *pvParameters)
{
    operation_request_t request;

    while (true)
    {
        if (xQueueReceive(fileSystemRequestQueue, &request, portMAX_DELAY) == pdTRUE)
        {
            result_t *result = new result_t();
            if (isInitial)
            {
                auto readDevicesWithFilter = [&](std::function<bool(const mqtt_device_t &)> filter, int &result_code)
                {
                    std::vector<mqtt_device_t> devices;
                    File root = LittleFS.open("/mqtt_devices/");

                    if (!root || !root.isDirectory())
                    {
                        DEBUG_PRINTLN_WARNING("Failed to open /mqtt_devices directory");
                        result_code = 2;
                        return devices;
                    }

                    File file = root.openNextFile();
                    while (file)
                    {
                        mqtt_device_t mqttEquipment;
                        if (deserialize_equipment(file, mqttEquipment))
                        {
                            if (filter(mqttEquipment))
                            {
                                devices.push_back(mqttEquipment);
                            }
                        }
                        else
                        {
                            Serial.println("Failed to deserialize equipment");
                        }

                        file.close();
                        file = root.openNextFile();
                    }

                    root.close();
                    return devices;
                };

                switch (request.requestType)
                {

                case STORE_DEVICE:
                {
                    if (xSemaphoreTake(mqttIdReadingManipulationMutex, portMAX_DELAY) == pdTRUE)
                    {
                        if (LittleFS.exists(("/mqtt_devices/" + request.mqttDevice.name + "_" + (String(request.mqttDevice.id).c_str()) + ".txt").c_str()))
                        { // Device already exist
                            result->result_code = 1;
                            xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                            xSemaphoreGive(mqttIdReadingManipulationMutex);
                            break;
                        }

                        File file = LittleFS.open(("/mqtt_devices/" + request.mqttDevice.name + "_" + (String(request.mqttDevice.id).c_str()) + ".txt").c_str(), FILE_WRITE);
                        if (!file)
                        { // Failed to get the created file
                            String filePath = ("/mqtt_devices/" + request.mqttDevice.name + "_" + (String(request.mqttDevice.id).c_str()) + ".txt").c_str();
                            file.close();
                            result->result_code = 2;
                            xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                            xSemaphoreGive(mqttIdReadingManipulationMutex);
                            break;
                        }
                        serialise_equipment(file, request.mqttDevice);
                        file.close();
                        result->result_code = 0; // Success to save data into the file
                        update_all_device_ids();
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        xSemaphoreGive(mqttIdReadingManipulationMutex);
                        break;
                    }
                    result->result_code = 3;
                    xQueueSend(request.resultQueue, &result, portMAX_DELAY); // Failed to get the mutex
                    break;
                }

                case REMOVE_DEVICE:
                {

                    if (xSemaphoreTake(mqttIdReadingManipulationMutex, portMAX_DELAY) == pdTRUE)
                    {
                        if (!LittleFS.exists(("/mqtt_devices/" + request.mqttDevice.name + "_" + (String(request.mqttDevice.id).c_str()) + ".txt").c_str()))
                        {
                            result->result_code = 1;
                        }
                        else if (LittleFS.remove(("/mqtt_devices/" + request.mqttDevice.name + "_" + (String(request.mqttDevice.id).c_str()) + ".txt").c_str()))
                        {
                            result->result_code = 0;
                            update_all_device_ids();
                            xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                            break;
                        }
                        else
                        {
                            result->result_code = 2;
                        }
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        xSemaphoreGive(mqttIdReadingManipulationMutex);
                        break;
                    }
                    else
                    {
                        result->result_code = 3;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        break;
                    }

                    result->result_code = 4;
                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    break;
                }

                case UPDATE_DEVICE:
                {
                    bool fileFound = false;

                    // Open the dir
                    File root = LittleFS.open("/mqtt_devices/");
                    if (!root || !root.isDirectory())
                    {
                        result->result_code = 1; // Failed to open the dir
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        break;
                    }

                    // Search for the file
                    File file = root.openNextFile();
                    while (file)
                    {
                        mqtt_device_t existingEquipment = request.mqttDevice;
                        if (deserialize_equipment(file, existingEquipment) && existingEquipment.id == request.mqttDevice.id)
                        {
                            fileFound = true;
                            std::string oldFilePath = std::string("/mqtt_devices/") + file.name();
                            file.close();
                            std::string newFilePath;
                            // Check if the name has been changed
                            if (existingEquipment.name != request.mqttDevice.name)
                            { // Check if the new name already exists

                                newFilePath = "/mqtt_devices/" + request.mqttDevice.name + "_" + std::to_string(request.mqttDevice.id) + ".txt";

                                if (!LittleFS.rename(oldFilePath.c_str(), newFilePath.c_str()))
                                {                            // Rename the file
                                    result->result_code = 2; // Failed to rename
                                    xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                                    break;
                                }
                            }
                            else
                            {
                                newFilePath = oldFilePath;
                            }
                            // update the file
                            Serial.println(newFilePath.c_str());
                            File updatedFile = LittleFS.open(newFilePath.c_str(), "w");
                            if (!updatedFile)
                            {
                                result->result_code = 3; // Failed to open file
                                xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                                break;
                            }
                            serialise_equipment(updatedFile, request.mqttDevice);
                            updatedFile.close();

                            result->result_code = 0; // Update success
                            xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                            break;
                        }
                        file = root.openNextFile();
                    }

                    root.close();

                    if (!fileFound)
                    {
                        result->result_code = 4; // Device ID not found
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    }

                    break;
                }

                case GET_DEVICE:
                {

                    // // 获取设备文件路径
                    // std::string filePath = "/mqtt_devices/" + request.mqttEquipment.name + ".dat";
                    // int *result = (int *)malloc(sizeof(result_t));
                    // // 打开设备文件
                    // File file = LittleFS.open(filePath.c_str(), "r");
                    // if (!file)
                    // {
                    //     Serial.println("Failed to open device file");
                    //     *result = 1; // 文件不存在
                    //     xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    //     break;
                    // }

                    // // 反序列化设备数据
                    // mqtt_equipment_t mqttEquipment;
                    // if (deserialize_equipment(file, mqttEquipment))
                    // {
                    //     request.mqttEquipment = mqttEquipment; // 将反序列化后的设备数据返回
                    //     Serial.println("Successfully deserialized equipment");
                    //     *result = 0; // 成功
                    // }
                    // else
                    // {
                    //     Serial.println("Failed to deserialize equipment data");
                    //     *result = 2; // 反序列化失败
                    // }

                    // file.close(); // 关闭文件

                    // // 返回设备数据或错误码给请求方
                    // xQueueSend(request.resultQueue, &result, portMAX_DELAY);

                    break;
                }

                case GET_ALL_DEVICES:
                {
                    if (xSemaphoreTake(mqttIdReadingManipulationMutex, portMAX_DELAY) == pdTRUE)
                    {
                        std::vector<mqtt_device_t> *devices = new std::vector<mqtt_device_t>();
                        int result_code = 0;

                        *devices = readDevicesWithFilter([](const mqtt_device_t &)
                                                         { return true; }, result_code);

                        if (result_code == 2)
                        {
                            result->result_code = 2;
                        }
                        else if (devices->empty())
                        {
                            result->result_code = 1;
                        }
                        else
                        {
                            result->result_code = 0;
                        }

                        result->devices = devices;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        xSemaphoreGive(mqttIdReadingManipulationMutex);
                        break;
                    }
                    else
                    {
                        DEBUG_PRINTLN_WARNING("Failed to acquire mutex for GET_ALL_DEVICES");
                        result->result_code = 6;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    }
                    break;
                }
                case GET_ADDED_DEVICES:
                {
                    if (xSemaphoreTake(mqttIdReadingManipulationMutex, portMAX_DELAY) == pdTRUE)
                    {
                        std::vector<std::vector<mqtt_device_t>> *devicesMultipleLines = new std::vector<std::vector<mqtt_device_t>>();
                        std::vector<mqtt_device_t> currentLine;
                        int result_code = 0;

                        auto devices = readDevicesWithFilter([](const mqtt_device_t &device)
                                                             { return device.isAdded; }, result_code);

                        if (result_code == 2)
                        {
                            result->result_code = 2; // Failed to open /mqtt_devices/
                        }
                        else
                        {
                            for (const auto &device : devices)
                            {
                                currentLine.push_back(device);
                                if (currentLine.size() == 4)
                                {
                                    devicesMultipleLines->push_back(currentLine);
                                    currentLine.clear();
                                }
                            }

                            if (!currentLine.empty())
                            {
                                devicesMultipleLines->push_back(currentLine);
                            }

                            result->result_code = devicesMultipleLines->empty() ? 1 : 0; // 1 - no data, 0 - success
                        }

                        result->devices_mutiple_lines = devicesMultipleLines;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                        xSemaphoreGive(mqttIdReadingManipulationMutex);
                    }
                    else
                    {
                        DEBUG_PRINTLN_WARNING("Failed to acquire mutex for GET_ADDED_DEVICES");
                        result->result_code = 6;
                        xQueueSend(request.resultQueue, &result, portMAX_DELAY);
                    }

                    break;
                }
                }
            }
            else
            {
                result->result_code = 5;
                xQueueSend(request.resultQueue, &result, portMAX_DELAY);
            }
        }
    }
}

// Clear all device files
void clear_all_device_files()
{
    // Open /mqtt_devices/
    File root = LittleFS.open("/mqtt_devices/");
    if (!root || !root.isDirectory())
    {
        DEBUG_PRINTLN_WARNING("Failed to open /mqtt_devices/ directory or it does not exist");
        return;
    }
    // Get the list of files in the directory
    File file = root.openNextFile();
    while (file)
    {
        // Get the file path
        String filePath = String("/mqtt_devices/") + String(file.name());

        // Print the file path
        DEBUG_PRINT_CLEAR_ALL_DEVICES("Deleting file: ");
        DEBUG_PRINTLN_CLEAR_ALL_DEVICES(filePath);

        // Close the file
        file.close();

        // Delete the file
        if (LittleFS.remove(filePath.c_str()))
        {
            DEBUG_PRINTLN_CLEAR_ALL_DEVICES("File deleted successfully");
        }
        else
        {
            DEBUG_PRINTLN_WARNING("Failed to delete file");
        }

        // Open the next file
        file = root.openNextFile();
    }
    root.close();
}

void serialise_equipment(File &file, const mqtt_device_t &mqttEquipment)
{
    // Write the equipment type (stored as an integer)
    file.write((uint8_t *)&mqttEquipment.type, sizeof(mqttEquipment.type));
    // Write the message type (stored as an integer)
    file.write((uint8_t *)&mqttEquipment.messageType, sizeof(mqttEquipment.messageType));
    // Helper function of writing the string length and content
    auto writeString = [&](const std::string &str)
    {
        uint32_t length = str.size();
        file.write((uint8_t *)&length, sizeof(length)); // Write the string length
        if (length > 0)
        {
            file.write((const uint8_t *)str.c_str(), length); // Write the string content
        }
    };

    // Write the required fields
    writeString(String(mqttEquipment.id).c_str());
    writeString(mqttEquipment.name);
    writeString(mqttEquipment.stateTopic);

    // Write the optional fields (based on the device type)
    if (mqttEquipment.type != SENSOR_BINARY && mqttEquipment.type != SENSOR_NON_BINARY)
    {
        writeString(mqttEquipment.commandTopic);
    }
    if (mqttEquipment.type == LIGHT_BRIGHTNESS_CONTROL || mqttEquipment.type == LIGHT_RGB_CONTROL)
    {
        writeString(mqttEquipment.stateTopicBrightness);
        writeString(mqttEquipment.commandTopicBrightness);
        if (mqttEquipment.messageType == JSON || mqttEquipment.messageType == RAW_JSON)
        {
            writeString(mqttEquipment.friendlyNameBrightness);
        }
    }
    if (mqttEquipment.type == LIGHT_RGB_CONTROL)
    {
        writeString(mqttEquipment.stateTopicRgb);
        writeString(mqttEquipment.commandTopicRgb);
        if (mqttEquipment.messageType == JSON || mqttEquipment.messageType == RAW_JSON)
        {
            file.write((uint8_t *)&mqttEquipment.friendlyNameColorMode, sizeof(mqttEquipment.friendlyNameColorMode));
        }
    }

    writeString(mqttEquipment.availabilityTopic);
    writeString(mqttEquipment.configTopic);
    writeString(mqttEquipment.stateTopicBattery);
    writeString(mqttEquipment.stateTopicLinkQuality);

    file.write((uint8_t *)&mqttEquipment.isAdded, sizeof(mqttEquipment.isAdded));

    if (mqttEquipment.messageType == JSON || mqttEquipment.messageType == RAW_JSON)
    {
        writeString(mqttEquipment.friendlyNameState);
        writeString(mqttEquipment.friendlyNameBattery);
        writeString(mqttEquipment.friendlyLinkQuality);
    }
}

// Deserialize equipment data from a file
bool deserialize_equipment(File &file, mqtt_device_t &mqttEquipment)
{
    // Initialise the structure to default values
    mqttEquipment = {};

    // Read the device type
    file.read((uint8_t *)&mqttEquipment.type, sizeof(mqttEquipment.type));
    // Read the message type
    file.read((uint8_t *)&mqttEquipment.messageType, sizeof(mqttEquipment.messageType));

    // Helper function of reading the string field
    auto readString = [&](std::string &str)
    {
        uint32_t length;
        if (file.read((uint8_t *)&length, sizeof(length)) != sizeof(length))
        {
            DEBUG_PRINTLN_WARNING("Failed to read string length");
            return false;
        }

        if (length > 1000)
        {
            DEBUG_PRINTLN_WARNING("String length is too large");
            return false;
        }

        if (length > 0)
        {
            char *buffer = new (std::nothrow) char[length + 1];
            if (!buffer)
            {
                DEBUG_PRINTLN_WARNING("Memory allocation failed");
                return false;
            }

            if (file.read((uint8_t *)buffer, length) != length)
            {
                DEBUG_PRINTLN_WARNING("Failed to read string content");
                delete[] buffer;
                return false;
            }

            buffer[length] = '\0';
            str = buffer;
            delete[] buffer;
        }
        else
        {
            str.clear(); // 长度为 0，设置为空字符串
        }

        return true;
    };

    // Read required fields
    std::string idStr;
    if (!readString(idStr))
        return false;
    mqttEquipment.id = static_cast<uint8_t>(std::stoi(idStr));

    if (!readString(mqttEquipment.name))
        return false;

    if (!readString(mqttEquipment.stateTopic))
        return false;

    // Read optional fields (based on the device type)
    if (mqttEquipment.type != SENSOR_BINARY && mqttEquipment.type != SENSOR_NON_BINARY)
    {
        readString(mqttEquipment.commandTopic);
    }
    if (mqttEquipment.type == LIGHT_BRIGHTNESS_CONTROL || mqttEquipment.type == LIGHT_RGB_CONTROL)
    {
        readString(mqttEquipment.stateTopicBrightness);
        readString(mqttEquipment.commandTopicBrightness);
        if (mqttEquipment.messageType == JSON || mqttEquipment.messageType == RAW_JSON)
        {
            readString(mqttEquipment.friendlyNameBrightness);
        }
    }
    if (mqttEquipment.type == LIGHT_RGB_CONTROL)
    {
        readString(mqttEquipment.stateTopicRgb);
        readString(mqttEquipment.commandTopicRgb);
        if (mqttEquipment.messageType == JSON || mqttEquipment.messageType == RAW_JSON)
        {
            file.read((uint8_t *)&mqttEquipment.friendlyNameColorMode, sizeof(mqttEquipment.friendlyNameColorMode));
        }
    }

    readString(mqttEquipment.availabilityTopic);
    readString(mqttEquipment.configTopic);
    readString(mqttEquipment.stateTopicBattery);
    readString(mqttEquipment.stateTopicLinkQuality);

    // Read the isAdded field
    if (file.read((uint8_t *)&mqttEquipment.isAdded, sizeof(mqttEquipment.isAdded)) != sizeof(mqttEquipment.isAdded))
    {
        mqttEquipment.isAdded = false;
        DEBUG_PRINTLN_WARNING("Failed to read isAdded, using default false");
    }

    if (mqttEquipment.messageType == JSON || mqttEquipment.messageType == RAW_JSON)
    {
        readString(mqttEquipment.friendlyNameState);
        readString(mqttEquipment.friendlyNameBattery);
        readString(mqttEquipment.friendlyLinkQuality);
    }

    return true;
}

bool store_device(mqtt_device_t mqttEquipment)
{
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {STORE_DEVICE, mqttEquipment, responseQueue, nullptr};
    xQueueSend(fileSystemRequestQueue, &request, portMAX_DELAY);

    result_t *result;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    char *response_message = (char *)malloc(100 * sizeof(char));

    auto freeData = [&]()
    {
        free(result->devices);
        delete result;
        free(response_message);
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        if (result->result_code == 0)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Device %s added successfully", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return true;
        }
        else if (result->result_code == 1)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Device %s already exist", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return false;
        }
        else if (result->result_code == 2)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Failed to store device %s", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return false;
        }
        else if (result->result_code == 3)
        {
            DEBUG_PRINTLN_WARNING("Failed to store, cannot get the mutex");
            freeData();
            return false;
        }
        else if (result->result_code == 5)
        {
            DEBUG_PRINTLN_WARNING("Please initialise the mqtt storage function first");
            freeData();
            return false;
        }
    }
    freeData();
    return false;
}

bool remove_device(mqtt_device_t mqttEquipment)
{
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {REMOVE_DEVICE, mqttEquipment, responseQueue, nullptr};
    xQueueSend(fileSystemRequestQueue, &request, portMAX_DELAY);

    result_t *result;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    char *response_message = (char *)malloc(100 * sizeof(char));

    auto freeData = [&]()
    {
        free(result->devices);
        delete result;
        free(response_message);
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {

        if (result->result_code == 0)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Device %s removed successfully", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return true;
        }
        else if (result->result_code == 1)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Device %s does not exist", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return false;
        }
        else if (result->result_code == 2)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Failed to remove device %s", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return false;
        }
        else if (result->result_code == 3)
        {
            DEBUG_PRINTLN_WARNING("Failed to remove, cannot get the mutex");
            freeData();
            return false;
        }
        else if (result->result_code == 4)
        {
            DEBUG_PRINTLN_WARNING("Failed to remove, id update functions not set");
            freeData();
            return false;
        }
        else if (result->result_code == 5)
        {
            DEBUG_PRINTLN_WARNING("Please initialise the mqtt storage functions first");
            freeData();
            return false;
        }
    }
    freeData();
    return false;
}

bool update_device(mqtt_device_t mqttEquipment)
{
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {UPDATE_DEVICE, mqttEquipment, responseQueue, nullptr};
    xQueueSend(fileSystemRequestQueue, &request, portMAX_DELAY);

    result_t *result;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    char *response_message = (char *)malloc(100 * sizeof(char));

    auto freeData = [&]()
    {
        free(result->devices);
        delete result;
        free(response_message);
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        char *response_message = (char *)malloc(100 * sizeof(char));
        if (result->result_code == 0)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Device %s updated successfully", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return true;
        }
        else if (result->result_code == 1)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Failed to open device %s", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return false;
        }
        else if (result->result_code == 2)
        {
            DEBUG_SNPRINTF_WARNING(response_message, 100, "Failed to create device %s", request.mqttDevice.name.c_str());
            DEBUG_PRINTLN_WARNING(response_message);
            freeData();
            return false;
        }
        else if (result->result_code == 3)
        {
            DEBUG_PRINTLN_WARNING("Failed to open file for updating");
            freeData();
            return false;
        }
        else if (result->result_code == 4)
        {
            DEBUG_PRINTLN_WARNING("Device ID not found");
            freeData();
            return false;
        }
        else if (result->result_code == 5)
        {
            DEBUG_PRINTLN_WARNING("Please initialise the mqtt storage function first");
            freeData();
            return false;
        }
    }
    freeData();
    return false;
    vQueueDelete(responseQueue);
}

std::vector<mqtt_device_t> get_all_devices()
{
    std::vector<mqtt_device_t> mqttEquipments;
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {GET_ALL_DEVICES, mqtt_device_t(), responseQueue, nullptr};

    xQueueSend(fileSystemRequestQueue, &request, portMAX_DELAY);

    result_t *result;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    char *response_message = (char *)malloc(100 * sizeof(char));

    auto freeData = [&]()
    {
        free(result->devices);
        delete result;
        free(response_message);
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        if (result->result_code == 0)
        {
            mqttEquipments = *result->devices;
            freeData();
            return mqttEquipments;
        }
        else if (result->result_code == 1)
        {
            DEBUG_PRINTLN_WARNING("Got Empty list");
            freeData();
            return {};
        }
        else if (result->result_code == 2)
        {
            DEBUG_PRINTLN_WARNING("Failed to get all devices");
            freeData();
            return {};
        }

        else if (result->result_code == 5)
        {
            DEBUG_PRINTLN_WARNING("Please initialise the mqtt storage function first");
            freeData();
            return {};
        }
    }

    DEBUG_PRINTLN_WARNING("Failed to receive GET_ALL_DEVICES response");
    freeData();
    return {};
}

std::vector<std::vector<mqtt_device_t>> get_added_deivces()
{
    std::vector<std::vector<mqtt_device_t>> mqttEquipments;
    QueueHandle_t responseQueue = xQueueCreate(1, sizeof(result_t *));
    operation_request_t request = {GET_ADDED_DEVICES, mqtt_device_t(), responseQueue, nullptr};

    xQueueSend(fileSystemRequestQueue, &request, portMAX_DELAY);

    result_t *result = nullptr;
    xQueueReceive(responseQueue, &result, portMAX_DELAY);
    char *response_message = (char *)malloc(100 * sizeof(char));

    auto freeData = [&]()
    {
        free(result->devices_mutiple_lines);
        delete result;
        free(response_message);
        vQueueDelete(responseQueue);
    };

    if (result != nullptr)
    {
        if (result->result_code == 0)
        {
            mqttEquipments = *result->devices_mutiple_lines;
            freeData();
            return mqttEquipments;
        }
        else if (result->result_code == 1)
        {
            DEBUG_PRINTLN_WARNING("Got Empty list");
            freeData();
            return {};
        }
        else if (result->result_code == 2)
        {
            DEBUG_PRINTLN_WARNING("Failed to get addded devices");
            freeData();
            return {};
        }
        else if (result->result_code == 5)
        {
            DEBUG_PRINTLN_WARNING("Please initialise the mqtt storage function first");
            freeData();
            return {};
        }
    }

    DEBUG_PRINTLN_WARNING("Failed to receive GET_ADDED_DEVICES response");
    freeData();
    return {};
}

void test()
{
    // Serial.println("test start");
    // clear_all_device_files();
    // update_all_device_ids();
    // create_new_devices();
    // Serial.println("test done");
}

void create_new_devices()
{
    mqtt_device_t nightLightDevice = {
        .type = MqttDeviceType::LIGHT_BRIGHTNESS_CONTROL,
        .messageType = MqttDeviceMessageType::TRADITIONAL,
        .id = 44,
        .name = "NightLight",
        .stateTopic = "bedroom/nightLight/status",
        .commandTopic = "bedroom/nightLight/switch",
        .stateTopicBrightness = "bedroom/nightLight/brightness/status",
        .commandTopicBrightness = "bedroom/nightLight/brightness/set",
        .isAdded = true};
    nightLightDevice.availabilityTopic = "";
    nightLightDevice.configTopic = "";

    store_device(nightLightDevice);

    mqtt_device_t newEquipment2;
    newEquipment2.type = MqttDeviceType::SENSOR_BINARY;
    newEquipment2.messageType = MqttDeviceMessageType::JSON;
    newEquipment2.id = 42;
    newEquipment2.name = "bedroom_human_sensor";
    newEquipment2.stateTopic = "zigbee2mqtt/bedroom_human_sensor";
    newEquipment2.friendlyNameState = "occupancy";
    newEquipment2.isAdded = true;
    store_device(newEquipment2);

    mqtt_device_t newEquipment7;
    newEquipment7.type = MqttDeviceType::DEVICE_CONTROLLABLE_BINARY;
    newEquipment7.messageType = MqttDeviceMessageType::JSON;
    newEquipment7.id = 56;
    newEquipment7.name = "bedroom_light";
    newEquipment7.stateTopic = "zigbee2mqtt/light_bedroom";
    newEquipment7.commandTopic = "zigbee2mqtt/light_bedroom/set";
    newEquipment7.isAdded = true;
    store_device(newEquipment7);

    mqtt_device_t newEquipment3;
    newEquipment3.type = MqttDeviceType::SENSOR_BINARY;
    newEquipment3.messageType = MqttDeviceMessageType::JSON;
    newEquipment3.id = 253;
    newEquipment3.name = "TestButton";
    newEquipment3.stateTopic = "zigbee2mqtt/mqtt_button_1";
    newEquipment3.friendlyNameState = "action";
    newEquipment3.isAdded = true;
    store_device(newEquipment3);

    mqtt_device_t newEquipment4;
    newEquipment4.type = MqttDeviceType::LIGHT_RGB_CONTROL;
    newEquipment4.messageType = MqttDeviceMessageType::JSON;
    newEquipment4.id = 252;
    newEquipment4.name = "TestDevice4";
    newEquipment4.stateTopic = "home/livingroom/light/state3";
    newEquipment4.commandTopic = "home/livingroom/light/command3";
    newEquipment4.stateTopicBrightness = "home/livingroom/light/brightness/state3";
    newEquipment4.commandTopicBrightness = "home/livingroom/light/brightness/command3";
    newEquipment4.stateTopicRgb = "home/livingroom/light/rgb/state3";
    newEquipment4.commandTopicRgb = "home/livingroom/light/rgb/command3";
    newEquipment4.availabilityTopic = "home/livingroom/light/availability3";
    newEquipment4.configTopic = "home/livingroom/light/config3";
    newEquipment4.stateTopicBattery = "home/livingroom/light/battery/state3";
    store_device(newEquipment4);

    mqtt_device_t newEquipment5;
    newEquipment5.type = MqttDeviceType::LIGHT_RGB_CONTROL;
    newEquipment5.messageType = MqttDeviceMessageType::TRADITIONAL;
    newEquipment5.id = 251;
    newEquipment5.name = "TestDevice5";
    newEquipment5.stateTopic = "home/livingroom/light/state4";
    newEquipment5.commandTopic = "home/livingroom/light/command4";
    newEquipment5.stateTopicBrightness = "home/livingroom/light/brightness/state4";
    newEquipment5.commandTopicBrightness = "home/livingroom/light/brightness/command4";
    newEquipment5.stateTopicRgb = "home/livingroom/light/rgb/state4";
    newEquipment5.commandTopicRgb = "home/livingroom/light/rgb/command4";
    newEquipment5.availabilityTopic = "home/livingroom/light/availability4";
    newEquipment5.configTopic = "home/livingroom/light/config4";
    newEquipment5.stateTopicBattery = "home/livingroom/light/battery/state4";
    store_device(newEquipment5);

    mqtt_device_t newEquipment6;
    newEquipment6.type = MqttDeviceType::LIGHT_RGB_CONTROL; 
    newEquipment6.messageType = MqttDeviceMessageType::TRADITIONAL;
    newEquipment6.id = 250;
    newEquipment6.name = "TestDevice6";
    newEquipment6.stateTopic = "home/livingroom/light/state5";
    newEquipment6.commandTopic = "home/livingroom/light/command5";
    newEquipment6.stateTopicBrightness = "home/livingroom/light/brightness/state5";
    newEquipment6.commandTopicBrightness = "home/livingroom/light/brightness/command5";
    newEquipment6.stateTopicRgb = "home/livingroom/light/rgb/state5";
    newEquipment6.commandTopicRgb = "home/livingroom/light/rgb/command5";
    newEquipment6.availabilityTopic = "home/livingroom/light/availability5";
    newEquipment6.configTopic = "home/livingroom/light/config5";
    newEquipment6.stateTopicBattery = "home/livingroom/light/battery/state5";
    store_device(newEquipment6);
}
