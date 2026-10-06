#include "views/watch_view_internal.h"

static const lv_style_prop_t button_motion_properties[]={LV_STYLE_TRANSFORM_SCALE_X,LV_STYLE_TRANSFORM_SCALE_Y,LV_STYLE_BG_COLOR,0};
static const lv_style_transition_dsc_t button_transition={
    .props=button_motion_properties,.path_xcb=lv_anim_path_ease_out,.time=90
};
static const lv_style_prop_t card_motion_properties[]={LV_STYLE_BG_COLOR,LV_STYLE_BORDER_COLOR,0};
static const lv_style_transition_dsc_t card_transition={
    .props=card_motion_properties,.path_xcb=lv_anim_path_ease_out,.time=90
};

void button_motion(lv_obj_t *button)
{
    /* Scaling a large card would allocate an expensive offscreen draw layer. */
    if(lv_obj_get_style_width(button,LV_PART_MAIN)>96) {
        lv_obj_set_style_transition(button,&card_transition,LV_STATE_DEFAULT);
        lv_obj_set_style_transition(button,&card_transition,LV_STATE_PRESSED);
        return;
    }
    lv_obj_set_style_transition(button,&button_transition,LV_STATE_DEFAULT);
    lv_obj_set_style_transition(button,&button_transition,LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_x(button,256,0);
    lv_obj_set_style_transform_scale_y(button,256,0);
    lv_obj_set_style_transform_scale_x(button,244,LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(button,244,LV_STATE_PRESSED);
    lv_obj_set_style_transform_pivot_x(button,lv_pct(50),0);
    lv_obj_set_style_transform_pivot_y(button,lv_pct(50),0);
}

void set_label(lv_obj_t *label, const char *value)
{
    /* An unchanged label must not allocate text or invalidate the display. */
    if(strcmp(lv_label_get_text(label), value)) lv_label_set_text(label, value);
}

lv_obj_t *label_at(lv_obj_t *parent, const char *text, int y, const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    set_label(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
    return l;
}

lv_obj_t *button_at(lv_obj_t *parent, const char *text, int x, int y,
                           int width, lv_event_cb_t callback, void *data)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, width, 60);
    lv_obj_align(b, LV_ALIGN_TOP_MID, x, y);
    lv_obj_set_style_radius(b, 30, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x26344C), 0);
    lv_obj_set_style_text_color(b, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_style_text_font(b, &lv_font_montserrat_20, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    button_motion(b);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x435675), LV_STATE_PRESSED);
    lv_obj_t *l = lv_label_create(b);
    set_label(l, text);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, callback, LV_EVENT_CLICKED, data);
    return l;
}

lv_obj_t *round_action(lv_obj_t *parent, const char *text, int x, int y,
                             int diameter, uint32_t color, lv_event_cb_t callback, void *data)
{
    lv_obj_t *label = button_at(parent, text, x, y, diameter, callback, data);
    lv_obj_t *button = lv_obj_get_parent(label);
    lv_obj_set_height(button, diameter);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    return label;
}

void style_roller(lv_obj_t *roller)
{
    lv_obj_set_style_radius(roller, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x152033), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xD9E4F6), LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x96E1C2), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_hex(0x182C2D), LV_PART_SELECTED);
}

void watch_ui_header(lv_obj_t *page, const char *title, lv_event_cb_t back, void *data)
{
    (void)back;
    lv_obj_t *label = label_at(page, title, 44, &lv_font_montserrat_24);
    lv_obj_set_width(label, 290);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
    for(unsigned i=0; i<SCREEN_COUNT; ++i) if(screens[i] == page) {
        page_titles[i] = label;
        back_targets[i] = (unsigned)(uintptr_t)data;
    }
}

void page_header(unsigned screen, const char *title, unsigned back)
{
    watch_ui_header(screens[screen], title, navigate, (void *)(uintptr_t)back);

}
