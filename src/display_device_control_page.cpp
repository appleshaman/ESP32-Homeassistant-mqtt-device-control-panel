#include "display_device_control_page.h"
#include <set>
#include <Preferences.h>
#include "utils_mqtt_server.h"
#include "utils_mqtt_device_store.h"
#include "brightness_icon.c"
#include "light_bulb_font.c"
#include <ArduinoJson.h>

namespace
{
    typedef struct
    {
        // For roller control
        const char *name;
        lv_obj_t *container;
        lv_obj_t *label_next_to_btn;
        uint8_t button_index;
        uint8_t min_value;
        uint8_t max_value;
        uint8_t current_value;

    } data_btn_t;

}

static SemaphoreHandle_t semaphorePageButton = xSemaphoreCreateBinary();

static SemaphoreHandle_t semaphoreDevicesAcess = xSemaphoreCreateBinary();
// Needs to lock down the device panel when the devices that bind to the containers are editing

static mqtt_device_t *current_device;
static uint8_t current_page = 0;
static std::vector<std::vector<mqtt_device_t>> devices_add_to_pages;

static lv_obj_t *cont_control_panel;

void init_device_control_page_display(lv_obj_t *parent)
{
    cont_control_panel = parent;
    xSemaphoreGive(semaphorePageButton);
    xSemaphoreGive(semaphoreDevicesAcess);
    lv_obj_t *cont_devices = lv_obj_create(parent); // 1st child 0
    lv_obj_set_pos(cont_devices, 0, 0);
    lv_obj_set_size(cont_devices, 770, 450);
    lv_obj_set_style_pad_all(cont_devices, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_devices, 0, LV_PART_MAIN);
    create_devices_panel(cont_devices);

    create_roller(parent); // 2nd child 1

    lv_obj_t *cont_buttom_display = lv_obj_create(parent); // 3rd child 2
    lv_obj_set_size(cont_buttom_display, 655, 95);
    lv_obj_align(cont_buttom_display, LV_ALIGN_BOTTOM_LEFT, 5, 0);
    lv_obj_set_style_pad_all(cont_buttom_display, 0, 0);
    create_page_control_panel(cont_buttom_display);

    devices_add_to_pages = get_added_deivces();
    update_devices_panel(cont_devices);
    update_cont_control_panel(cont_buttom_display);
    registerMqttMessageCallback(onMqttMessageReceived);
}

static void create_page_control_panel(lv_obj_t *parent)
{
    // Create the buttom page control part
    lv_obj_t *btn_prev = lv_btn_create(parent);
    lv_obj_set_size(btn_prev, 100, 40);
    lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_LEFT, 50, -5);
    lv_obj_t *label_prev = lv_label_create(btn_prev);
    lv_obj_set_user_data(btn_prev, (void *)-1);
    lv_label_set_text(label_prev, "Last Page");
    lv_obj_center(label_prev);
    lv_obj_add_event_cb(btn_prev, page_change_cb, LV_EVENT_CLICKED, lv_obj_get_child(lv_obj_get_parent(parent), 0));

    lv_obj_t *btn_next = lv_btn_create(parent);
    lv_obj_set_size(btn_next, 100, 40);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -50, -5);
    lv_obj_t *label_next = lv_label_create(btn_next);
    lv_obj_set_user_data(btn_next, (void *)1);
    lv_label_set_text(label_next, "Next Page");
    lv_obj_center(label_next);
    lv_obj_add_event_cb(btn_next, page_change_cb, LV_EVENT_CLICKED, lv_obj_get_child(lv_obj_get_parent(parent), 0));

    lv_obj_t *label_page_display = lv_label_create(parent);
    lv_obj_align(label_page_display, LV_ALIGN_BOTTOM_MID, 0, 0);
    std::string page_info = std::to_string(0) + " / " + std::to_string(0);
    lv_label_set_text(label_page_display, page_info.c_str());
}

