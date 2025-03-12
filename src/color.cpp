// color.c

#include "color.h"


lv_style_t dark_bg_style;
lv_style_t dark_text_style;
lv_style_t dark_btn_style;
lv_style_t dark_btn_pressed_style;
lv_style_t dark_separator_style;


void init_dark_theme() {
    lv_style_init(&dark_bg_style);
    lv_style_set_bg_color(&dark_bg_style, lv_color_hex(0x121212));

    lv_style_init(&dark_text_style);
    lv_style_set_text_color(&dark_text_style, lv_color_hex(0xFFFFFF));

    lv_style_init(&dark_btn_style);
    lv_style_set_bg_color(&dark_btn_style, lv_color_hex(0x303030));

    lv_style_init(&dark_btn_pressed_style);
    lv_style_set_bg_color(&dark_btn_pressed_style, lv_color_hex(0xBB86FC));

    lv_style_init(&dark_separator_style);
    lv_style_set_border_color(&dark_separator_style, lv_color_hex(0x666666));
    // 初始化更多样式
}


lv_style_t light_bg_style;
lv_style_t light_text_style;
lv_style_t light_btn_style;
lv_style_t light_btn_pressed_style;
lv_style_t light_separator_style;


void init_light_theme() {
    lv_style_init(&light_bg_style);
    lv_style_set_bg_color(&light_bg_style, lv_color_hex(0xFFFFFF));

    lv_style_init(&light_text_style);
    lv_style_set_text_color(&light_text_style, lv_color_hex(0x121212));

    lv_style_init(&light_btn_style);
    lv_style_set_bg_color(&light_btn_style, lv_color_hex(0xE0E0E0));

    lv_style_init(&light_btn_pressed_style);
    lv_style_set_bg_color(&light_btn_pressed_style, lv_color_hex(0x6200EE));

    lv_style_init(&light_separator_style);
    lv_style_set_border_color(&light_separator_style, lv_color_hex(0xCCCCCC));

}
