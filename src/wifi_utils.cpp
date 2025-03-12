#include "wifi_utils.h"

static Preferences preferences;
static SemaphoreHandle_t mutexPreferenceEdit = NULL;

static QueueHandle_t wifiRequestQueue;

static bool autoConnectToWifiWhenStart = true;

static int wifiStatus = -1; // has to be gloabal other wise will lose information when thread is blocked

static bool isInitial = false;

// Function to handle WiFi operations
void wifi_task(void *pvParameters)
{
    WifiRequest request;
    while (true)
    {
        if (xQueueReceive(wifiRequestQueue, &request, portMAX_DELAY) == pdTRUE)
        {
            switch (request.requestType)
            {
            case WIFI_REQUEST_CHECK_STATUS:
            {
                wifiStatus = (WiFi.status() == WL_CONNECTED) ? 1 : 0;
                xQueueSend(request.resultQueue, &wifiStatus, portMAX_DELAY);
                break;
            }
            case WIFI_REQUEST_IS_CERTAIN_WIFI:
            {
                WifiNetwork *network = (WifiNetwork *)request.param;
                String ssid = network->ssid.c_str();
                String currentSSID = WiFi.SSID();
                Serial.println("ssid");
                Serial.println(ssid);
                Serial.println("currentSSID");
                Serial.println(currentSSID);
                int is_certain_wifi = currentSSID.equals(ssid) ? 1 : 0;
                if (is_certain_wifi)
                {
                    Serial.println("Same!");
                }
                else
                {
                    Serial.println("Not the same!");
                }
                xQueueSend(request.resultQueue, &is_certain_wifi, portMAX_DELAY);
                break;
            }
            case WIFI_REQUEST_SCAN_NETWORKS:
            {
                Serial.println("Scanning networks...!!!");
                std::vector<WifiNetwork> *wifiList = new std::vector<WifiNetwork>(); // Allocate on heap
                int n = WiFi.scanNetworks();
                for (int i = 0; i < n; ++i)
                {
                    WifiNetwork network;
                    network.ssid = WiFi.SSID(i).c_str();
                    network.rssi = WiFi.RSSI(i);
                    wifiList->push_back(network);
                }
                Serial.println("Scanning networks...finished");
                xQueueSend(request.resultQueue, &wifiList, portMAX_DELAY);
                break;
            }
            case WIFI_REQUEST_CONNECT:
            {
                WifiNetwork *network = (WifiNetwork *)request.param;
                Serial.printf("Connecting to %s...\n", network->ssid.c_str());
                WiFi.begin(network->ssid.c_str(), network->password.c_str());
                int count = 0;
                while (WiFi.status() != WL_CONNECTED && count < 10)
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    count++;
                }
                if (WiFi.status() == WL_CONNECTED)
                {
                    store_password("last_time", network->ssid.c_str()); // if success connect store the ssid
                }
                int connect_status = (WiFi.status() == WL_CONNECTED) ? 1 : -1;
                if (connect_status == 1)
                {
                    Serial.printf("Connected to %s\n", network->ssid.c_str());
                }
                if (WiFi.status() != WL_CONNECTED)
                {
                    Serial.println("Connection Failed! Releasing resources and resetting WiFi state.");
                    WiFi.disconnect(true);
                    vTaskDelay(500);
                    WiFi.mode(WIFI_STA);
                    vTaskDelay(500);
                    WiFi.scanDelete();
                    vTaskDelay(100);
                }
                xQueueSend(request.resultQueue, &connect_status, portMAX_DELAY);
                break;
            }
            case WIFI_REQUEST_DISCONNECT:
            {
                WiFi.disconnect();
                break;
            }
            }
        }
    }
}

int init_wifi()
{
    mutexPreferenceEdit = xSemaphoreCreateBinary();
    xSemaphoreGive(mutexPreferenceEdit);

    wifiRequestQueue = xQueueCreate(20, sizeof(WifiRequest));

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);

    xTaskCreatePinnedToCore(wifi_task, "WiFiTask", 4096, NULL, 1, NULL, 0);
    
    isInitial = true;

    if (autoConnectToWifiWhenStart)
    {
        connect_to_last_wifi();
    }
    
    return 1;
}