static void create_devices_panel(lv_obj_t *parent)
{
    uint8_t container_width = 160; // Small container width
    int container_height = 350;    // Small container height
    uint8_t margin = 5;            // Small container margin
    uint8_t icon_height = 130;     // Icon height
    uint8_t btn_width = 55;
    uint8_t btn_height = 30;

    for (uint8_t i = 0; i < 4; i++)
    {

        // Create the container for the individual device
        lv_obj_t *cont_individual_device = lv_obj_create(parent);
        lv_obj_set_size(cont_individual_device, container_width, container_height);
        lv_obj_set_style_pad_all(cont_individual_device, 0, 0);
        lv_obj_set_style_bg_color(cont_individual_device, lv_color_make(240, 240, 240), 0);
        lv_obj_align(cont_individual_device, LV_ALIGN_TOP_LEFT,
                     5 + (i % 4) * (container_width + margin),
                     (i / 4) * (container_height + margin));

        // Create the label for the device name
        lv_obj_t *label_name = lv_label_create(cont_individual_device);
        lv_obj_set_width(label_name, container_width - 5);
        lv_label_set_long_mode(label_name, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(label_name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(label_name, LV_ALIGN_TOP_MID, 0, 5);

        // Create the icon of the device
        lv_obj_t *btn_icon = lv_btn_create(cont_individual_device);
        lv_obj_set_size(btn_icon, icon_height, icon_height);
        lv_obj_align(btn_icon, LV_ALIGN_TOP_MID, 0, 50);
        lv_obj_set_style_pad_all(btn_icon, 0, 0);

        lv_obj_add_event_cb(btn_icon, main_control_button_cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t *label_icon = lv_label_create(btn_icon);
        lv_label_set_text(label_icon, "UNDEFINED_SYMBOL");
        lv_obj_center(label_icon);

        // Create the button to control the device
        for (uint8_t j = 0; j < 4; j++)
        {
            lv_obj_t *btn = lv_btn_create(cont_individual_device);
            lv_obj_set_size(btn, btn_width, btn_height);
            lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 15, 190 + j * (btn_height + 10));

            lv_obj_set_style_pad_all(btn, 0, 0);
            lv_obj_set_size(btn, 25, 25);
            lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);

            static lv_style_t style_normal_btn;
            lv_style_init(&style_normal_btn);
            lv_style_set_shadow_opa(&style_normal_btn, LV_OPA_TRANSP);
            lv_obj_add_style(btn, &style_normal_btn, 0);

            lv_obj_t *label_value = lv_label_create(cont_individual_device);
            lv_obj_align(label_value, LV_ALIGN_CENTER, 0, 0);

            static lv_style_t style_label;
            lv_style_init(&style_label);
            lv_style_set_text_font(&style_label, &lv_font_montserrat_24);
            lv_obj_add_style(label_value, &style_label, LV_PART_MAIN);
            lv_label_set_text(label_value, "0");

            lv_obj_align_to(label_value, btn, LV_ALIGN_OUT_RIGHT_MID, 35, 0);
            lv_obj_set_style_pad_all(label_value, 0, 0);

            auto *btn_data = new data_btn_t();

            if (j == 0)
            {
                btn_data->name = "brightness";
                static lv_style_t style_brightness_btn;
                lv_style_init(&style_brightness_btn);
                lv_style_set_bg_opa(&style_brightness_btn, LV_OPA_TRANSP);
                lv_style_set_shadow_opa(&style_brightness_btn, LV_OPA_TRANSP);
                lv_obj_set_size(btn, 27, 27);
                lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 14, 190 + j * (btn_height + 10));
                lv_obj_add_style(btn, &style_brightness_btn, 0);
                lv_obj_t *img = lv_img_create(btn);
                lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
                lv_img_set_src(img, &brightness_icon);
                lv_obj_center(img);
            }
            else if (j == 1)
            {
                btn_data->name = "color_red";
                lv_obj_set_style_bg_color(btn, lv_color_make(255, 0, 0), LV_PART_MAIN);
            }
            else if (j == 2)
            {
                btn_data->name = "color_green";
                lv_obj_set_style_bg_color(btn, lv_color_make(0, 255, 0), LV_PART_MAIN);
            }
            else if (j == 3)
            {
                btn_data->name = "color_blue";
                lv_obj_set_style_bg_color(btn, lv_color_make(0, 0, 255), LV_PART_MAIN);
            }

            btn_data->container = cont_individual_device;
            btn_data->label_next_to_btn = label_value;
            btn_data->button_index = j;
            btn_data->min_value = (j == 0) ? 0 : 0;    // First button range is 0-99
            btn_data->max_value = (j == 0) ? 99 : 255; // Other buttons range is 0-255
            btn_data->current_value = 0;               // Start from 0

            lv_obj_set_user_data(btn, btn_data);

            lv_obj_add_event_cb(btn, small_button_event_cb, LV_EVENT_CLICKED, label_value);
        }

        lv_obj_t *label_non_binary_value = lv_label_create(cont_individual_device);
        static lv_style_t style_label_non_binary_value;
        lv_style_init(&style_label_non_binary_value);
        lv_style_set_text_font(&style_label_non_binary_value, &lv_font_montserrat_40);
        lv_obj_add_style(label_non_binary_value, &style_label_non_binary_value, LV_PART_MAIN);
        lv_obj_align_to(label_non_binary_value, btn_icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);
    }
}

