#include "display_wifi_settings.h"

static std::map<std::string, lv_obj_t *> childMap;
static SemaphoreHandle_t mutexWifiScan = NULL;

static lv_obj_t *label_wifi_status;
static lv_style_t style_label_wifi_status;

void init_wifi_settings_display()
{
    mutexWifiScan = xSemaphoreCreateBinary();
    xSemaphoreGive(mutexWifiScan);
}

void create_status_bar_wifi_icons(lv_obj_t *parent)
{
    static lv_style_t style_fonts_icons;
    lv_style_init(&style_fonts_icons);
    lv_style_set_text_font(&style_fonts_icons, &lv_font_montserrat_22);

    lv_obj_t *cont_wifi = lv_obj_create(parent);
    lv_obj_set_size(cont_wifi, 75, 40);
    lv_obj_set_pos(cont_wifi, 450, 0);
    lv_obj_add_flag(cont_wifi, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(cont_wifi, LV_OBJ_FLAG_SCROLLABLE);
    // lv_obj_add_event_cb(status_cont, wifi_icon_event_cb, LV_EVENT_CLICKED, NULL);
    // Remove any default padding or border for status container
    lv_obj_set_style_pad_all(cont_wifi, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_wifi, 0, LV_PART_MAIN);
    // Set status bar style: straight corners
    lv_obj_set_style_radius(cont_wifi, 0, LV_PART_MAIN);

    lv_obj_t *label_wifi_icon = lv_label_create(cont_wifi);
    lv_obj_set_pos(label_wifi_icon, 0, 1);
    lv_obj_add_style(label_wifi_icon, &style_fonts_icons, LV_PART_MAIN);
    lv_label_set_text(label_wifi_icon, LV_SYMBOL_WIFI);

    lv_style_init(&style_label_wifi_status);
    lv_style_set_text_font(&style_label_wifi_status, &lv_font_montserrat_32);

    label_wifi_status = lv_label_create(cont_wifi);
    lv_obj_set_pos(label_wifi_status, 35, -3);
    lv_obj_add_style(label_wifi_status, &style_label_wifi_status, LV_PART_MAIN);
    lv_label_set_text(label_wifi_status, "");
    // lv_obj_t *label_mqtt = lv_label_create(parent);

    lv_timer_create(check_status, 1000, NULL);
    lv_obj_add_event_cb(cont_wifi, wifi_list_event_cb, LV_EVENT_CLICKED, NULL);
}

static void wifi_list_event_cb(lv_event_t *e) // First window after clicking wifi icon
{
    // Create a modal background
    lv_obj_t *bg_modal = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(bg_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bg_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg_modal, LV_OPA_50, 0);
    lv_obj_set_size(bg_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(bg_modal);
    lv_obj_add_event_cb(bg_modal, wifi_list_modal_bg_event_cb, LV_EVENT_CLICKED, NULL);

    // Create a container for the popup
    lv_obj_t *cont_popup = lv_obj_create(bg_modal);
    lv_obj_set_size(cont_popup, 700, 400);
    lv_obj_center(cont_popup);
    lv_obj_clear_flag(cont_popup, LV_OBJ_FLAG_SCROLLABLE);

    static lv_style_t style_fonts_buttons;
    lv_style_init(&style_fonts_buttons);
    lv_style_set_text_font(&style_fonts_buttons, &lv_font_montserrat_18);

    lv_obj_t *btn_close = lv_btn_create(cont_popup);
    lv_obj_set_size(btn_close, 100, 40);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_t *label_close_btn = lv_label_create(btn_close);
    lv_label_set_text(label_close_btn, "Close");
    lv_obj_add_style(label_close_btn, &style_fonts_buttons, LV_PART_MAIN);
    lv_obj_align(label_close_btn, LV_ALIGN_CENTER, 0, 0);
    // Add event handler to the button_close
    lv_obj_add_event_cb(btn_close, button_close_enter_event_handler, LV_EVENT_CLICKED, bg_modal);

    lv_obj_t *btn_refresh = lv_btn_create(cont_popup);
    lv_obj_set_size(btn_refresh, 100, 40);
    lv_obj_align(btn_refresh, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_t *label_refresh_btn = lv_label_create(btn_refresh);
    lv_label_set_text(label_refresh_btn, "Refresh");
    lv_obj_add_style(label_refresh_btn, &style_fonts_buttons, LV_PART_MAIN);
    lv_obj_align(label_refresh_btn, LV_ALIGN_CENTER, 0, 0);
    // Add event handler to the button_close
    lv_obj_add_event_cb(btn_refresh, refresh_wifi_list, LV_EVENT_CLICKED, bg_modal);

    // Create a list for available WiFi networks
    static lv_style_t style_fonts_list_wifi_name;
    lv_style_init(&style_fonts_list_wifi_name);
    lv_style_set_text_font(&style_fonts_list_wifi_name, &lv_font_montserrat_22);
    lv_obj_t *list_wifi_name = lv_list_create(cont_popup);
    lv_obj_set_size(list_wifi_name, 650, 270);
    lv_obj_align(list_wifi_name, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_style(list_wifi_name, &style_fonts_list_wifi_name, LV_PART_MAIN);
    childMap["list_wifi_name"] = list_wifi_name;

    // Show loading label

    lv_obj_t *loading_label = lv_label_create(list_wifi_name);
    lv_label_set_text(loading_label, "Scanning for WiFi networks...");
    lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);

    // Start scanning WiFi networks asynchronously
    xTaskCreate(
        scan_and_update_wifi_list,        // Function to implement the task
        "scan_and_update_list_wifi_name", // Name of the task
        4096,                             // Stack size in words
        (void *)list_wifi_name,           // Task input parameter
        1,                                // Priority of the task
        NULL                              // Task handle
    );
    create_modal_bg_wifi_name_click();
}

static void refresh_wifi_list(lv_event_t *e)
{
    lv_obj_t *list_wifi_name = childMap["list_wifi_name"];
    xTaskCreate(
        scan_and_update_wifi_list,        // Function to implement the task
        "scan_and_update_list_wifi_name", // Name of the task
        4096,                             // Stack size in words
        (void *)list_wifi_name,           // Task input parameter
        1,                                // Priority of the task
        NULL                              // Task handle
    );
}

static void update_wifi_list(lv_obj_t *list_wifi_name, const std::vector<WifiNetwork> &wifiList)
{
    if (lv_obj_is_valid(list_wifi_name))
    {
        lvgl_port_lock(-1);           // needs to lock since this function is ruinning on different thread
        lv_obj_clean(list_wifi_name); // Clear the current list
        if (wifiList.empty())
        {
            lv_list_add_btn(list_wifi_name, LV_SYMBOL_WIFI, "NO WIFI FOUND");
        }
        else
        {
            for (size_t i = 0; i < wifiList.size(); ++i)
            {
                lv_obj_t *btn = lv_list_add_btn(list_wifi_name, LV_SYMBOL_WIFI, wifiList[i].ssid.c_str());
                // Add checkmark to the connected WiFi
                if (is_connected_to_certain_wifi(wifiList[i].ssid.c_str()) == 1)
                {   
                    lv_obj_t *label = lv_label_create(btn);
                    lv_label_set_text(label, LV_SYMBOL_OK " ");      // Add checkmark symbol
                    lv_obj_align(label, LV_ALIGN_RIGHT_MID, -10, 0); // Align to the right side
                }

                // Add signal strength text
                String signal_text;
                if (wifiList[i].rssi > -50)
                {
                    signal_text = "(" + (String)wifiList[i].rssi + ") " + "Strong";
                }
                else if (wifiList[i].rssi > -70)
                {
                    signal_text = "(" + (String)wifiList[i].rssi + ") " + "Fair";
                }
                else
                {
                    signal_text = "(" + (String)wifiList[i].rssi + ") " + "Weak";
                }
                lv_obj_t *signal_label = lv_label_create(btn);
                lv_label_set_text(signal_label, signal_text.c_str());
                lv_obj_align(signal_label, LV_ALIGN_RIGHT_MID, -50, 0); // Adjust alignment as needed

                // Dynamically allocate memory for the ssid
                char *ssid_copy = strdup(wifiList[i].ssid.c_str());
                // Set event handler for button click
                lv_obj_add_event_cb(btn, wifi_click_name_event_cb, LV_EVENT_CLICKED, (void *)ssid_copy);
                // Set event handler for button delete to free memory
                lv_obj_add_event_cb(btn, wifi_delete_event_cb, LV_EVENT_DELETE, (void *)ssid_copy);
            }
        }
        lvgl_port_unlock();
    }
}

static void scan_and_update_wifi_list(void *pvParameters)
{
    if (xSemaphoreTake(mutexWifiScan, portMAX_DELAY) == pdTRUE)
    {
        lv_obj_t *list_wifi_name = (lv_obj_t *)pvParameters;
        std::vector<WifiNetwork> wifiList = scan_wifi_networks();

        update_wifi_list(list_wifi_name, wifiList);
        xSemaphoreGive(mutexWifiScan);
    }
    vTaskDelete(NULL); // Delete this task when done
}



lv_obj_t *bg_modal_wifi_name_click;
lv_obj_t *cont_popup;
lv_obj_t *wifi_name;
lv_obj_t *textarea;
lv_obj_t *keyboard;

static void create_modal_bg_wifi_name_click()
{
    // Create a modal background
    bg_modal_wifi_name_click = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(bg_modal_wifi_name_click, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bg_modal_wifi_name_click, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg_modal_wifi_name_click, LV_OPA_0, 0);
    lv_obj_set_size(bg_modal_wifi_name_click, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(bg_modal_wifi_name_click);
    lv_obj_add_event_cb(bg_modal_wifi_name_click, wifi_name_modal_bg_event_cb, LV_EVENT_CLICKED, NULL);

    // Create a container for the popup
    cont_popup = lv_obj_create(bg_modal_wifi_name_click);
    lv_obj_set_size(cont_popup, 430, 370);
    lv_obj_center(cont_popup);
    lv_obj_clear_flag(cont_popup, LV_OBJ_FLAG_SCROLLABLE);

    wifi_name = lv_label_create(cont_popup);
    lv_obj_align(wifi_name, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(wifi_name, 250, 30);

    // Create a Textarea for password input
    textarea = lv_textarea_create(cont_popup);
    lv_textarea_set_password_mode(textarea, true);
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_placeholder_text(textarea, "Enter password");
    lv_obj_set_size(textarea, 250, 50);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 50);

    // Create a Keyboard
    keyboard = lv_keyboard_create(cont_popup);
    lv_keyboard_set_textarea(keyboard, textarea);
    lv_obj_set_size(keyboard, 400, 210);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Add event handler to the Button
    lv_obj_add_event_cb(keyboard, keyboard_enter_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_flag(bg_modal_wifi_name_click, LV_OBJ_FLAG_HIDDEN);
}

static void wifi_click_name_event_cb(lv_event_t *e) // Show the keyboard when the user clicks on the any wifi name
{
    // Get the clicked object from the event
    lv_obj_t *wifiClicked = lv_event_get_target(e);
    const char *wifiName = (const char *)e->user_data;
    Serial.println("wifiName");
    Serial.println(wifiName);
    if ((is_connected_to_wifi() == 1) && (is_connected_to_certain_wifi(wifiName) == 1))
    {
        Serial.println("Connected");
        disconnect_from_wifi();
    }
    else
    {
        Serial.println("notConnected");
        if (is_stored_password(wifiName))
        {
            Serial.println("strored password");
            connect_to_wifi(wifiName);
        }
        else
        {
            Serial.println("Key!");
            const char *param = (const char *)e->user_data;
            char text[100];
            sprintf(text, "Connect to \"%s\"", param);
            lv_label_set_text(wifi_name, text);
            lv_obj_clear_flag(bg_modal_wifi_name_click, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void wifi_delete_event_cb(lv_event_t *e)
{
    const char *user_data = static_cast<const char *>(lv_event_get_user_data(e));
    free(const_cast<char *>(user_data));
}

static void keyboard_enter_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb); // Get button that was clicked

        lv_obj_t *textarea = lv_keyboard_get_textarea(e->target);
        const char *password = lv_textarea_get_text(textarea);
        Serial.println(password);
        if (btn_id == 39)
        {
            // tick
            if (strlen(password) != 0)
            {
                connect_to_wifi(extract_text_in_quotes(lv_label_get_text(wifi_name)).c_str(), password);
            }
            lv_obj_add_flag(bg_modal_wifi_name_click, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void wifi_list_modal_bg_event_cb(lv_event_t *e) // close
{
    lv_obj_t *modal_bg = lv_event_get_target(e);
    lv_obj_del(modal_bg);                 // Close the modal background
    lv_obj_del(bg_modal_wifi_name_click); // Close the modal background of key boards
}

static void wifi_name_modal_bg_event_cb(lv_event_t *e) // close
{
    lv_obj_add_flag(bg_modal_wifi_name_click, LV_OBJ_FLAG_HIDDEN);
}

static void button_close_enter_event_handler(lv_event_t *e)
{
    lv_obj_t *modal_bg = (lv_obj_t *)e->user_data;
    lv_obj_del(modal_bg); // Close the modal background
}

static String extract_text_in_quotes(const String &input)
{
    String prefix = "Connect to \"";
    int startIndex = input.indexOf(prefix);
    if (startIndex == -1)
    {
        return "";
    }
    startIndex += prefix.length();
    int endIndex = input.indexOf('"', startIndex);
    if (endIndex == -1)
    {
        return "";
    }
    return input.substring(startIndex, endIndex);
}
static void check_status(lv_timer_t *timer)
{
    int wifi_status = is_connected_to_wifi();
    
    if (wifi_status == 1)
    {
        lv_label_set_text(label_wifi_status, LV_SYMBOL_OK);
        lv_style_set_text_color(&style_label_wifi_status, lv_color_make(0, 255, 0));
    }
    else
    {
        lv_label_set_text(label_wifi_status, LV_SYMBOL_CLOSE);
        lv_style_set_text_color(&style_label_wifi_status, lv_color_make(255, 0, 0));
    }
    
    lv_obj_refresh_style(label_wifi_status, LV_PART_MAIN, LV_STYLE_TEXT_COLOR);
}