bool is_stored_password(const char *ssid)
{
    bool temp;
    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        preferences.begin("Wifi_store", false);
        temp = preferences.isKey(ssid);
        xSemaphoreGive(mutexPreferenceEdit);
        preferences.end();
        return temp;
    }
    return false;
}

String get_stored_password(const char *ssid)
{
    String temp;
    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        preferences.begin("Wifi_store", false);
        temp = preferences.getString(ssid, "").c_str();
        preferences.end();
        xSemaphoreGive(mutexPreferenceEdit);
        return temp;
    }
    return "";
}

void store_password(const char *ssid, const char *password)
{
    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        preferences.begin("Wifi_store", false);
        preferences.putString(ssid, password);
        preferences.end();
        xSemaphoreGive(mutexPreferenceEdit);
    }
}

void delete_password(const char *ssid)
{
    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        preferences.begin("Wifi_store", false);
        preferences.remove(ssid);
        preferences.end();
        xSemaphoreGive(mutexPreferenceEdit);
    }
}

void delete_all_passwords()
{
    if (xSemaphoreTake(mutexPreferenceEdit, portMAX_DELAY) == pdTRUE)
    {
        preferences.begin("Wifi_store", false);
        preferences.clear();
        preferences.end();
        xSemaphoreGive(mutexPreferenceEdit);
    }
}

void disconnect_from_wifi()
{
    if (isInitial)
    {
        WifiRequest request = {WIFI_REQUEST_DISCONNECT, NULL, nullptr};
        xQueueSend(wifiRequestQueue, &request, portMAX_DELAY);
    }
    else
    {
        Serial.println("Please initial WiFi first");
    }
}

int is_connected_to_wifi()
{
    if (isInitial)
    {
        const TickType_t timeout = pdMS_TO_TICKS(300);

        QueueHandle_t resultQueue = xQueueCreate(1, sizeof(int));
        WifiRequest request = {WIFI_REQUEST_CHECK_STATUS, resultQueue, nullptr};
        xQueueSend(wifiRequestQueue, &request, timeout);

        BaseType_t receiveResult = xQueueReceive(resultQueue, &wifiStatus, timeout);
        if (receiveResult != pdPASS)
        {
            Serial.println("WiFi Receing overtime"); // still need to confirm if the status has been changed, even the wifiStatus is global variable that not need to be edited
        }
        else if (resultQueue != NULL)
        {
            vQueueDelete(resultQueue);
        }
    }
    else
    {
        Serial.println("Please initial WiFi first");
        isInitial = 0;
    }

    return wifiStatus;
}

int is_connected_to_certain_wifi(const char *ssid)
{
    if (isInitial)
    {
        WifiNetwork *wifiNetwork = new WifiNetwork();
        wifiNetwork->ssid = ssid;

        QueueHandle_t resultQueue = xQueueCreate(1, sizeof(int));

        WifiRequest request = {WIFI_REQUEST_IS_CERTAIN_WIFI, resultQueue, (void *)wifiNetwork};
        xQueueSend(wifiRequestQueue, &request, portMAX_DELAY);

        int is_certain_wifi;
        xQueueReceive(resultQueue, &is_certain_wifi, portMAX_DELAY);
        if (resultQueue != NULL)
        {
            vQueueDelete(resultQueue);
        }

        delete wifiNetwork;
        return is_certain_wifi;
    }
    else
    {
        Serial.println("Please initial WiFi first");
        return 0;
    }
}

void connect_to_last_wifi()
{
    if (isInitial)
    {
        WifiNetwork *wifiNetwork = new WifiNetwork();
        if (get_stored_password("last_time") != "")
        {
            wifiNetwork->ssid = get_stored_password("last_time").c_str();
            wifiNetwork->password = get_stored_password(wifiNetwork->ssid.c_str()).c_str();

            connect_to_wifi(wifiNetwork);
        }
    }
    else
    {
        Serial.println("Please initial WiFi first");
    }
}