static void small_button_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    auto *btn_data = (data_btn_t *)lv_obj_get_user_data(btn);

    if (!btn_data)
    {
        printf("Error: btn_data is null!\n");
        return;
    }

    // Get the roller
    lv_obj_t *roller = lv_obj_get_child(lv_obj_get_parent(lv_obj_get_parent(btn_data->container)), 1);
    if (!roller)
    {
        printf("Error: Roller is null for button %d!\n", btn_data->button_index);
        return;
    }

    // Calculate the range of roller
    uint8_t range = btn_data->max_value - btn_data->min_value;
    int total_length = ((int)range + 1) * 6; // Each option is 5 bytes + 1 byte null terminator

    char *opts = (char *)malloc(total_length);
    if (!opts)
    {
        printf("Error: Failed to allocate memory for roller options!\n");
        return;
    }

    opts[0] = '\0';

    for (uint8_t i = 0; i <= range; i++)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d\n", i);
        strcat(opts, buf);
    }

    // Remove the \n
    if (strlen(opts) > 0 && opts[strlen(opts) - 1] == '\n')
    {
        opts[strlen(opts) - 1] = '\0';
    }

    // Set the roller options
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
    mqtt_device_t *device = static_cast<mqtt_device_t *>(lv_obj_get_user_data(btn_data->container));
    if (btn_data->name == "brightness")
    {
        lv_roller_set_selected(roller, std::stoi(device->brightnessMessage), LV_ANIM_OFF);
    }

    // Bind the roller to the button
    lv_obj_set_user_data(roller, btn_data);
    free(opts);

    printf("Roller updated for button index %d\n", btn_data->button_index);
}

static void update_devices_panel(lv_obj_t *parent)
{
    Serial.println("update_devices_panel");
    lvgl_port_lock(-1);

    auto &current_devices = devices_add_to_pages[current_page]; // Get current device
    uint8_t device_count = 0;

    for (device_count = 0; device_count < (uint8_t)current_devices.size(); device_count++)
    {
        lv_obj_t *cont_individual_device = lv_obj_get_child(parent, device_count);
        if (cont_individual_device)
        {
            update_device_container(cont_individual_device, current_devices[device_count]);
        }
    }

    for (uint8_t i = device_count; i < 4; i++)
    {
        lv_obj_t *cont_individual_device = lv_obj_get_child(parent, i);
        if (cont_individual_device)
        {
            lv_obj_add_flag(cont_individual_device, LV_OBJ_FLAG_HIDDEN);
        }
    }

    lvgl_port_unlock();
}

