#ifndef DISPLAY_MQTT_SETTINGS_H
#define DISPLAY_MQTT_SETTINGS_H

#include <lvgl.h>
#include "lvgl_port_v8.h"
#include <Arduino.h>
#include "struct_mqtt_device.h"

namespace
{
    typedef struct
    {
        const char *name;
        bool confirmed;
        int type;
        lv_obj_t *confirm_btn;
        lv_obj_t *cancel_btn;
        lv_obj_t *text_area_or_ddl;
        lv_obj_t *modal_bg;
        mqtt_device_t *device;
    } data_btn_t;
}

void create_status_bar_mqtt_icons(lv_obj_t *parent);
static void show_mqtt_container(lv_event_t *e);
static void check_status(lv_timer_t *timer);
static void mqtt_container_modal_bg_event_cb(lv_event_t *e);
static void create_mqtt_server_settings_display(lv_obj_t *parent);


static void tab_mqtt_overall_settings_event_cb(lv_event_t *e);

static void device_type_event_cb(lv_event_t *e);
static void device_type_visibility(mqtt_device_t *device, lv_obj_t *tabview_device_settings);
static void device_editing_page_modal_bg_event_cb(lv_event_t *e);
static void tab_mqtt_device_settings_event_cb(lv_event_t *e);

static void device_select_event_cb(lv_event_t *e);
static void create_mqtt_device_basic_settings(lv_obj_t *parent, mqtt_device_t *device);
static void create_mqtt_device_lights_settings(lv_obj_t *parent, mqtt_device_t *device);
static void create_mqtt_device_others_settings(lv_obj_t *parent, mqtt_device_t *device);
static void create_mqtt_device_json_settings(lv_obj_t *parent, mqtt_device_t *device);

static bool is_valid_ip(const char *str);
static bool is_valid_port(const char *str);
static bool is_empty_or_whitespace(const char *str);
static void handle_textarea_after_confirm(data_btn_t *data);

static void cancel_btn_event_cb(lv_event_t *e);
static void confirm_btn_event_cb(lv_event_t *e);
// For  keyboard
static void keyboard_event_cb(lv_event_t *e);
static void kb_and_container_event_cb(lv_event_t *e);

static bool is_empty_or_whitespace(const char *str);

#endif /* DISPLAY_MQTT_SETTINGS_H */