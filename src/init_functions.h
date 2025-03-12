#include "utils_mqtt_server.h"
#include "utils_mqtt_device_store.h"
#include "wifi_utils.h"

void init_functions(){
    init_wifi();
    init_mqtt_server();
    init_mqtt_device_store();
    //test();
    //init_device_control_page();
}