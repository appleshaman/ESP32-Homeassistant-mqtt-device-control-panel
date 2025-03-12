#ifndef DISPLAY_DEVICE_CONTROL_PAGE_H
#define DISPLAY_DEVICE_CONTROL_PAGE_H

#include <lvgl.h>
#include "lvgl_port_v8.h"
#include <Arduino.h>
#include "struct_mqtt_device.h"



void init_device_control_page_display(lv_obj_t *parent);
static void create_page_control_panel(lv_obj_t *parent);
static void page_change_cb(lv_event_t *e);
static void roller_event_cb(lv_event_t *e);

static void create_roller(lv_obj_t *parent);
static void create_devices_panel(lv_obj_t *parent);
static void update_devices_panel(lv_obj_t *parent);
static void update_device_container(lv_obj_t *cont_individual_device, mqtt_device_t &device);
static void update_cont_control_panel(lv_obj_t *parent);
void onMqttMessageReceived(const std::string &topic, const std::string &message);

static void small_button_event_cb(lv_event_t *e);
void main_control_button_cb(lv_event_t *e);
void test_in_ids();

void refreshMainpage();

#endif /* DISPLAY_DEVICE_CONTROL_PAGE_H */