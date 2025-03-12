#include "display.h"


void init_display()
{
  ESP_Panel *panel = new ESP_Panel();
  
  panel->init();

#if LVGL_PORT_AVOID_TEAR
  // When avoid tearing function is enabled, configure the RGB bus according to the LVGL configuration
  ESP_PanelBus_RGB *rgb_bus = static_cast<ESP_PanelBus_RGB *>(panel->getLcd()->getBus());
  rgb_bus->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
  rgb_bus->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
#endif
  panel->begin();

  Serial.println("Initialize LVGL");
  lvgl_port_init(panel->getLcd(), panel->getTouch());

  Serial.println("Create UI");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  lvgl_port_lock(-1);

  // Create a main container
  lv_obj_t *main_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_cont, 800, 480);
  lv_obj_set_pos(main_cont, 0, 0);
  lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
  // Remove any default padding or border for main container
  lv_obj_set_style_pad_all(main_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(main_cont, 0, LV_PART_MAIN);

  create_status_bar(main_cont);

  lv_obj_t *tabview = lv_tabview_create(main_cont, LV_DIR_LEFT, 30);
  lv_obj_set_size(tabview, 800, 450);
  lv_obj_set_pos(tabview, 0, 30);
  lv_obj_set_style_pad_all(tabview, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(tabview, 0, LV_PART_MAIN);
  // Set status bar style: straight corners
  lv_obj_set_style_radius(tabview, 0, LV_PART_MAIN);
  //lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
  // Create tabs
  lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Tab 1");
  lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "Tab 2");
  //lv_obj_clear_flag(tab1, LV_OBJ_FLAG_SCROLLABLE);
  //lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);
  

  // Remove any default padding or border for status container
  lv_obj_set_style_pad_all(tab1, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(tab1, 0, LV_PART_MAIN);
  // Set status bar style: straight corners
  lv_obj_set_style_radius(tab1, 0, LV_PART_MAIN);

  init_device_control_page_display(tab1);

  /* Release the mutex */
  lvgl_port_unlock();
}

static void btn_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED)
  {
    static uint8_t cnt = 0;
    cnt++;
    /*Get the first child of the button which is the label and change its text*/
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text_fmt(label, "Button: %d", cnt);
  }
}

static void my_switch_event_cb(lv_event_t *e)
{
}



void create_switches(lv_obj_t *parent)
{
  // Create a container for the switches
  lv_obj_t *sw_cont = lv_obj_create(parent);
  lv_obj_set_size(sw_cont, 120, 300);
  lv_obj_set_pos(sw_cont, 30, 0);
  lv_obj_set_flex_flow(sw_cont, LV_FLEX_FLOW_COLUMN); // Change to LV_FLEX_FLOW_ROW for horizontal scrolling
  lv_obj_add_flag(sw_cont, LV_OBJ_FLAG_SCROLLABLE);
  // Remove any default padding or border for main container
  lv_obj_set_style_pad_all(sw_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(sw_cont, 0, LV_PART_MAIN);
  // Set status bar style: straight corners
  lv_obj_set_style_radius(sw_cont, 0, LV_PART_MAIN);

  // Create switches and labels
  const char *switch_names[] = {"Switch 1", "Switch 2", "Switch 3", "Switch 4"};
  for (int i = 0; i < 4; i++)
  {
    // Create switch
    lv_obj_t *sw = lv_switch_create(sw_cont);
    lv_obj_set_size(sw, 100, 50); // Adjust size of the switch
    lv_obj_add_event_cb(sw, my_switch_event_cb, LV_EVENT_VALUE_CHANGED, (void *)switch_names[i]);

    // Create label
    lv_obj_t *label = lv_label_create(sw_cont);
    lv_label_set_text(label, switch_names[i]);
    lv_obj_align_to(label, sw, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  }
}
