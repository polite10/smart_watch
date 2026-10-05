#include "views/watch_view_internal.h"
static lv_obj_t *water_ring,*water_total,*water_remaining,*water_goal_label,*water_reminder_label,*history_labels[WATCH_HISTORY];

static void water_add(lv_event_t *e)
{
    if(!ui_clock_valid) { clock_editor_open(); return; }
    watch_water_vm_add((int)(intptr_t)lv_event_get_user_data(e)); storage_refresh(); apps_refresh(); notifications_refresh();
}
static void water_goal(lv_event_t *e)
{
    if(!ui_clock_valid) { clock_editor_open(); return; }
    watch_water_vm_goal((int)(intptr_t)lv_event_get_user_data(e)); storage_refresh(); apps_refresh();
}
static void water_reminder(lv_event_t *e) { (void)e; watch_water_vm_reminder(); storage_refresh(); apps_refresh(); notifications_refresh(); }
void water_refresh(void)
{
    watch_water_state_t state=watch_water_vm_state();
    lv_arc_set_value(water_ring,state.progress); set_label(water_total,state.total); set_label(water_remaining,state.remaining);
    set_label(water_goal_label,state.goal); set_label(water_reminder_label,state.reminder);
    char text[96]; for(unsigned i=0;i<WATCH_HISTORY;++i) { watch_water_vm_history(i,text,sizeof text); set_label(history_labels[i],text); }
}

void water_init(void)
{    page_header(WATER, "Su takibi", MENU);
    water_ring = lv_arc_create(screens[WATER]); lv_obj_set_size(water_ring, 206, 206);
    lv_obj_align(water_ring, LV_ALIGN_TOP_MID, 0, 103);
    lv_arc_set_rotation(water_ring, 270); lv_arc_set_bg_angles(water_ring, 0, 360);
    lv_arc_set_range(water_ring, 0, 100);
    lv_obj_set_style_arc_width(water_ring, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(water_ring, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(water_ring, lv_color_hex(0x253147), LV_PART_MAIN);
    lv_obj_set_style_arc_color(water_ring, lv_color_hex(0x82D9F4), LV_PART_INDICATOR);
    lv_obj_remove_style(water_ring, NULL, LV_PART_KNOB); lv_obj_remove_flag(water_ring, LV_OBJ_FLAG_CLICKABLE);
    water_total = label_at(screens[WATER], "0 ml", 159, &lv_font_montserrat_32);
    label_at(screens[WATER], "BUGUN", 201, &lv_font_montserrat_16);
    water_remaining = label_at(screens[WATER], "", 235, &lv_font_montserrat_16);
    lv_obj_set_width(water_remaining, 174); lv_obj_set_style_text_align(water_remaining, LV_TEXT_ALIGN_CENTER, 0);
    round_action(screens[WATER], LV_SYMBOL_MINUS, -148, 175, 72, 0x37465F, water_add, (void *)(intptr_t)-1);
    round_action(screens[WATER], LV_SYMBOL_PLUS, 148, 167, 88, 0x288BAD, water_add, (void *)(intptr_t)1);
    label_at(screens[WATER], "1 bardak / 200 ml", 319, &lv_font_montserrat_20);
    button_at(screens[WATER], "Hedef", -76, 357, 140, navigate, (void *)(uintptr_t)WATER_SETTINGS);
    button_at(screens[WATER], "Gecmis", 76, 357, 140, navigate, (void *)(uintptr_t)HISTORY);
    page_header(WATER_SETTINGS, "Su ayarlari", WATER);
    water_goal_label = label_at(screens[WATER_SETTINGS], "", 119, &lv_font_montserrat_24);
    round_action(screens[WATER_SETTINGS], "-200", -78, 167, 96, 0x37465F, water_goal, (void *)(intptr_t)-200);
    round_action(screens[WATER_SETTINGS], "+200", 78, 167, 96, 0x288BAD, water_goal, (void *)(intptr_t)200);
    label_at(screens[WATER_SETTINGS], "1 bardak = 200 ml", 266, &lv_font_montserrat_16);
    water_reminder_label = button_at(screens[WATER_SETTINGS], "", 0, 302, 294, water_reminder, NULL);
    label_at(screens[WATER_SETTINGS], "Kapali / 30 / 60 / 120 dk", 378, &lv_font_montserrat_16);
    label_at(screens[WATER_SETTINGS], "08:00 - 22:00", 410, &lv_font_montserrat_16);
    page_header(HISTORY, "Su gecmisi", WATER);
    lv_obj_t *list = lv_obj_create(screens[HISTORY]);
    lv_obj_set_size(list, 320, 290); lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 106);
    lv_obj_set_style_radius(list,100,0);
    lv_obj_set_style_border_width(list,0,0);
    lv_obj_set_style_bg_opa(list,LV_OPA_TRANSP,0);
    lv_obj_set_style_text_color(list, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_style_pad_top(list,40,0);
    lv_obj_set_style_pad_bottom(list,40,0);
    for(unsigned i = 0; i < WATCH_HISTORY; ++i) {
        lv_obj_t *row=lv_obj_create(list);
        lv_obj_set_size(row,278,46); lv_obj_set_pos(row,0,i*54);
        lv_obj_set_style_radius(row,23,0); lv_obj_set_style_pad_all(row,0,0);
        lv_obj_set_style_border_width(row,0,0);
        lv_obj_set_style_bg_color(row,lv_color_hex(0x152033),0);
        lv_obj_set_style_text_color(row,lv_color_hex(0xF0F4FC),0);
        lv_obj_remove_flag(row,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
        history_labels[i] = lv_label_create(row);
        lv_obj_set_style_text_font(history_labels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_width(history_labels[i],256);
        lv_obj_set_style_text_align(history_labels[i],LV_TEXT_ALIGN_CENTER,0);
        lv_obj_center(history_labels[i]); set_label(history_labels[i], "--");
    }
}