void connect_to_wifi_task(void *param)
{
    WifiNetwork *wifiNetwork = static_cast<WifiNetwork *>(param);
    Serial.println(wifiNetwork->password.c_str());
    Serial.println(wifiNetwork->ssid.c_str());

    QueueHandle_t resultQueue = xQueueCreate(1, sizeof(int));

    // Send request to WiFi task queue
    WifiRequest request = {WIFI_REQUEST_CONNECT, resultQueue, (void *)wifiNetwork};
    xQueueSend(wifiRequestQueue, &request, portMAX_DELAY);

    // Wait for connection result
    int connect_status;
    xQueueReceive(resultQueue, &connect_status, portMAX_DELAY);

    // Clean up the queue if created here
    if (resultQueue != NULL)
    {
        vQueueDelete(resultQueue);
    }

    // Clean up and delete task
    delete wifiNetwork;
    vTaskDelete(NULL);
}

bool connect_to_wifi(const char *ssid, const char *password)
{
    if (isInitial)
    {
        WifiNetwork *wifiNetwork = new WifiNetwork;
        wifiNetwork->ssid = ssid;
        wifiNetwork->password = password;

        // Create a task to handle WiFi connection asynchronously
        xTaskCreatePinnedToCore(connect_to_wifi_task, "ConnectToWifiTask", 1024, wifiNetwork, 1, NULL, 0);

        return true;
    }
    else
    {
        Serial.println("Please initial WiFi first");
        return false;
    }
}

bool connect_to_wifi(const char *ssid)
{
    if (isInitial)
    {
        WifiNetwork *wifiNetwork = new WifiNetwork;
        wifiNetwork->password = get_stored_password(ssid).c_str();
        wifiNetwork->ssid = ssid;
        if (wifiNetwork->password.length() == 0)
        {
            delete wifiNetwork;
            return false;
        }

        // Create a task to handle WiFi connection asynchronously
        xTaskCreatePinnedToCore(connect_to_wifi_task, "ConnectToWifiTask", 1024, wifiNetwork, 1, NULL, 0);

        return true;
    }
    else
    {
        Serial.println("Please initial WiFi first");
        return false;
    }
}

bool connect_to_wifi(WifiNetwork *wifiNetwork)
{
    if (isInitial)
    {
        // Create a task to handle WiFi connection asynchronously
        xTaskCreatePinnedToCore(connect_to_wifi_task, "ConnectToWifiTask", 1024, wifiNetwork, 1, NULL, 0);

        return true;
    }
    else
    {
        Serial.println("Please initial WiFi first");
        return false;
    }
}

std::vector<WifiNetwork> scan_wifi_networks()
{
    if (isInitial)
    {
        QueueHandle_t resultQueue = xQueueCreate(1, sizeof(std::vector<WifiNetwork> *));
        if (resultQueue == NULL)
        {
            Serial.println("Failed to create resultQueue");
            return std::vector<WifiNetwork>();
        }

        WifiRequest request = {WIFI_REQUEST_SCAN_NETWORKS, resultQueue, nullptr};
        if (xQueueSend(wifiRequestQueue, &request, portMAX_DELAY) != pdPASS)
        {
            Serial.println("Failed to send request to wifiRequestQueue");
            vQueueDelete(resultQueue);
            return std::vector<WifiNetwork>();
        }

        std::vector<WifiNetwork> *wifiListPtr;
        if (xQueueReceive(resultQueue, &wifiListPtr, portMAX_DELAY) != pdPASS)
        {
            Serial.println("Failed to receive scan result");
            vQueueDelete(resultQueue);
            return std::vector<WifiNetwork>();
        }

        // Dereference the pointer to get the actual vector
        std::vector<WifiNetwork> wifiList = *wifiListPtr;
        delete wifiListPtr; // Don't forget to free the allocated memory

        vQueueDelete(resultQueue);

        return wifiList;
    }
    else
    {
        Serial.println("Please initial WiFi first");
        std::vector<WifiNetwork> wifiList;
        return wifiList;
    }
}
