#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <vector>
#include <string>
#include <Preferences.h>
#include <WiFi.h>

typedef struct
{
    std::string ssid;
    std::string password;
    int rssi;
} WifiNetwork;

typedef struct 
{
    int requestType;
    QueueHandle_t resultQueue;
    void *param;
} WifiRequest;

enum WifiRequestType {
    WIFI_REQUEST_CHECK_STATUS,
    WIFI_REQUEST_IS_CERTAIN_WIFI,
    WIFI_REQUEST_SCAN_NETWORKS,
    WIFI_REQUEST_CONNECT,
    WIFI_REQUEST_DISCONNECT
};

int init_wifi();
bool is_stored_password(const char *ssid);
String get_stored_password(const char *ssid);
void store_password(const char *ssid, const char *password);
void delete_password(const char *ssid);
void delete_all_passwords();
void disconnect_from_wifi();
int is_connected_to_wifi();
int is_connected_to_certain_wifi(const char *ssid);
bool connect_to_wifi(const char *ssid, const char *password);
bool connect_to_wifi(WifiNetwork *wifiNetwork);
bool connect_to_wifi(const char *ssid);
void connect_to_wifi_task(void *param);
std::vector<WifiNetwork> scan_wifi_networks();
void connect_to_last_wifi();

#endif // WIFI_UTILS_H
