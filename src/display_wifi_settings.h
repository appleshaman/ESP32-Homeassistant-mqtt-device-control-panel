#ifndef DISPLAY_WIFI_SETTINGS_H
#define DISPLAY_WIFI_SETTINGS_H

#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_port_v8.h"
#include <Arduino.h>
#include "wifi_utils.h"
#include <WiFi.h>

void init_wifi_settings_display();
void create_status_bar_wifi_icons(lv_obj_t *parent);
static void wifi_list_event_cb(lv_event_t *e);
static void button_close_enter_event_handler(lv_event_t *e);
static void keyboard_enter_event_handler(lv_event_t *e);
static void scan_and_update_wifi_list(void *pvParameters);
static void update_wifi_list(lv_obj_t *list_wifi_name, const std::vector<WifiNetwork>& wifiList);
static void refresh_wifi_list(lv_event_t *e);
static void create_modal_bg_wifi_name_click();
static void wifi_click_name_event_cb(lv_event_t *e);
static void wifi_list_modal_bg_event_cb(lv_event_t *e);
static void wifi_name_modal_bg_event_cb(lv_event_t *e);
static void wifi_delete_event_cb(lv_event_t * e);
static String extract_text_in_quotes(const String& input);
static void check_status(lv_timer_t *timer);

#endif /* DISPLAY_WIFI_SETTINGS_H */