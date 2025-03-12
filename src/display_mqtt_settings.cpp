#include "display_mqtt_settings.h"
#include "utils_mqtt_device_store.h"
#include "utils_mqtt_server.h"
#include <regex>
#include "display_device_control_page.h"

static lv_obj_t *label_mqtt_status;
static lv_style_t style_label_mqtt_status;

// Function to create the UI (public interface, no static keyword)
void create_status_bar_mqtt_icons(lv_obj_t *parent)
{
    static lv_style_t style_fonts_icons;
    lv_style_init(&style_fonts_icons);
    lv_style_set_text_font(&style_fonts_icons, &lv_font_montserrat_22);

    lv_obj_t *cont_mqtt = lv_obj_create(parent);
    lv_obj_set_size(cont_mqtt, 105, 40);
    lv_obj_set_pos(cont_mqtt, 250, 0);
    lv_obj_add_flag(cont_mqtt, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(cont_mqtt, LV_OBJ_FLAG_SCROLLABLE);
    // lv_obj_add_event_cb(status_cont, wifi_icon_event_cb, LV_EVENT_CLICKED, NULL);
    // Remove any default padding or border for status container
    lv_obj_set_style_pad_all(cont_mqtt, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_mqtt, 0, LV_PART_MAIN);
    // Set status bar style: straight corners

    lv_obj_set_style_radius(cont_mqtt, 0, LV_PART_MAIN);
    lv_obj_t *label_mqtt_icon = lv_label_create(cont_mqtt);
    lv_obj_set_pos(label_mqtt_icon, 0, 1);
    lv_obj_add_style(label_mqtt_icon, &style_fonts_icons, LV_PART_MAIN);
    lv_label_set_text(label_mqtt_icon, "MQTT");

    lv_style_init(&style_label_mqtt_status);
    lv_style_set_text_font(&style_label_mqtt_status, &lv_font_montserrat_32);

    label_mqtt_status = lv_label_create(cont_mqtt);
    lv_obj_set_pos(label_mqtt_status, 70, -3);
    lv_obj_add_style(label_mqtt_status, &style_label_mqtt_status, LV_PART_MAIN);
    lv_label_set_text(label_mqtt_status, "");
    // lv_obj_t *label_mqtt = lv_label_create(parent);

    lv_timer_create(check_status, 1000, NULL);
    lv_obj_add_event_cb(cont_mqtt, show_mqtt_container, LV_EVENT_CLICKED, NULL);
}

// Callback function to show the MQTT input container (static, as it is only used in this file)
static void show_mqtt_container(lv_event_t *e)
{
    // connect_to_mqtt();
    // test_night_light_control();

    lv_obj_t *bg_modal = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(bg_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bg_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg_modal, LV_OPA_50, 0);
    lv_obj_set_size(bg_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(bg_modal);
    lv_obj_add_event_cb(bg_modal, mqtt_container_modal_bg_event_cb, LV_EVENT_CLICKED, NULL);

    // Create a tabview
    lv_obj_t *tabview_mqtt_settings = lv_tabview_create(bg_modal, LV_DIR_TOP, 50);
    lv_obj_set_size(tabview_mqtt_settings, 700, 400);
    lv_obj_center(tabview_mqtt_settings);

    lv_obj_t *tab_mqtt_server = lv_tabview_add_tab(tabview_mqtt_settings, "Server");
    create_mqtt_server_settings_display(tab_mqtt_server);

    // Tab 2: Equipments topic IDs
    lv_obj_t *tab_mqtt_equipments = lv_tabview_add_tab(tabview_mqtt_settings, "Equipments");
    lv_obj_add_event_cb(tabview_mqtt_settings, tab_mqtt_overall_settings_event_cb, LV_EVENT_VALUE_CHANGED, tab_mqtt_equipments);

    // lv_obj_add_event_cb(tab_mqtt_equipments, tab_mqtt_overall_settings_event_cb, LV_EVENT_CLICKED, NULL);
    //  lv_obj_t *tabview_mqtt_equipments_settings = lv_tabview_create(tab_mqtt_equipments, LV_DIR_BOTTOM, 50);

    // create_device_list_display(tab_mqtt_equipments);
}

static void create_mqtt_server_settings_display(lv_obj_t *parent)
{

    lv_obj_t *cont_two_top_button = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_two_top_button, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_two_top_button, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_two_top_button, LV_OPA_0, 0);
    lv_obj_set_size(cont_two_top_button, 250, 50);
    lv_obj_add_flag(cont_two_top_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(cont_two_top_button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont_two_top_button, LV_FLEX_FLOW_ROW);

    // Connect button
    data_btn_t *data_btn_connect = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_connect->name = "Connect";
    data_btn_connect->type = 0;

    lv_obj_t *btn_connect = lv_btn_create(cont_two_top_button);
    lv_obj_set_size(btn_connect, 100, 40);
    lv_obj_t *label_btn_connect = lv_label_create(btn_connect);
    lv_label_set_text(label_btn_connect, "Connect");
    lv_obj_set_align(label_btn_connect, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_user_data(btn_connect, data_btn_connect);
    lv_obj_add_event_cb(btn_connect, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // disconnect button
    data_btn_t *data_btn_disconnect = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_disconnect->name = "Disconnect";
    data_btn_disconnect->type = 0;

    lv_obj_t *btn_disconnect = lv_btn_create(cont_two_top_button);
    lv_obj_set_size(btn_disconnect, 100, 40);
    lv_obj_t *label_btn_disconnect = lv_label_create(btn_disconnect);
    lv_label_set_text(label_btn_disconnect, "Disconnect");
    lv_obj_set_align(label_btn_disconnect, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_user_data(btn_disconnect, data_btn_disconnect);
    lv_obj_add_event_cb(btn_disconnect, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Server IP

    lv_obj_t *label_name_server_ip = lv_label_create(parent);
    lv_label_set_text(label_name_server_ip, "Server IP");
    lv_obj_align_to(label_name_server_ip, cont_two_top_button, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_t *ta_server_ip = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(ta_server_ip, get_mqtt_server_param(MQTT_PARAM_SERVER_IP).c_str());
    lv_obj_set_size(ta_server_ip, 250, 50);
    lv_obj_align_to(ta_server_ip, label_name_server_ip, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_server_ip = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_server_ip->name = "Server IP";
    data_btn_server_ip->confirmed = false;
    data_btn_server_ip->type = 1;
    data_btn_server_ip->text_area_or_ddl = ta_server_ip;
    lv_obj_add_event_cb(ta_server_ip, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_server_ip);

    lv_obj_t *btn_server_ip = lv_btn_create(parent);
    lv_obj_set_size(btn_server_ip, 100, 40);
    lv_obj_align_to(btn_server_ip, ta_server_ip, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_server_ip, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_server_ip, data_btn_server_ip);
    lv_obj_t *label_btn_server_ip = lv_label_create(btn_server_ip);
    lv_label_set_text(label_btn_server_ip, "Confirm");

    // Server Port

    lv_obj_t *label_name_server_port = lv_label_create(parent);
    lv_label_set_text(label_name_server_port, "Server Port");
    lv_obj_align_to(label_name_server_port, ta_server_ip, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_t *ta_server_port = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(ta_server_port, get_mqtt_server_param(MQTT_PARAM_SERVER_PORT).c_str());
    lv_obj_set_size(ta_server_port, 250, 50);
    lv_obj_align_to(ta_server_port, label_name_server_port, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_server_port = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_server_port->name = "Server Port";
    data_btn_server_port->confirmed = false;
    data_btn_server_port->type = 1;
    data_btn_server_port->text_area_or_ddl = ta_server_port;
    lv_obj_add_event_cb(ta_server_port, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_server_port);

    lv_obj_t *btn_server_port = lv_btn_create(parent);
    lv_obj_set_size(btn_server_port, 100, 40);
    lv_obj_align_to(btn_server_port, ta_server_port, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_server_port, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_server_port, data_btn_server_port);
    lv_obj_t *label_btn_server_port = lv_label_create(btn_server_port);
    lv_label_set_text(label_btn_server_port, "Confirm");

    // User

    lv_obj_t *label_name_user = lv_label_create(parent);
    lv_label_set_text(label_name_user, "User");
    lv_obj_align_to(label_name_user, ta_server_port, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_t *ta_user = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(ta_user, get_mqtt_server_param(MQTT_PARAM_USER).c_str());
    lv_obj_set_size(ta_user, 250, 50);
    lv_obj_align_to(ta_user, label_name_user, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_user = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_user->name = "User";
    data_btn_user->confirmed = false;
    data_btn_user->type = 1;
    data_btn_user->text_area_or_ddl = ta_user;
    lv_obj_add_event_cb(ta_user, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_user);

    lv_obj_t *btn_user = lv_btn_create(parent);
    lv_obj_set_size(btn_user, 100, 40);
    lv_obj_align_to(btn_user, ta_user, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_user, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_user, data_btn_user);
    lv_obj_t *label_btn_user = lv_label_create(btn_user);
    lv_label_set_text(label_btn_user, "Confirm");

    // Password

    lv_obj_t *label_name_password = lv_label_create(parent);
    lv_label_set_text(label_name_password, "Password");
    lv_obj_align_to(label_name_password, ta_user, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_t *ta_password = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(ta_password, get_mqtt_server_param(MQTT_PARAM_PASSWORD).c_str());
    lv_obj_set_size(ta_password, 250, 50);
    lv_obj_align_to(ta_password, label_name_password, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_password = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_password->name = "Password";
    data_btn_password->confirmed = false;
    data_btn_password->type = 1;
    data_btn_password->text_area_or_ddl = ta_password;
    lv_obj_add_event_cb(ta_password, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_password);

    lv_obj_t *btn_password = lv_btn_create(parent);
    lv_obj_set_size(btn_password, 100, 40);
    lv_obj_align_to(btn_password, ta_password, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_password, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_password, data_btn_password);
    lv_obj_t *label_btn_password = lv_label_create(btn_password);
    lv_label_set_text(label_btn_password, "Confirm");

    // Client ID

    lv_obj_t *label_name_client_id = lv_label_create(parent);
    lv_label_set_text(label_name_client_id, "Client ID");
    lv_obj_align_to(label_name_client_id, ta_password, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_t *ta_client_id = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(ta_client_id, get_mqtt_server_param(MQTT_PARAM_CLIENT_ID).c_str());
    lv_obj_set_size(ta_client_id, 250, 50);
    lv_obj_align_to(ta_client_id, label_name_client_id, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_client_id = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_client_id->name = "Client ID";
    data_btn_client_id->confirmed = false;
    data_btn_client_id->type = 1;
    data_btn_client_id->text_area_or_ddl = ta_client_id;
    lv_obj_add_event_cb(ta_client_id, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_client_id);

    lv_obj_t *btn_client_id = lv_btn_create(parent);
    lv_obj_set_size(btn_client_id, 100, 40);
    lv_obj_align_to(btn_client_id, ta_client_id, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_client_id, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_client_id, data_btn_client_id);
    lv_obj_t *label_btn_client_id = lv_label_create(btn_client_id);
    lv_label_set_text(label_btn_client_id, "Confirm");
}

// Callback function when the background is clicked, clear the detailed information window and return to the device list
static void device_editing_page_modal_bg_event_cb(lv_event_t *e)
{
    lv_obj_t *modal_bg = lv_event_get_target(e);
    lv_obj_t *tab2 = (lv_obj_t *)e->user_data;

    std::vector<mqtt_device_t> *mqttEquipments = nullptr;

    mqttEquipments = (std::vector<mqtt_device_t> *)lv_obj_get_user_data(tab2); // get old device list and delete them
    if (mqttEquipments != nullptr)
    {
        delete mqttEquipments;
        mqttEquipments = nullptr;
        lv_obj_set_user_data(tab2, nullptr);
        Serial.println("old device deleted");
    }

    lv_obj_clean(tab2);
    lv_obj_t *list = lv_list_create(tab2);
    mqttEquipments = new std::vector<mqtt_device_t>(get_all_devices());

    lv_obj_set_user_data(tab2, mqttEquipments);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    for (size_t i = 0; i < mqttEquipments->size(); i++)
    {
        mqtt_device_t &mqttDevice = (*mqttEquipments)[i];

        // Create list item button and set name
        lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_FILE, mqttDevice.name.c_str());
        if (btn != NULL)
        {
            lv_obj_set_user_data(btn, &mqttDevice); // Put the pointer to the device data in the user_data of the button

            lv_obj_add_event_cb(btn, device_select_event_cb, LV_EVENT_CLICKED, tab2);
        }
        else
        {
            Serial.printf("Failed to create button for device %d\n", i);
        }
    }
    Serial.println("MQTT list container modal bg event cb");

    lv_obj_del(modal_bg);
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    data_btn_t *data = (data_btn_t *)lv_event_get_user_data(e);

    static lv_obj_t *kb_container; // keyboard container
    static lv_obj_t *kb;           // keyboard

    if (!kb_container)
    {
        kb_container = lv_obj_create(lv_scr_act());
        lv_obj_clear_flag(kb_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(kb_container, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(kb_container, LV_OPA_0, 0);
        lv_obj_set_size(kb_container, LV_HOR_RES, LV_VER_RES);
        lv_obj_add_event_cb(kb_container, kb_and_container_event_cb, LV_EVENT_CLICKED, NULL);
        kb = lv_keyboard_create(kb_container);
        lv_obj_add_event_cb(kb, kb_and_container_event_cb, LV_EVENT_VALUE_CHANGED, kb_container);
    }

    lv_keyboard_set_textarea(kb, ta);
    lv_obj_move_foreground(kb_container);
    lv_obj_clear_flag(kb_container, LV_OBJ_FLAG_HIDDEN);
}

static void kb_and_container_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e); // It's container
    lv_obj_t *kb_or_container = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_add_flag(kb_or_container, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {

        uint32_t btn_id = lv_keyboard_get_selected_btn(kb_or_container); // It's keyboard
        const char *txt = lv_keyboard_get_btn_text(kb_or_container, btn_id);

        if (btn_id == 39)
        { // close the keyboard
            lv_obj_t *kb_container = (lv_obj_t *)e->user_data;
            lv_obj_add_flag(kb_container, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// cancel button cb
static void cancel_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *cancel_btn = lv_event_get_target(e);
    lv_obj_t *confirm_btn = (lv_obj_t *)e->user_data;
    data_btn_t *data = (data_btn_t *)lv_obj_get_user_data(confirm_btn);
    // delete cancel button
    lv_obj_del_async(cancel_btn);

    // back to normal
    data->cancel_btn = NULL;
    data->confirmed = false;

    lv_label_set_text(lv_obj_get_child(confirm_btn, 0), "Confirm");
    lv_obj_set_size(confirm_btn, 100, 40);
}

// confirm button cb
static void confirm_btn_event_cb(lv_event_t *e)
{

    lv_obj_t *button = lv_event_get_target(e);
    data_btn_t *data = (data_btn_t *)lv_obj_get_user_data(button);

    if (data->type == 0)
    {
        if (data->name == "Connect")
        {
            connect_to_mqtt();
        }
        else if (data->name == "Disconnect")
        {
            disconnect_from_mqtt();
        }
        else if (data->name == "Add")
        {
            lv_obj_t *parent = lv_obj_get_parent(button);
            lv_obj_t *label = lv_obj_get_child(parent, 0);
            data->device->isAdded = true;
            lv_label_set_text(label, "This device has been added to main screen");
            if(update_device(*data->device)){
                refreshMainpage();
            }
            
        }
        else if (data->name == "Remove")
        {
            lv_obj_t *parent = lv_obj_get_parent(button);
            lv_obj_t *label = lv_obj_get_child(parent, 0);
            data->device->isAdded = false;
            lv_label_set_text(label, "This device has not been added to main screen");
            if(update_device(*data->device)){
                refreshMainpage();
            }
        }
    }
    else if ((data->type == 1) || (data->type == 2) || (data->type == 3))
    {
        if (!data->confirmed)
        {
            // first time click confirm button, display cancel button
            lv_obj_t *cancel_btn = lv_btn_create(lv_obj_get_parent(button));
            lv_obj_set_size(cancel_btn, 90, 40);
            lv_obj_align_to(cancel_btn, button, LV_ALIGN_OUT_RIGHT_MID, 50, 0);
            lv_obj_add_event_cb(cancel_btn, cancel_btn_event_cb, LV_EVENT_CLICKED, button);
            lv_obj_t *cancel_btn_label = lv_label_create(cancel_btn);
            lv_label_set_text(cancel_btn_label, "Cancel");

            // update confirmation status
            data->confirmed = true;
            lv_label_set_text(lv_obj_get_child(button, 0), "Are you sure?"); // lv_obj_get_child(btn, 0) means the label in the button
            lv_obj_set_size(button, 140, 40);

            // save cancel button's pointer
            data->cancel_btn = cancel_btn;
        }
        else
        {
            // The second time click confirm button
            printf("%s button confirmed\n", data->name);

            // Delete cancel button
            if (data->cancel_btn)
            {
                lv_obj_del_async(data->cancel_btn);
                data->cancel_btn = NULL;
            }

            if ((data->type == 1) || (data->type == 2))
            {
                // Save the text area's text
                if (!is_empty_or_whitespace(lv_textarea_get_text(data->text_area_or_ddl)))
                {
                    handle_textarea_after_confirm(data);
                }
            }
            else if (data->type == 3)
            {
                handle_textarea_after_confirm(data);
            }

            // back to normal
            data->confirmed = false;
            lv_label_set_text(lv_obj_get_child(button, 0), "Confirm");
            lv_obj_set_size(button, 100, 40);
        }
    }
}

static void handle_textarea_after_confirm(data_btn_t *data)
{
    if (data->type == 1)
    {
        const char *text = lv_textarea_get_text(data->text_area_or_ddl);
        MqttParam param;
        if (data->name == "Server IP")
        {
            param = MQTT_PARAM_SERVER_IP;
        }
        else if (data->name == "Server Port")
        {
            param = MQTT_PARAM_SERVER_PORT;
        }
        else if (data->name == "User")
        {
            param = MQTT_PARAM_USER;
        }
        else if (data->name == "Password")
        {
            param = MQTT_PARAM_PASSWORD;
        }
        else if (data->name == "Client ID")
        {
            param = MQTT_PARAM_CLIENT_ID;
        }
        else
        {
            return;
        }

        if (data->name == "Server IP" && !is_valid_ip(text))
        {
            lv_textarea_set_text(data->text_area_or_ddl, "");
            return;
        }
        if (data->name == "Server Port" && !is_valid_port(text))
        {
            lv_textarea_set_text(data->text_area_or_ddl, "");
            return;
        }
        if (store_mqtt_server_param(param, text)) // if there is no problem, store them
        {
            lv_textarea_set_text(data->text_area_or_ddl, "");
            lv_textarea_set_placeholder_text(data->text_area_or_ddl, get_mqtt_server_param(param).c_str());
        }
    }
    else if (data->type == 2)
    {
        if (data->name == "Name")
        {
            if (data->device->name == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->name = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "State Topic")
        {
            if (data->device->stateTopic == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->stateTopic = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "Command Topic")
        {
            if (data->device->commandTopic == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->commandTopic = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "Brightness State Topic")
        {
            if (data->device->stateTopicBrightness == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->stateTopicBrightness = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "Brightness Command Topic")
        {
            if (data->device->commandTopicBrightness == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->commandTopicBrightness = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "RGB State Topic")
        {
            if (data->device->stateTopicRgb == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->stateTopicRgb = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "RGB Command Topic")
        {
            if (data->device->commandTopicRgb == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->commandTopicRgb = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "Availability Topic")
        {
            if (data->device->availabilityTopic == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->availabilityTopic = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "Config Topic")
        {
            if (data->device->configTopic == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->configTopic = lv_textarea_get_text(data->text_area_or_ddl);
        }
        else if (data->name == "Battery State Topic")
        {
            if (data->device->stateTopicBattery == lv_textarea_get_text(data->text_area_or_ddl))
            {
                return;
            }
            data->device->stateTopicBattery = lv_textarea_get_text(data->text_area_or_ddl);
        }

        if (update_device(*data->device)) // This is the saving function
        {
            lv_textarea_set_text(data->text_area_or_ddl, "");
            if (data->name == "Name")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->name.c_str());
            }
            else if (data->name == "State Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->stateTopic.c_str());
            }
            else if (data->name == "Command Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->commandTopic.c_str());
            }
            else if (data->name == "Brightness State Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->stateTopicBrightness.c_str());
            }
            else if (data->name == "Brightness Command Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->commandTopicBrightness.c_str());
            }
            else if (data->name == "RGB State Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->stateTopicRgb.c_str());
            }
            else if (data->name == "RGB Command Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->commandTopicRgb.c_str());
            }
            else if (data->name == "Availability Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->availabilityTopic.c_str());
            }
            else if (data->name == "Config Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->configTopic.c_str());
            }
            else if (data->name == "Battery State Topic")
            {
                lv_textarea_set_placeholder_text(data->text_area_or_ddl, data->device->stateTopicBattery.c_str());
            }
        }
    }
    else if (data->type == 3)
    {
        if (data->name == "Equipment Type")
        {
            if (data->device->type == lv_dropdown_get_selected(data->text_area_or_ddl))
            {
                return;
            }
            data->device->type = (MqttDeviceType)lv_dropdown_get_selected(data->text_area_or_ddl);
            update_device(*data->device); // If there is no problem, store them
        }
        else if (data->name == "Message Type")
        {
            if (data->device->messageType == lv_dropdown_get_selected(data->text_area_or_ddl))
            {
                return;
            }
            data->device->type = (MqttDeviceType)lv_dropdown_get_selected(data->text_area_or_ddl);
            update_device(*data->device); // If there is no problem, store them
        }
    }
}

static bool is_empty_or_whitespace(const char *str)
{
    while (*str != '\0')
    {
        if (!isspace(*str))
        {
            return false;
        }
        str++;
    }
    return true;
}

static bool is_valid_ip(const char *str)
{
    std::regex ip_regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return std::regex_match(str, ip_regex);
}

static bool is_valid_port(const char *str)
{
    std::regex port_regex("^\\d{4}$");
    return std::regex_match(str, port_regex);
}

static void check_status(lv_timer_t *timer)
{
    if (is_connected_to_mqtt_server())
    {
        lv_label_set_text(label_mqtt_status, LV_SYMBOL_OK);
        lv_style_set_text_color(&style_label_mqtt_status, lv_color_make(0, 255, 0));
        lv_obj_refresh_style(label_mqtt_status, LV_PART_MAIN, LV_STYLE_TEXT_COLOR);
    }
    else
    {
        lv_label_set_text(label_mqtt_status, LV_SYMBOL_CLOSE);
        lv_style_set_text_color(&style_label_mqtt_status, lv_color_make(255, 0, 0));
        lv_obj_refresh_style(label_mqtt_status, LV_PART_MAIN, LV_STYLE_TEXT_COLOR);
    }
}

static void mqtt_container_modal_bg_event_cb(lv_event_t *e)
{
    lv_obj_t *modal_bg = lv_event_get_target(e);
    lv_obj_t *tabview = lv_obj_get_child(modal_bg, 0);
    lv_obj_t *tabs = lv_tabview_get_content(tabview);
    lv_obj_t *tab2 = lv_obj_get_child(tabs, 1);

    std::vector<mqtt_device_t> *mqttEquipments = (std::vector<mqtt_device_t> *)lv_obj_get_user_data(tab2);
    if (mqttEquipments != nullptr)
    {
        delete mqttEquipments;
        mqttEquipments = nullptr;
        lv_obj_set_user_data(tab2, nullptr);
    }
    else
    {
        Serial.println("container :mqttEquipments is null!");
    }
    lv_obj_del(modal_bg);
}

static void tab_mqtt_overall_settings_event_cb(lv_event_t *e)
{
    lv_obj_t *tabview = lv_event_get_target(e);
    uint16_t tab_id = lv_tabview_get_tab_act(tabview);

    if ((tab_id == 0) || (tab_id == 1)) // Check if it's a valid touch
    {
        lv_obj_t *tabs = lv_tabview_get_content(tabview);
        if (tab_id == 1)
        { // Check if it's the second tab
            // Update the content of tab2
            std::vector<mqtt_device_t> *mqttEquipments = new std::vector<mqtt_device_t>(get_all_devices());
            // Clear the content of the device list container
            lv_obj_t *tab2 = lv_obj_get_child(tabs, 1);

            Serial.printf("mqttEquipments size: %d \n", mqttEquipments->size());
            lv_obj_clean(tab2);
            lv_obj_t *list = lv_list_create(tab2);
            lv_obj_set_user_data(tab2, mqttEquipments);

            lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
            for (size_t i = 0; i < mqttEquipments->size(); i++)
            {
                mqtt_device_t &mqttDevice = (*mqttEquipments)[i];

                // Create a list item button and set the name
                lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_FILE, mqttDevice.name.c_str());
                if (btn != NULL)
                {
                    lv_obj_set_user_data(btn, &mqttDevice); // Store the pointer to the device data in the user_data of the button

                    lv_obj_add_event_cb(btn, device_select_event_cb, LV_EVENT_CLICKED, tab2);
                }
                else
                {
                    Serial.printf("Failed to create button for device %d\n", i);
                }
            }
        }
        else if (tab_id == 0)
        {
            Serial.println("Switching to Server tab, cleaning up Equipments");
            lv_obj_t *tab2 = lv_obj_get_child(tabs, 1);

            std::vector<mqtt_device_t> *mqttEquipments = (std::vector<mqtt_device_t> *)lv_obj_get_user_data(tab2);
            // Directly release the memory of mqttEquipments
            if (mqttEquipments != nullptr)
            {
                delete mqttEquipments;
                mqttEquipments = nullptr;
                lv_obj_set_user_data(tab2, nullptr);
                lv_obj_clean(tab2);
                lv_obj_t *list = lv_list_create(tab2);
            }
            else
            {
                Serial.println("mqttEquipments is null, nothing to clean up");
            }
        }
    }
}

// Print the device information after click the device
static void device_select_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        // Create a modal window
        lv_obj_t *screen = lv_scr_act();
        lv_obj_t *bg_modal = lv_obj_create(screen);
        lv_obj_clear_flag(bg_modal, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(bg_modal, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(bg_modal, LV_OPA_0, 0);
        lv_obj_set_size(bg_modal, lv_obj_get_width(screen), lv_obj_get_height(screen));
        lv_obj_center(bg_modal);
        lv_obj_add_event_cb(bg_modal, device_editing_page_modal_bg_event_cb, LV_EVENT_CLICKED, (lv_obj_t *)e->user_data);

        lv_obj_t *tabview_device_settings = lv_tabview_create(bg_modal, LV_DIR_TOP, 50);
        lv_obj_set_size(tabview_device_settings, 700, 400);
        lv_obj_center(tabview_device_settings);
        lv_obj_add_flag(tabview_device_settings, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_pad_all(tabview_device_settings, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(tabview_device_settings, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(tabview_device_settings, 0, LV_PART_MAIN);

        lv_obj_t *tab_mqtt_settings_basic = lv_tabview_add_tab(tabview_device_settings, "Basic");
        lv_obj_t *tab_mqtt_settings_lights = lv_tabview_add_tab(tabview_device_settings, "Lights");
        lv_obj_t *tab_mqtt_settings_others = lv_tabview_add_tab(tabview_device_settings, "Others");
        lv_obj_t *tab_mqtt_settings_json = lv_tabview_add_tab(tabview_device_settings, "JSON");

        lv_obj_set_flex_flow(tab_mqtt_settings_basic, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab_mqtt_settings_basic, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_flow(tab_mqtt_settings_lights, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab_mqtt_settings_lights, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_flow(tab_mqtt_settings_others, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab_mqtt_settings_others, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_flow(tab_mqtt_settings_json, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab_mqtt_settings_json, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        // Create a tabview for the device settings
        lv_obj_t *btn = lv_event_get_target(e);
        mqtt_device_t *device = (mqtt_device_t *)lv_obj_get_user_data(btn);
        lv_obj_set_user_data(tabview_device_settings, device); // Add this device to the user_data of the tabview
        lv_obj_add_event_cb(tabview_device_settings, tab_mqtt_device_settings_event_cb, LV_EVENT_VALUE_CHANGED, device);
        lv_obj_set_user_data(tabview_device_settings, (void *)"Type");

        create_mqtt_device_basic_settings(tab_mqtt_settings_basic, device);
        create_mqtt_device_lights_settings(tab_mqtt_settings_lights, device);
        create_mqtt_device_others_settings(tab_mqtt_settings_others, device);
        create_mqtt_device_json_settings(tab_mqtt_settings_json, device);

        device_type_visibility(device, tabview_device_settings);
    }
}

// Those are for basic settings
static void create_mqtt_device_basic_settings(lv_obj_t *parent, mqtt_device_t *device)
{
    //
    // Create subscription selection
    //

    lv_obj_t *cont_add_to_main_screen = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_add_to_main_screen, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_add_to_main_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_add_to_main_screen, LV_OPA_0, 0);
    lv_obj_set_size(cont_add_to_main_screen, 400, 80);

    lv_obj_t *label_add_to_main_screen = lv_label_create(cont_add_to_main_screen);

    if (device->isAdded)
    {
        lv_label_set_text(label_add_to_main_screen, "This device has been added to main screen");
    }
    else
    {
        lv_label_set_text(label_add_to_main_screen, "This device has not been added to main screen");
    }

    lv_obj_set_align(label_add_to_main_screen, LV_FLEX_ALIGN_START);

    // Add button
    data_btn_t *data_btn_add = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_add->name = "Add";
    data_btn_add->type = 0;
    data_btn_add->device = device;

    lv_obj_t *btn_add = lv_btn_create(cont_add_to_main_screen);
    lv_obj_set_size(btn_add, 100, 40);
    lv_obj_align_to(btn_add, label_add_to_main_screen, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    lv_obj_t *label_btn_add = lv_label_create(btn_add);
    lv_label_set_text(label_btn_add, "Add");
    lv_obj_set_align(label_btn_add, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_user_data(btn_add, data_btn_add);
    lv_obj_add_event_cb(btn_add, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Remove button
    data_btn_t *data_btn_remove = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_remove->name = "Remove";
    data_btn_remove->type = 0;
    data_btn_remove->device = device;

    lv_obj_t *btn_remove = lv_btn_create(cont_add_to_main_screen);
    lv_obj_set_size(btn_remove, 100, 40);
    lv_obj_align_to(btn_remove, btn_add, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_t *label_btn_remove = lv_label_create(btn_remove);
    lv_label_set_text(label_btn_remove, "Remove");
    lv_obj_set_align(label_btn_remove, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_user_data(btn_remove, data_btn_remove);
    lv_obj_add_event_cb(btn_remove, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);

    //
    // Create dropdown for device type selection
    //

    lv_obj_t *cont_equipment_type = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_equipment_type, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_equipment_type, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_equipment_type, LV_OPA_0, 0);
    lv_obj_set_size(cont_equipment_type, 630, 80);
    lv_obj_t *label_equipment_type = lv_label_create(cont_equipment_type);
    lv_label_set_text(label_equipment_type, "Equipment Type");
    lv_obj_t *ddlist_equipment_type = lv_dropdown_create(cont_equipment_type);
    lv_obj_set_user_data(ddlist_equipment_type, (void *)"Equipment Type");
    lv_dropdown_set_options_static(ddlist_equipment_type, "SENSOR_BINARY\nSENSOR_NON_BINARY\nDEVICE_CONTROLLABLE_BINARY\nLIGHT_BRIGHTNESS_CONTROL\nLIGHT_RGB_CONTROL");
    lv_dropdown_set_selected(ddlist_equipment_type, device->type);

    lv_obj_set_style_text_align(ddlist_equipment_type, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(ddlist_equipment_type, 370, 50);
    lv_obj_align_to(ddlist_equipment_type, label_equipment_type, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_equipment_type = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_equipment_type->name = "Equipment Type";
    data_btn_equipment_type->confirmed = false;
    data_btn_equipment_type->type = 3;
    data_btn_equipment_type->device = device;
    data_btn_equipment_type->text_area_or_ddl = ddlist_equipment_type;
    lv_obj_add_event_cb(ddlist_equipment_type, device_type_event_cb, LV_EVENT_VALUE_CHANGED, device);

    lv_obj_t *btn_equipment_type = lv_btn_create(cont_equipment_type);
    lv_obj_set_size(btn_equipment_type, 100, 40);
    lv_obj_align_to(btn_equipment_type, ddlist_equipment_type, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_equipment_type, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_equipment_type, data_btn_equipment_type);
    lv_obj_t *label_btn_equipment_type = lv_label_create(btn_equipment_type);
    lv_label_set_text(label_btn_equipment_type, "Confirm");

    //
    // Create dropdown for device message type selection
    //

    lv_obj_t *cont_message_type = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_message_type, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_message_type, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_message_type, LV_OPA_0, 0);
    lv_obj_set_size(cont_message_type, 630, 80);
    lv_obj_t *label_message_type = lv_label_create(cont_message_type);
    lv_label_set_text(label_message_type, "Message Type");
    lv_obj_t *ddlist_message_type = lv_dropdown_create(cont_message_type);
    lv_obj_set_user_data(ddlist_message_type, (void *)"Message Type");
    lv_dropdown_set_options_static(ddlist_message_type, "TRADITIONAL\nJSON\nRAW_JSON");
    lv_dropdown_set_selected(ddlist_message_type, device->messageType);

    lv_obj_set_style_text_align(ddlist_message_type, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(ddlist_message_type, 370, 50);
    lv_obj_align_to(ddlist_message_type, label_message_type, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_message_type = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_message_type->name = "Message Type";
    data_btn_message_type->confirmed = false;
    data_btn_message_type->type = 3;
    data_btn_message_type->device = device;
    data_btn_message_type->text_area_or_ddl = ddlist_message_type;
    lv_obj_add_event_cb(ddlist_message_type, device_type_event_cb, LV_EVENT_VALUE_CHANGED, device);

    lv_obj_t *btn_message_type = lv_btn_create(cont_message_type);
    lv_obj_set_size(btn_message_type, 100, 40);
    lv_obj_align_to(btn_message_type, ddlist_message_type, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_message_type, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_message_type, data_btn_message_type);
    lv_obj_t *label_btn_message_type = lv_label_create(btn_message_type);
    lv_label_set_text(label_btn_message_type, "Confirm");

    //
    // Name
    //

    lv_obj_t *cont_name = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_name, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_name, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_name, LV_OPA_0, 0);
    lv_obj_set_size(cont_name, 630, 80);
    lv_obj_t *label_name = lv_label_create(cont_name);
    lv_label_set_text(label_name, "Name");
    lv_obj_t *ta_name = lv_textarea_create(cont_name);
    lv_textarea_set_placeholder_text(ta_name, device->name.c_str());
    lv_obj_set_size(ta_name, 370, 50);
    lv_obj_align_to(ta_name, label_name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_name = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_name->name = "Name";
    data_btn_name->confirmed = false;
    data_btn_name->type = 2;
    data_btn_name->device = device;
    data_btn_name->text_area_or_ddl = ta_name;
    lv_obj_add_event_cb(ta_name, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_name);

    lv_obj_t *btn_name = lv_btn_create(cont_name);
    lv_obj_set_size(btn_name, 100, 40);
    lv_obj_align_to(btn_name, ta_name, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_name, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_name, data_btn_name);
    lv_obj_t *label_btn_name = lv_label_create(btn_name);
    lv_label_set_text(label_btn_name, "Confirm");

    //
    // State topic
    //

    lv_obj_t *cont_state = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_state, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_state, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_state, LV_OPA_0, 0);
    lv_obj_set_size(cont_state, 630, 80);
    lv_obj_t *label_state = lv_label_create(cont_state);
    lv_label_set_text(label_state, "State Topic");
    lv_obj_t *ta_state = lv_textarea_create(cont_state);
    lv_textarea_set_placeholder_text(ta_state, device->stateTopic.c_str());
    Serial.println(device->stateTopic.c_str());
    lv_obj_set_size(ta_state, 370, 50);
    lv_obj_align_to(ta_state, label_state, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_state = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_state->name = "State Topic";
    data_btn_state->confirmed = false;
    data_btn_state->type = 2;
    data_btn_state->device = device;
    data_btn_state->text_area_or_ddl = ta_state;
    lv_obj_add_event_cb(ta_state, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_state);

    lv_obj_t *btn_state = lv_btn_create(cont_state);
    lv_obj_set_size(btn_state, 100, 40);
    lv_obj_align_to(btn_state, ta_state, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_state, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_state, data_btn_state);
    lv_obj_t *label_btn_state = lv_label_create(btn_state);
    lv_label_set_text(label_btn_state, "Confirm");

    //
    // Command topic
    //

    lv_obj_t *cont_command = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_command, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_command, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_command, LV_OPA_0, 0);
    lv_obj_set_size(cont_command, 630, 80);
    lv_obj_t *label_command = lv_label_create(cont_command);
    lv_label_set_text(label_command, "Command Topic");
    lv_obj_t *ta_command = lv_textarea_create(cont_command);
    lv_textarea_set_placeholder_text(ta_command, device->commandTopic.c_str());
    lv_obj_set_size(ta_command, 370, 50);
    lv_obj_align_to(ta_command, label_command, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_command = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_command->name = "Command Topic";
    data_btn_command->confirmed = false;
    data_btn_command->type = 2;
    data_btn_command->device = device;
    data_btn_command->text_area_or_ddl = ta_command;
    lv_obj_add_event_cb(ta_command, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_command);

    lv_obj_t *btn_command = lv_btn_create(cont_command);
    lv_obj_set_size(btn_command, 100, 40);
    lv_obj_align_to(btn_command, ta_command, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_command, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_command, data_btn_command);
    lv_obj_t *label_btn_command = lv_label_create(btn_command);
    lv_label_set_text(label_btn_command, "Confirm");
}

// Those are for lights settings
static void create_mqtt_device_lights_settings(lv_obj_t *parent, mqtt_device_t *device)
{
    //
    // Brightness state topic
    //

    lv_obj_t *cont_brightness_state = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_brightness_state, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_brightness_state, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_brightness_state, LV_OPA_0, 0);
    lv_obj_set_size(cont_brightness_state, 630, 80);
    lv_obj_t *label_brightness_state = lv_label_create(cont_brightness_state);
    lv_label_set_text(label_brightness_state, "Brightness State Topic");
    lv_obj_t *ta_brightness_state = lv_textarea_create(cont_brightness_state);
    lv_textarea_set_placeholder_text(ta_brightness_state, device->stateTopicBrightness.c_str());
    lv_obj_set_size(ta_brightness_state, 370, 50);
    lv_obj_align_to(ta_brightness_state, label_brightness_state, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_brightness_state = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_brightness_state->name = "Brightness State Topic";
    data_btn_brightness_state->confirmed = false;
    data_btn_brightness_state->type = 2;
    data_btn_brightness_state->device = device;
    data_btn_brightness_state->text_area_or_ddl = ta_brightness_state;
    lv_obj_add_event_cb(ta_brightness_state, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_brightness_state);

    lv_obj_t *btn_brightness_state = lv_btn_create(cont_brightness_state);
    lv_obj_set_size(btn_brightness_state, 100, 40);
    lv_obj_align_to(btn_brightness_state, ta_brightness_state, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_brightness_state, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_brightness_state, data_btn_brightness_state);
    lv_obj_t *label_btn_brightness_state = lv_label_create(btn_brightness_state);
    lv_label_set_text(label_btn_brightness_state, "Confirm");

    //
    // Brightness command topic
    //

    lv_obj_t *cont_brightness_command = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_brightness_command, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_brightness_command, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_brightness_command, LV_OPA_0, 0);
    lv_obj_set_size(cont_brightness_command, 630, 80);
    lv_obj_t *label_brightness_command = lv_label_create(cont_brightness_command);
    lv_label_set_text(label_brightness_command, "Brightness Command Topic");
    lv_obj_t *ta_brightness_command = lv_textarea_create(cont_brightness_command);
    lv_textarea_set_placeholder_text(ta_brightness_command, device->commandTopicBrightness.c_str());
    lv_obj_set_size(ta_brightness_command, 370, 50);
    lv_obj_align_to(ta_brightness_command, label_brightness_command, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_brightness_command = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_brightness_command->name = "Brightness Command Topic";
    data_btn_brightness_command->confirmed = false;
    data_btn_brightness_command->type = 2;
    data_btn_brightness_command->device = device;
    data_btn_brightness_command->text_area_or_ddl = ta_brightness_command;
    lv_obj_add_event_cb(ta_brightness_command, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_brightness_command);

    lv_obj_t *btn_brightness_command = lv_btn_create(cont_brightness_command);
    lv_obj_set_size(btn_brightness_command, 100, 40);
    lv_obj_align_to(btn_brightness_command, ta_brightness_command, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_brightness_command, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_brightness_command, data_btn_brightness_command);
    lv_obj_t *label_btn_brightness_command = lv_label_create(btn_brightness_command);
    lv_label_set_text(label_btn_brightness_command, "Confirm");

    //
    // RGB state topic
    //

    lv_obj_t *cont_rgb_state = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_rgb_state, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_rgb_state, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_rgb_state, LV_OPA_0, 0);
    lv_obj_set_size(cont_rgb_state, 630, 80);
    lv_obj_t *label_rgb_state = lv_label_create(cont_rgb_state);
    lv_label_set_text(label_rgb_state, "RGB State Topic");
    lv_obj_t *ta_rgb_state = lv_textarea_create(cont_rgb_state);
    lv_textarea_set_placeholder_text(ta_rgb_state, device->stateTopicRgb.c_str());
    lv_obj_set_size(ta_rgb_state, 370, 50);
    lv_obj_align_to(ta_rgb_state, label_rgb_state, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_rgb_state = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_rgb_state->name = "RGB State Topic";
    data_btn_rgb_state->confirmed = false;
    data_btn_rgb_state->type = 2;
    data_btn_rgb_state->device = device;
    data_btn_rgb_state->text_area_or_ddl = ta_rgb_state;
    lv_obj_add_event_cb(ta_rgb_state, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_rgb_state);

    lv_obj_t *btn_rgb_state = lv_btn_create(cont_rgb_state);
    lv_obj_set_size(btn_rgb_state, 100, 40);
    lv_obj_align_to(btn_rgb_state, ta_rgb_state, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_rgb_state, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_rgb_state, data_btn_rgb_state);
    lv_obj_t *label_btn_rgb_state = lv_label_create(btn_rgb_state);
    lv_label_set_text(label_btn_rgb_state, "Confirm");

    //
    // RGB command topic
    //

    lv_obj_t *cont_rgb_command = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_rgb_command, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_rgb_command, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_rgb_command, LV_OPA_0, 0);
    lv_obj_set_size(cont_rgb_command, 630, 80);
    lv_obj_t *label_rgb_command = lv_label_create(cont_rgb_command);
    lv_label_set_text(label_rgb_command, "RGB Command Topic");
    lv_obj_t *ta_rgb_command = lv_textarea_create(cont_rgb_command);
    lv_textarea_set_placeholder_text(ta_rgb_command, device->stateTopicRgb.c_str());
    lv_obj_set_size(ta_rgb_command, 370, 50);
    lv_obj_align_to(ta_rgb_command, label_rgb_command, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_rgb_command = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_rgb_command->name = "RGB Command Topic";
    data_btn_rgb_command->confirmed = false;
    data_btn_rgb_command->type = 2;
    data_btn_rgb_command->device = device;
    data_btn_rgb_command->text_area_or_ddl = ta_rgb_command;
    lv_obj_add_event_cb(ta_rgb_command, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_rgb_command);

    lv_obj_t *btn_rgb_command = lv_btn_create(cont_rgb_command);
    lv_obj_set_size(btn_rgb_command, 100, 40);
    lv_obj_align_to(btn_rgb_command, ta_rgb_command, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_rgb_command, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_rgb_command, data_btn_rgb_command);
    lv_obj_t *label_btn_rgb_command = lv_label_create(btn_rgb_command);
    lv_label_set_text(label_btn_rgb_command, "Confirm");
}

// Those are for others settings
static void create_mqtt_device_others_settings(lv_obj_t *parent, mqtt_device_t *device)
{
    //
    // Availability Topic
    //

    lv_obj_t *cont_avail = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_avail, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_avail, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_avail, LV_OPA_0, 0);
    lv_obj_set_size(cont_avail, 630, 80);
    lv_obj_t *label_avail = lv_label_create(cont_avail);
    lv_label_set_text(label_avail, "Availability Topic");
    lv_obj_t *ta_avail = lv_textarea_create(cont_avail);
    lv_textarea_set_placeholder_text(ta_avail, device->availabilityTopic.c_str());
    lv_obj_set_size(ta_avail, 370, 50);
    lv_obj_align_to(ta_avail, label_avail, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_avail = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_avail->name = "Availability Topic";
    data_btn_avail->confirmed = false;
    data_btn_avail->type = 2;
    data_btn_avail->device = device;
    data_btn_avail->text_area_or_ddl = ta_avail;
    lv_obj_add_event_cb(ta_avail, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_avail);

    lv_obj_t *btn_avail = lv_btn_create(cont_avail);
    lv_obj_set_size(btn_avail, 100, 40);
    lv_obj_align_to(btn_avail, ta_avail, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_avail, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_avail, data_btn_avail);
    lv_obj_t *label_btn_avail = lv_label_create(btn_avail);
    lv_label_set_text(label_btn_avail, "Confirm");

    //
    // Configration Topic
    //

    lv_obj_t *cont_config = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_config, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_config, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_config, LV_OPA_0, 0);
    lv_obj_set_size(cont_config, 630, 80);
    lv_obj_t *label_config = lv_label_create(cont_config);
    lv_label_set_text(label_config, "Config Topic");
    lv_obj_t *ta_config = lv_textarea_create(cont_config);
    lv_textarea_set_placeholder_text(ta_config, device->configTopic.c_str());
    lv_obj_set_size(ta_config, 370, 50);
    lv_obj_align_to(ta_config, label_config, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_config = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_config->name = "Config Topic";
    data_btn_config->confirmed = false;
    data_btn_config->type = 2;
    data_btn_config->device = device;
    data_btn_config->text_area_or_ddl = ta_config;
    lv_obj_add_event_cb(ta_config, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_config);

    lv_obj_t *btn_config = lv_btn_create(cont_config);
    lv_obj_set_size(btn_config, 100, 40);
    lv_obj_align_to(btn_config, ta_config, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_config, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_config, data_btn_config);
    lv_obj_t *label_btn_config = lv_label_create(btn_config);
    lv_label_set_text(label_btn_config, "Confirm");

    //
    // Battery State Topic
    //

    lv_obj_t *cont_battery_state = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_battery_state, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_battery_state, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_battery_state, LV_OPA_0, 0);
    lv_obj_set_size(cont_battery_state, 630, 80);
    lv_obj_t *label_battery_state = lv_label_create(cont_battery_state);
    lv_label_set_text(label_battery_state, "Battery State Topic");
    lv_obj_t *ta_battery_state = lv_textarea_create(cont_battery_state);
    lv_textarea_set_placeholder_text(ta_battery_state, device->stateTopicBattery.c_str());
    lv_obj_set_size(ta_battery_state, 370, 50);
    lv_obj_align_to(ta_battery_state, label_battery_state, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_battery_state = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_battery_state->name = "Battery State Topic";
    data_btn_battery_state->confirmed = false;
    data_btn_battery_state->type = 2;
    data_btn_battery_state->device = device;
    data_btn_battery_state->text_area_or_ddl = ta_battery_state;
    lv_obj_add_event_cb(ta_battery_state, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_battery_state);

    lv_obj_t *btn_battery_state = lv_btn_create(cont_battery_state);
    lv_obj_set_size(btn_battery_state, 100, 40);
    lv_obj_align_to(btn_battery_state, ta_battery_state, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_battery_state, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_battery_state, data_btn_battery_state);
    lv_obj_t *label_btn_battery_state = lv_label_create(btn_battery_state);
    lv_label_set_text(label_btn_battery_state, "Confirm");
}

// Those are for JSON device settings
static void create_mqtt_device_json_settings(lv_obj_t *parent, mqtt_device_t *device)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN); 
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    //
    // Friendly name of state topic
    //

    lv_obj_t *cont_friendly_name_state = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_friendly_name_state, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_friendly_name_state, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_friendly_name_state, LV_OPA_0, 0);
    lv_obj_set_size(cont_friendly_name_state, 630, 80);
    lv_obj_t *label_friendly_name_state = lv_label_create(cont_friendly_name_state);
    lv_label_set_text(label_friendly_name_state, "Friendly name for state topic");
    lv_obj_t *ta_friendly_name_state = lv_textarea_create(cont_friendly_name_state);
    lv_textarea_set_placeholder_text(ta_friendly_name_state, device->friendlyNameState.c_str());
    lv_obj_set_size(ta_friendly_name_state, 370, 50);
    lv_obj_align_to(ta_friendly_name_state, label_friendly_name_state, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_friendly_name_state = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_friendly_name_state->name = "Friendly State";
    data_btn_friendly_name_state->confirmed = false;
    data_btn_friendly_name_state->type = 2;
    data_btn_friendly_name_state->device = device;
    data_btn_friendly_name_state->text_area_or_ddl = ta_friendly_name_state;
    lv_obj_add_event_cb(ta_friendly_name_state, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_friendly_name_state);

    lv_obj_t *btn_friendly_name_state = lv_btn_create(cont_friendly_name_state);
    lv_obj_set_size(btn_friendly_name_state, 100, 40);
    lv_obj_align_to(btn_friendly_name_state, ta_friendly_name_state, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_friendly_name_state, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_friendly_name_state, data_btn_friendly_name_state);
    lv_obj_t *label_btn_friendly_name_state = lv_label_create(btn_friendly_name_state);
    lv_label_set_text(label_btn_friendly_name_state, "Confirm");

    //
    // Friendly name of brightness topic
    //

    lv_obj_t *cont_friendly_name_brightness = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_friendly_name_brightness, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_friendly_name_brightness, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_friendly_name_brightness, LV_OPA_0, 0);
    lv_obj_set_size(cont_friendly_name_brightness, 630, 80);
    lv_obj_t *label_friendly_name_brightness = lv_label_create(cont_friendly_name_brightness);
    lv_label_set_text(label_friendly_name_brightness, "Friendly name for brightness topic");
    lv_obj_t *ta_friendly_name_brightness = lv_textarea_create(cont_friendly_name_brightness);
    lv_textarea_set_placeholder_text(ta_friendly_name_brightness, device->friendlyNameBrightness.c_str());
    lv_obj_set_size(ta_friendly_name_brightness, 370, 50);
    lv_obj_align_to(ta_friendly_name_brightness, label_friendly_name_brightness, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_friendly_name_brightness = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_friendly_name_brightness->name = "Friendly Brightness";
    data_btn_friendly_name_brightness->confirmed = false;
    data_btn_friendly_name_brightness->type = 2;
    data_btn_friendly_name_brightness->device = device;
    data_btn_friendly_name_brightness->text_area_or_ddl = ta_friendly_name_brightness;
    lv_obj_add_event_cb(ta_friendly_name_brightness, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_friendly_name_brightness);

    lv_obj_t *btn_friendly_name_brightness = lv_btn_create(cont_friendly_name_brightness);
    lv_obj_set_size(btn_friendly_name_brightness, 100, 40);
    lv_obj_align_to(btn_friendly_name_brightness, ta_friendly_name_brightness, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_friendly_name_brightness, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_friendly_name_brightness, data_btn_friendly_name_brightness);
    lv_obj_t *label_btn_friendly_name_brightness = lv_label_create(btn_friendly_name_brightness);
    lv_label_set_text(label_btn_friendly_name_brightness, "Confirm");

    //
    // Create dropdown for color type selection
    //

    lv_obj_t *cont_equipment_color_mode = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_equipment_color_mode, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_equipment_color_mode, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_equipment_color_mode, LV_OPA_0, 0);
    lv_obj_set_size(cont_equipment_color_mode, 630, 80);
    lv_obj_t *label_equipment_color_mode = lv_label_create(cont_equipment_color_mode);
    lv_label_set_text(label_equipment_color_mode, "Friendly name for color mode");
    lv_obj_t *ddlist_equipment_color_mode = lv_dropdown_create(cont_equipment_color_mode);
    lv_obj_set_user_data(ddlist_equipment_color_mode, (void *)"Color Mode");
    lv_dropdown_set_options_static(ddlist_equipment_color_mode, "RGB\nRGBW");
    lv_dropdown_set_selected(ddlist_equipment_color_mode, device->friendlyNameColorMode);

    lv_obj_set_style_text_align(ddlist_equipment_color_mode, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(ddlist_equipment_color_mode, 370, 50);
    lv_obj_align_to(ddlist_equipment_color_mode, label_equipment_color_mode, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_equipment_color_mode = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_equipment_color_mode->name = "Color Mode";
    data_btn_equipment_color_mode->confirmed = false;
    data_btn_equipment_color_mode->type = 3;
    data_btn_equipment_color_mode->device = device;
    data_btn_equipment_color_mode->text_area_or_ddl = ddlist_equipment_color_mode;
    lv_obj_add_event_cb(ddlist_equipment_color_mode, device_type_event_cb, LV_EVENT_VALUE_CHANGED, device);

    lv_obj_t *btn_equipment_color_mode = lv_btn_create(cont_equipment_color_mode);
    lv_obj_set_size(btn_equipment_color_mode, 100, 40);
    lv_obj_align_to(btn_equipment_color_mode, ddlist_equipment_color_mode, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_equipment_color_mode, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_equipment_color_mode, data_btn_equipment_color_mode);
    lv_obj_t *label_btn_equipment_type = lv_label_create(btn_equipment_color_mode);
    lv_label_set_text(label_btn_equipment_type, "Confirm");

    //
    // Friendly name for battery topic
    //

    lv_obj_t *cont_friendly_name_battery = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_friendly_name_battery, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_friendly_name_battery, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_friendly_name_battery, LV_OPA_0, 0);
    lv_obj_set_size(cont_friendly_name_battery, 630, 80);
    lv_obj_t *label_friendly_name_battery = lv_label_create(cont_friendly_name_battery);
    lv_label_set_text(label_friendly_name_battery, "Friendly name for battery topic");
    lv_obj_t *ta_friendly_name_battery = lv_textarea_create(cont_friendly_name_battery);
    lv_textarea_set_placeholder_text(ta_friendly_name_battery, device->friendlyNameBattery.c_str());
    lv_obj_set_size(ta_friendly_name_battery, 370, 50);
    lv_obj_align_to(ta_friendly_name_battery, label_friendly_name_battery, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_friendly_name_battery = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_friendly_name_battery->name = "Friendly Battery";
    data_btn_friendly_name_battery->confirmed = false;
    data_btn_friendly_name_battery->type = 2;
    data_btn_friendly_name_battery->device = device;
    data_btn_friendly_name_battery->text_area_or_ddl = ta_friendly_name_battery;
    lv_obj_add_event_cb(ta_friendly_name_battery, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_friendly_name_battery);

    lv_obj_t *btn_friendly_name_battery = lv_btn_create(cont_friendly_name_battery);
    lv_obj_set_size(btn_friendly_name_battery, 100, 40);
    lv_obj_align_to(btn_friendly_name_battery, ta_friendly_name_battery, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_friendly_name_battery, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_friendly_name_battery, data_btn_friendly_name_battery);
    lv_obj_t *label_btn_friendly_name_battery = lv_label_create(btn_friendly_name_battery);
    lv_label_set_text(label_btn_friendly_name_battery, "Confirm");

    //
    // Friendly name for link quality topic
    //

    lv_obj_t *cont_friendly_name_link_quality = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont_friendly_name_link_quality, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_friendly_name_link_quality, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_friendly_name_link_quality, LV_OPA_0, 0);
    lv_obj_set_size(cont_friendly_name_link_quality, 630, 80);
    lv_obj_t *label_friendly_name_link_quality = lv_label_create(cont_friendly_name_link_quality);
    lv_label_set_text(label_friendly_name_link_quality, "Config Topic");
    lv_obj_t *ta_friendly_name_link_quality = lv_textarea_create(cont_friendly_name_link_quality);
    lv_textarea_set_placeholder_text(ta_friendly_name_link_quality, device->friendlyLinkQuality.c_str());
    lv_obj_set_size(ta_friendly_name_link_quality, 370, 50);
    lv_obj_align_to(ta_friendly_name_link_quality, label_friendly_name_link_quality, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    data_btn_t *data_btn_friendly_name_link_quality = (data_btn_t *)malloc(sizeof(data_btn_t));
    data_btn_friendly_name_link_quality->name = "Config Topic";
    data_btn_friendly_name_link_quality->confirmed = false;
    data_btn_friendly_name_link_quality->type = 2;
    data_btn_friendly_name_link_quality->device = device;
    data_btn_friendly_name_link_quality->text_area_or_ddl = ta_friendly_name_link_quality;
    lv_obj_add_event_cb(ta_friendly_name_link_quality, keyboard_event_cb, LV_EVENT_CLICKED, data_btn_friendly_name_link_quality);

    lv_obj_t *btn_friendly_name_link_quality = lv_btn_create(cont_friendly_name_link_quality);
    lv_obj_set_size(btn_friendly_name_link_quality, 100, 40);
    lv_obj_align_to(btn_friendly_name_link_quality, ta_friendly_name_link_quality, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(btn_friendly_name_link_quality, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn_friendly_name_link_quality, data_btn_friendly_name_link_quality);
    lv_obj_t *label_btn_friendly_name_link_quality = lv_label_create(btn_friendly_name_link_quality);
    lv_label_set_text(label_btn_friendly_name_link_quality, "Confirm");



    lv_obj_t *label_json_settings_tab = lv_label_create(parent);
    lv_label_set_text(label_json_settings_tab, "Those Settings are for JSON devices only");
    lv_obj_set_align(label_json_settings_tab, LV_ALIGN_CENTER);
    lv_obj_align(label_json_settings_tab, LV_ALIGN_CENTER, 0, 0);

}
static void device_type_event_cb(lv_event_t *e)
{
    lv_obj_t *ddlist_type = lv_event_get_target(e);
    const char *type_ddlist = (char *)lv_obj_get_user_data(ddlist_type);
    mqtt_device_t *device = (mqtt_device_t *)lv_event_get_user_data(e);
    Serial.println(type_ddlist);
    if (type_ddlist == "Equipment Type")
    {
        device->type = (MqttDeviceType)lv_dropdown_get_selected(ddlist_type);
    }
    else if (type_ddlist == "Message Type")
    {
        device->messageType = (MqttDeviceMessageType)lv_dropdown_get_selected(ddlist_type);
    }
    lv_obj_t *tab = lv_obj_get_parent(lv_obj_get_parent(ddlist_type));
    lv_obj_t *tabview_device_settings = lv_obj_get_parent(lv_obj_get_parent(tab));
    device_type_visibility(device, tabview_device_settings);
}

static void device_type_visibility(mqtt_device_t *device, lv_obj_t *tabview_device_settings)
{
    uint16_t tab_id = lv_tabview_get_tab_act(tabview_device_settings);
    lv_obj_t *tabs = lv_tabview_get_content(tabview_device_settings);
    if (tab_id == 0) // Basic
    {
        lv_obj_t *tab1 = lv_obj_get_child(tabs, 0);
        lv_obj_t *cont_command = lv_obj_get_child(tab1, 5);
        if (device->type == SENSOR_BINARY || device->type == SENSOR_NON_BINARY)
        {
            lv_obj_add_flag(cont_command, LV_OBJ_FLAG_HIDDEN);
        }
        else if ((device->type == DEVICE_CONTROLLABLE_BINARY) ||
                 (device->type == LIGHT_BRIGHTNESS_CONTROL) ||
                 (device->type == LIGHT_BRIGHTNESS_CONTROL) ||
                 (device->type == LIGHT_RGB_CONTROL))
        {
            lv_obj_clear_flag(cont_command, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_update_layout(tab1);
    }
    else if (tab_id == 1) // Lights
    {
        lv_obj_t *tab2 = lv_obj_get_child(tabs, 1);
        lv_obj_t *cont_brightness_state = lv_obj_get_child(tab2, 0);
        lv_obj_t *cont_brightness_command = lv_obj_get_child(tab2, 1);
        lv_obj_t *cont_rgb_state = lv_obj_get_child(tab2, 2);
        lv_obj_t *cont_rgb_command = lv_obj_get_child(tab2, 3);

        if (device->type == SENSOR_BINARY ||
            device->type == SENSOR_NON_BINARY ||
            device->type == DEVICE_CONTROLLABLE_BINARY)
        {
            lv_obj_add_flag(cont_brightness_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_brightness_command, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_rgb_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_rgb_command, LV_OBJ_FLAG_HIDDEN);
        }
        else if (device->type == LIGHT_BRIGHTNESS_CONTROL)
        {
            lv_obj_clear_flag(cont_brightness_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cont_brightness_command, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_rgb_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_rgb_command, LV_OBJ_FLAG_HIDDEN);
        }
        else if (device->type == LIGHT_RGB_CONTROL)
        {
            lv_obj_clear_flag(cont_brightness_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cont_brightness_command, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cont_rgb_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cont_rgb_command, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_update_layout(tab2);
    }
    else if (tab_id == 3) // JSON
    {
        lv_obj_t *tab4 = lv_obj_get_child(tabs, 3);
        lv_obj_t *cont_friendly_name_state = lv_obj_get_child(tab4, 0);
        lv_obj_t *cont_friendly_name_brightness = lv_obj_get_child(tab4, 1);
        lv_obj_t *cont_equipment_color_mode = lv_obj_get_child(tab4, 2);
        lv_obj_t *cont_friendly_name_battery = lv_obj_get_child(tab4, 3);
        lv_obj_t *cont_friendly_name_link_quality = lv_obj_get_child(tab4, 4);
        lv_obj_t *label_json_settings_tab = lv_obj_get_child(tab4, 5);
        if (device->messageType == JSON)
        {
            if (device->type == SENSOR_BINARY || device->type == SENSOR_NON_BINARY)
            {
            }
            lv_obj_add_flag(label_json_settings_tab, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(cont_friendly_name_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_friendly_name_brightness, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_equipment_color_mode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_friendly_name_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_friendly_name_link_quality, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(label_json_settings_tab, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_align(label_json_settings_tab, LV_ALIGN_CENTER);
        }
        lv_obj_update_layout(tab4);
    }
}

static void tab_mqtt_device_settings_event_cb(lv_event_t *e)
{
    lv_obj_t *tabview = lv_event_get_target(e);
    mqtt_device_t *device = (mqtt_device_t *)lv_event_get_user_data(e);
    uint16_t tab_id = lv_tabview_get_tab_act(tabview);
    if ((tab_id == 0) || (tab_id == 1) || (tab_id == 2) || (tab_id == 3))
    { // Check if it's a valid touch
        device_type_visibility(device, tabview);
    }
}