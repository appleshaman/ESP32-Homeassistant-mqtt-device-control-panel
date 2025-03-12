#ifndef DISPLAY_STATUS_BAR
#define DISPLAY_STATUS_BAR

#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_port_v8.h"
#include "display_wifi_settings.h"
#include "display_mqtt_settings.h"

void create_status_bar(lv_obj_t *parent);
#endif