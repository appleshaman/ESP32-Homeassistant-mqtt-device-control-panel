#include "display_status_bar.h"

void create_status_bar(lv_obj_t *parent)
{
    
    // Create a status bar container
    lv_obj_t *cont_status = lv_obj_create(parent);
    lv_obj_set_size(cont_status, 800, 30);
    lv_obj_set_pos(cont_status, 0, 0);
    lv_obj_clear_flag(cont_status, LV_OBJ_FLAG_SCROLLABLE);
    
    // lv_obj_add_event_cb(status_cont, wifi_icon_event_cb, LV_EVENT_CLICKED, NULL);

    // Remove any default padding or border for status container
    lv_obj_set_style_pad_all(cont_status, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_status, 0, LV_PART_MAIN);
    // Set status bar style: straight corners
    lv_obj_set_style_radius(cont_status, 0, LV_PART_MAIN);

    init_wifi_settings_display();
    create_status_bar_wifi_icons(cont_status);
    create_status_bar_mqtt_icons(cont_status);
}