static void update_device_container(lv_obj_t *cont_individual_device, mqtt_device_t &device)
{

    lv_obj_set_user_data(cont_individual_device, &device);

    if (!device.isSubscribed)
    {
        if (subscribe_mqtt_device(device))
        {
            device.isSubscribed = true;
        }
    }

    lv_obj_t *label_name = lv_obj_get_child(cont_individual_device, 0); // Device name
    if (label_name)
    {
        lv_label_set_text(label_name, device.name.c_str());
    }

    lv_obj_clear_flag(cont_individual_device, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *btn_main = lv_obj_get_child(cont_individual_device, 1);
    lv_obj_t *label_btn = lv_obj_get_child(btn_main, 0);

    if (device.type == 2 || device.type == 3 || device.type == 4)
    {
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &light_bulb_font);

        if (device.basicMessage == "OFF")
        {
            lv_style_set_text_color(&style, lv_color_hex(0xFFFFFF));
        }
        else if (device.basicMessage == "ON")
        {
            lv_style_set_text_color(&style, lv_color_hex(0xFED841));
        }

        lv_obj_add_style(label_btn, &style, 0);
        lv_label_set_text(label_btn, "\xEE\xA4\x80"); // "\xEE\xA4\x80" is the UTF-8 encoding of the character U+E900
        lv_obj_align(label_btn, LV_ALIGN_CENTER, 0, 0);
    }

    if (device.type == 1 || device.type == 0)
    {
        lv_obj_clear_flag(btn_main, LV_OBJ_FLAG_CLICKABLE);
    }
    else
    {
        lv_obj_add_flag(btn_main, LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *btn_brightness = lv_obj_get_child(cont_individual_device, 2);
    lv_obj_t *label_brightness = lv_obj_get_child(cont_individual_device, 3);
    lv_obj_t *btn_red_color = lv_obj_get_child(cont_individual_device, 4);
    lv_obj_t *label_red_color = lv_obj_get_child(cont_individual_device, 5);
    lv_obj_t *btn_green_color = lv_obj_get_child(cont_individual_device, 6);
    lv_obj_t *label_green_color = lv_obj_get_child(cont_individual_device, 7);
    lv_obj_t *btn_blue_color = lv_obj_get_child(cont_individual_device, 8);
    lv_obj_t *label_blue_color = lv_obj_get_child(cont_individual_device, 9);
    lv_obj_t *label_non_binary_value = lv_obj_get_child(cont_individual_device, 10);

    // Lambda used to set availability
    auto set_visibility = [btn_brightness, label_brightness,
                           btn_red_color, label_red_color,
                           btn_green_color, label_green_color,
                           btn_blue_color, label_blue_color,
                           label_non_binary_value](bool brightness, bool red, bool green, bool blue, bool non_binary)
    {
        if (brightness)
        {
            lv_obj_clear_flag(btn_brightness, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(label_brightness, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(btn_brightness, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(label_brightness, LV_OBJ_FLAG_HIDDEN);
        }

        if (red)
        {
            lv_obj_clear_flag(btn_red_color, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(label_red_color, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(btn_red_color, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(label_red_color, LV_OBJ_FLAG_HIDDEN);
        }

        if (green)
        {
            lv_obj_clear_flag(btn_green_color, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(label_green_color, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(btn_green_color, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(label_green_color, LV_OBJ_FLAG_HIDDEN);
        }

        if (blue)
        {
            lv_obj_clear_flag(btn_blue_color, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(label_blue_color, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(btn_blue_color, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(label_blue_color, LV_OBJ_FLAG_HIDDEN);
        }

        if (non_binary)
        {
            lv_obj_clear_flag(label_non_binary_value, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(label_non_binary_value, LV_OBJ_FLAG_HIDDEN);
        }
    };

    // Set the visibility of the buttons and labels based on the device type
    if ((device.type == 0) || (device.type == 2))
    {
        set_visibility(false, false, false, false, false);
    }
    else if (device.type == 1)
    {
        set_visibility(true, false, false, false, true);
    }
    else if (device.type == 3)
    {
        set_visibility(true, false, false, false, false);
    }
    else if (device.type == 4)
    {
        set_visibility(true, true, true, true, false);
    }
}

static void update_device_container_childs(lv_obj_t *cont_individual_device) {}

static void update_cont_control_panel(lv_obj_t *parent)
{
    lv_obj_t *label_page_display = lv_obj_get_child(parent, 2);
    current_page = 0;
    std::string page_info = std::to_string(current_page + 1) + " / " + std::to_string(devices_add_to_pages.size());
    lv_label_set_text(label_page_display, page_info.c_str());
}

void main_control_button_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *cont_individual_device = lv_obj_get_parent(btn);

    mqtt_device_t *device = static_cast<mqtt_device_t *>(lv_obj_get_user_data(cont_individual_device));

    if (!device)
        return; // If has not set data, return

    if (!device->isSubscribed)
        return;

    if (device->basicMessage == "OFF")
    {
        publish_to_mqtt(device->commandTopic, "ON", *device);
    }
    else if (device->basicMessage == "ON")
    {
        publish_to_mqtt(device->commandTopic, "OFF", *device);
    }
}

void page_change_cb(lv_event_t *e)
{
    if (xSemaphoreTake(semaphorePageButton, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        lv_obj_t *btn = lv_event_get_target(e);
        uint8_t direction = (uint8_t)(intptr_t)lv_obj_get_user_data(btn);

        // Update current page
        current_page += direction;

        // Limit the current page range
        if (current_page < 0)
        {
            current_page = 0;
        }
        else if (current_page >= devices_add_to_pages.size())
        {
            current_page = devices_add_to_pages.size() - 1;
        }

        // Update device display
        update_devices_panel(((lv_obj_t *)lv_event_get_user_data(e)));

        // Update page display
        lv_obj_t *parent = lv_obj_get_parent(btn);
        lv_obj_t *label_page_display = lv_obj_get_child(parent, 2);
        std::string page_info = std::to_string(current_page + 1) + " / " + std::to_string(devices_add_to_pages.size());
        lv_label_set_text(label_page_display, page_info.c_str());

        lv_obj_t *roller = lv_obj_get_child(cont_control_panel, 1);
        lv_obj_set_user_data(roller, NULL);
        const char *opts = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";
        lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
        lv_roller_set_selected(roller, 3, LV_ANIM_OFF);

        xSemaphoreGive(semaphorePageButton);
    }
}

static void create_roller(lv_obj_t *parent)
{
    const char *opts = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";
    lv_obj_t *roller;
    /*A roller on the right with right aligned text, and custom width*/
    roller = lv_roller_create(parent);
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(roller, 4);
    lv_obj_set_height(roller, 400);
    lv_obj_set_width(roller, 95);

    static lv_style_t style_unsel;
    lv_style_init(&style_unsel);
    lv_style_set_text_line_space(&style_unsel, 60);
    lv_style_set_text_font(&style_unsel, &lv_font_montserrat_18);
    lv_obj_add_style(roller, &style_unsel, LV_PART_MAIN);
    static lv_style_t style_sel;
    lv_style_set_text_font(&style_sel, &lv_font_montserrat_20);
    lv_obj_add_style(roller, &style_sel, LV_PART_SELECTED);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_align(roller, LV_ALIGN_RIGHT_MID, -5, -25);
    lv_obj_add_event_cb(roller, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_roller_set_selected(roller, 3, LV_ANIM_OFF);
}

static void roller_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *roller = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        // Get the button data bound to the roller
        auto *btn_data = (data_btn_t *)lv_obj_get_user_data(roller);
        if (!btn_data)
        {
            printf("Error: btn_data is null in roller_event_cb!\n");
            return;
        }

        // Get the selected value from the roller
        char buf[32];
        lv_roller_get_selected_str(roller, buf, sizeof(buf));
        int new_value = atoi(buf); // Use the selected value directly

        // Update the label next to the button
        char label_text[32];
        snprintf(label_text, sizeof(label_text), "%d", new_value);
        lv_label_set_text(btn_data->label_next_to_btn, label_text);

        // Update the current value of the btn_data
        mqtt_device_t *device = (mqtt_device_t *)lv_obj_get_user_data(btn_data->container);
        if (device)
        {
            std::string topic = device->commandTopicBrightness;
            if (btn_data->name == "brightness")
            {
                topic = device->commandTopicBrightness;

                Serial.println(std::to_string(round((new_value * 255.0) / 99.0)).c_str());
                publish_to_mqtt(topic, std::to_string(round((new_value * 255.0) / 99.0)).c_str(), *device);
            }
            else if (device->name == "red")
            {
                topic = device->commandTopicRgb;
            }
            else if (device->name == "green")
            {
                topic = device->commandTopicRgb;
            }
            else if (device->name == "blue")
            {
                topic = device->commandTopicRgb;
            }
        }
        btn_data->current_value = new_value;
    }
}

void onMqttMessageReceived(const std::string &topic, const std::string &message)
{
    lv_obj_t *cont_devices = lv_obj_get_child(cont_control_panel, 0);
    Serial.printf("Received MQTT message: topic=%s, message=%s\n", topic.c_str(), message.c_str());

    lvgl_port_lock(-1);

    lv_obj_t *roller = lv_obj_get_child(cont_control_panel, 1);
    auto *btn_data = (data_btn_t *)lv_obj_get_user_data(roller);

    for (size_t page_index = 0; page_index < devices_add_to_pages.size(); ++page_index)
    {
        auto &devices_on_this_page = devices_add_to_pages[page_index];

        for (size_t device_index = 0; device_index < devices_on_this_page.size(); ++device_index)
        {
            mqtt_device_t &device = devices_on_this_page[device_index];

            // Skip unsubscribed devices
            if (!device.isSubscribed)
                continue;

            bool updated = false; // For as long as one device is updated, print a log

            if (device.messageType == JSON && device.stateTopic == topic) // JSON format device
            {
                // Ensure that you are receiving JSON data
                if (message.find('{') == std::string::npos)
                {
                    Serial.println("Error: Expected JSON but received non-JSON data!");
                    continue;
                }

                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, message);

                if (error || !doc.is<JsonObject>())
                {
                    Serial.println("Error: Invalid JSON format, skipping.");
                    continue;
                }

                // Pasering JSON device status
                if (doc.containsKey(device.friendlyNameState))
                {
                    std::string newBasicMessage = doc[device.friendlyNameState].as<std::string>();
                    if (device.basicMessage != newBasicMessage)
                    {
                        device.basicMessage = newBasicMessage;
                        updated = true;
                        // If the device is in the current page and visible, update the UI
                        if (page_index == current_page && device_index < 4)
                        {
                            lv_obj_t *cont_individual_device = lv_obj_get_child(cont_devices, device_index);
                            if (cont_individual_device)
                            {
                                lv_obj_t *btn = lv_obj_get_child(cont_individual_device, 1);
                                lv_obj_t *label_btn = lv_obj_get_child(btn, 0);

                                if (device.type == 0 || device.type == 2 || device.type == 3 || device.type == 4)
                                {
                                    if (device.basicMessage == "OFF")
                                    {
                                        lv_obj_set_style_text_color(label_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                                    }
                                    else if (device.basicMessage == "ON")
                                    {
                                        lv_obj_set_style_text_color(label_btn, lv_color_hex(0xFED841), LV_PART_MAIN);
                                    }
                                    else
                                    {
                                        lv_label_set_text(label_btn, device.basicMessage.c_str());
                                    }
                                }
                                else if (device.type == 1) // Sensor non binary
                                {
                                    lv_obj_t *label_non_binary_value = lv_obj_get_child(cont_individual_device, 10);
                                    lv_label_set_text(label_non_binary_value, device.basicMessage.c_str());
                                }
                            }
                        }
                    }
                }

                if (doc.containsKey(device.friendlyNameBrightness))
                {
                    int brightnessValue = doc[device.friendlyNameBrightness].as<int>();
                    int displayBrightness = round((brightnessValue * 99.0) / 255.0);
                    std::string newBrightnessMessage = std::to_string(displayBrightness);
                    if (device.brightnessMessage != newBrightnessMessage)
                    {
                        device.brightnessMessage = newBrightnessMessage;
                        updated = true;
                    }
                }

                if (device.friendlyNameColorMode == RGB && doc.containsKey("color"))
                {
                    JsonObject colorObj = doc["color"];
                    int new_r = colorObj["r"] | 0;
                    int new_g = colorObj["g"] | 0;
                    int new_b = colorObj["b"] | 0;

                    if (new_r != std::stoi(device.redMessage) || new_g != std::stoi(device.greenMessage) || new_b != std::stoi(device.blueMessage))
                    {
                        device.redMessage = std::to_string(new_r);
                        device.greenMessage = std::to_string(new_g);
                        device.blueMessage = std::to_string(new_b);
                        updated = true;
                    }
                }

                if (doc.containsKey(device.friendlyNameBattery))
                {
                    std::string newBatteryMessage = doc[device.friendlyNameBattery].as<std::string>();
                    if (device.batteryMessage != newBatteryMessage)
                    {
                        device.batteryMessage = newBatteryMessage;
                        updated = true;
                    }
                }

                if (doc.containsKey(device.friendlyLinkQuality))
                {
                    std::string newLinkQualityMessage = doc[device.friendlyLinkQuality].as<std::string>();
                    if (device.linkQualityMessage != newLinkQualityMessage)
                    {
                        device.linkQualityMessage = newLinkQualityMessage;
                        updated = true;
                    }
                }
            }
            // Traditional format devices
            else if (device.messageType != JSON)
            {
                // Handle different topics for different devices
                // Traditional message can only handle one topic during one time, so we use if else to handle them
                if (device.stateTopic == topic && device.basicMessage != message)
                {
                    device.basicMessage = message;
                    updated = true;
                    // If the device is in the current page and visible, update the UI
                    if (page_index == current_page && device_index < 4)
                    {
                        lv_obj_t *cont_individual_device = lv_obj_get_child(cont_devices, device_index);
                        if (cont_individual_device)
                        {
                            lv_obj_t *btn = lv_obj_get_child(cont_individual_device, 1);
                            lv_obj_t *label_btn = lv_obj_get_child(btn, 0);
                            if (device.type == 0 || device.type == 2 || device.type == 3 || device.type == 4)
                            {
                                if (device.basicMessage == "OFF")
                                {
                                    lv_obj_set_style_text_color(label_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                                }
                                else if (device.basicMessage == "ON")
                                {
                                    lv_obj_set_style_text_color(label_btn, lv_color_hex(0xFED841), LV_PART_MAIN);
                                }
                            }
                            else if (device.type == 1)
                            {
                                lv_obj_t *label_non_binary_value = lv_obj_get_child(cont_individual_device, 10);
                                lv_label_set_text(label_non_binary_value, device.basicMessage.c_str());
                            }
                        }
                    }
                }
                else if (device.stateTopicBrightness == topic && device.brightnessMessage != message)
                {
                    int brightnessValue = std::stoi(message);
                    int displayBrightness = round((brightnessValue * 99.0) / 255.0);
                    std::string newBrightnessMessage = std::to_string(displayBrightness);
                    device.brightnessMessage = newBrightnessMessage;
                    updated = true;

                    // If the device is in the current page and visible, update the UI
                    if (page_index == current_page && device_index < 4)
                    {
                        lv_obj_t *cont_individual_device = lv_obj_get_child(cont_devices, device_index);
                        if (cont_individual_device)
                        {
                            lv_obj_t *label_brightness = lv_obj_get_child(cont_individual_device, 3);
                            lv_label_set_text(label_brightness, device.brightnessMessage.c_str());
                            if (btn_data && btn_data->name == "brightness")
                            {
                                lv_roller_set_selected(roller, displayBrightness, LV_ANIM_OFF);
                            }
                        }
                    }
                }
                else if (device.stateTopicBattery == topic && device.batteryMessage != message)
                {
                    device.batteryMessage = message;
                    updated = true;
                    // If there is a battery UI, add update code in the future, not deved
                }
                else if (device.stateTopicRgb == topic)
                {
                    // Parse RGB values
                    int r = device.redMessage.empty() ? 0 : std::stoi(device.redMessage);
                    int g = device.greenMessage.empty() ? 0 : std::stoi(device.greenMessage);
                    int b = device.blueMessage.empty() ? 0 : std::stoi(device.blueMessage);
                    int new_r = r, new_g = g, new_b = b;

                    JsonDocument doc;
                    doc.to<JsonVariant>().set(JsonVariantConst()); // Ensure JSON is initialised to empty

                    if (device.messageType == JSON)
                    {
                        DeserializationError error = deserializeJson(doc, message);
                        if (!error)
                        {
                            new_r = doc["r"] | 0;
                            new_g = doc["g"] | 0;
                            new_b = doc["b"] | 0;
                        }
                    }
                    else if (message.find(',') != std::string::npos)
                    {
                        //  Use sscanf to parse the values. E.g. "255,128,64"
                        sscanf(message.c_str(), "%d,%d,%d", &new_r, &new_g, &new_b);
                    }

                    if (new_r != r || new_g != g || new_b != b)
                    {
                        device.redMessage = std::to_string(new_r);
                        device.greenMessage = std::to_string(new_g);
                        device.blueMessage = std::to_string(new_b);
                        updated = true;

                        // If the device is in the current page and visible, update the UI
                        if (page_index == current_page && device_index < 4)
                        {
                            lv_obj_t *cont_individual_device = lv_obj_get_child(cont_devices, device_index);
                            if (cont_individual_device)
                            {
                                lv_obj_t *btn_red = lv_obj_get_child(cont_individual_device, 4);
                                lv_obj_t *btn_green = lv_obj_get_child(cont_individual_device, 6);
                                lv_obj_t *btn_blue = lv_obj_get_child(cont_individual_device, 8);

                                lv_obj_set_style_bg_color(btn_red, lv_color_make(new_r, 0, 0), LV_PART_MAIN);
                                lv_obj_set_style_bg_color(btn_green, lv_color_make(0, new_g, 0), LV_PART_MAIN);
                                lv_obj_set_style_bg_color(btn_blue, lv_color_make(0, 0, new_b), LV_PART_MAIN);

                                if (btn_data && btn_data->name == "color_red")
                                {
                                    lv_roller_set_selected(roller, new_r, LV_ANIM_OFF);
                                }
                                if (btn_data && btn_data->name == "color_green")
                                {
                                    lv_roller_set_selected(roller, new_g, LV_ANIM_OFF);
                                }
                                if (btn_data && btn_data->name == "color_blue")
                                {
                                    lv_roller_set_selected(roller, new_b, LV_ANIM_OFF);
                                }
                            }
                        }
                    }
                }
            }
            // As long as one device is updated, print a log
            if (updated)
            {
                Serial.printf("Device [%s] updated\n", device.name.c_str());
                break;
            }
        }
    }

    lvgl_port_unlock();
}

void refreshMainpage()
{
    lv_obj_t *cont_devices = lv_obj_get_child(cont_control_panel, 0);
    lv_obj_t *roller = lv_obj_get_child(cont_control_panel, 1);
    lv_obj_t *cont_buttom_display = lv_obj_get_child(cont_control_panel, 2);

    devices_add_to_pages = get_added_deivces();
    lv_obj_set_user_data(roller, NULL);
    const char *opts = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
    lv_roller_set_selected(roller, 3, LV_ANIM_OFF);

    update_devices_panel(cont_devices);
    update_cont_control_panel(cont_buttom_display);
}