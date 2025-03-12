#ifndef DISPLAY_H
#define DISPLAY_H

#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_port_v8.h"
#include <Arduino.h>
#include "display_status_bar.h"
#include "display_device_control_page.h"

void init_display();

static void btn_event_cb(lv_event_t *e);
static void my_switch_event_cb(lv_event_t *e);
void create_roller(lv_obj_t *parent);
void create_switches(lv_obj_t *parent);
static void roller_event_cb(lv_event_t *e);
#endif // DISPLAY_H