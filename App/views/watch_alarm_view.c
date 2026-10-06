#include "views/watch_view_internal.h"
static lv_obj_t *alarm_labels[WATCH_ALARMS],*alarm_toggle_labels[WATCH_ALARMS],*alarm_repeats[WATCH_ALARMS],*alarm_switches[WATCH_ALARMS],*alarm_hour,*alarm_minute,*alarm_repeat_label;

static void alarm_toggle(lv_event_t *e) { watch_alarm_vm_toggle((unsigned)(uintptr_t)lv_event_get_user_data(e)); storage_refresh(); apps_refresh(); }
static void alarm_edit(lv_event_t *e)
{
    watch_alarm_vm_open((unsigned)(uintptr_t)lv_event_get_user_data(e));
    const watch_alarm_editor_t *editor=watch_alarm_vm_editor();
    lv_roller_set_selected(alarm_hour,editor->hour,LV_ANIM_OFF); lv_roller_set_selected(alarm_minute,editor->minute,LV_ANIM_OFF);
    set_label(alarm_repeat_label,editor->repeat); watch_navigation_show(ALARM_EDIT);
}
static void alarm_repeat(lv_event_t *e) { (void)e; watch_alarm_vm_repeat(); set_label(alarm_repeat_label,watch_alarm_vm_editor()->repeat); }
static void alarm_save(lv_event_t *e)
{
    (void)e; watch_alarm_vm_save(lv_roller_get_selected(alarm_hour),lv_roller_get_selected(alarm_minute));
    storage_refresh(); apps_refresh(); watch_navigation_show(ALARMS);
}

void alarms_refresh(void)
{    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        watch_alarm_row_t row=watch_alarm_vm_row(i);
        const watch_alarm_row_t *a=&row;
        set_label(alarm_labels[i],row.time);
        set_label(alarm_repeats[i],row.repeat);
        set_label(alarm_toggle_labels[i],row.toggle);
        lv_obj_t *card=lv_obj_get_parent(alarm_labels[i]);
        lv_obj_set_style_bg_color(card,lv_color_hex(light_theme ? 0xFFFFFF : 0x152033),0);
        lv_obj_set_style_text_color(card,lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC),0);
        lv_obj_set_style_text_color(alarm_repeats[i],lv_color_hex(light_theme ? 0x586C83 : 0xA8B9D2),0);
        lv_obj_set_style_border_color(card,lv_color_hex(a->enabled ? 0x66BBA1 : light_theme ? 0xCDD8E6 : 0x2A3B52),0);
        lv_obj_set_style_bg_color(lv_obj_get_parent(alarm_toggle_labels[i]), lv_color_hex(a->enabled ? 0x1E685F : 0x253147), 0);
        if(a->enabled) lv_obj_add_state(alarm_switches[i],LV_STATE_CHECKED);
        else lv_obj_remove_state(alarm_switches[i],LV_STATE_CHECKED);
    }
}

void alarms_init(void)
{    page_header(ALARMS, "Alarmlar", MENU);
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        int width=i==1 ? 332 : 308;
        alarm_labels[i]=button_at(screens[ALARMS],"",0,112+i*104,width,alarm_edit,(void *)(uintptr_t)i);
        lv_obj_t *card=lv_obj_get_parent(alarm_labels[i]); lv_obj_set_height(card,88);
        lv_obj_set_style_radius(card,32,0); lv_obj_set_style_border_width(card,1,0);
        lv_obj_set_style_text_font(alarm_labels[i],&lv_font_montserrat_32,0);
        lv_obj_align(alarm_labels[i],LV_ALIGN_LEFT_MID,22,-12);
        alarm_repeats[i]=lv_label_create(card);
        lv_obj_set_style_text_font(alarm_repeats[i],&lv_font_montserrat_14,0);
        lv_obj_align(alarm_repeats[i],LV_ALIGN_BOTTOM_LEFT,24,-12);
        alarm_toggle_labels[i]=button_at(card,"",width/2-52,16,76,alarm_toggle,(void *)(uintptr_t)i);
        lv_obj_t *toggle=lv_obj_get_parent(alarm_toggle_labels[i]); lv_obj_set_height(toggle,56);
        lv_obj_set_style_radius(toggle,24,0); lv_obj_set_style_text_font(alarm_toggle_labels[i],&lv_font_montserrat_14,0);
        lv_obj_align(alarm_toggle_labels[i],LV_ALIGN_BOTTOM_MID,0,-7);
        alarm_switches[i]=lv_switch_create(toggle); lv_obj_set_size(alarm_switches[i],36,18);
        lv_obj_align(alarm_switches[i],LV_ALIGN_TOP_MID,0,8);
        lv_obj_remove_flag(alarm_switches[i],LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_anim_duration(alarm_switches[i],120,0);
        lv_obj_set_style_bg_color(alarm_switches[i],lv_color_hex(0x96E1C2),LV_PART_INDICATOR|LV_STATE_CHECKED);
    }
    label_at(screens[ALARMS], "Saate dokunarak duzenleyin", 429, &lv_font_montserrat_14);
    page_header(ALARM_EDIT, "Alarm ayari", ALARMS);
    char options[180] = {0};
    for(unsigned i = 0; i < 24; ++i) { char n[5]; snprintf(n, sizeof n, i == 23 ? "%02u" : "%02u\n", i); strcat(options, n); }
    alarm_hour = lv_roller_create(screens[ALARM_EDIT]);
    style_roller(alarm_hour);
    lv_roller_set_options(alarm_hour, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_hour, 3); lv_obj_set_width(alarm_hour, 112);
    lv_obj_align(alarm_hour, LV_ALIGN_TOP_MID, -68, 112);
    options[0] = '\0';
    for(unsigned i = 0; i < 60; ++i) { char n[5]; snprintf(n, sizeof n, i == 59 ? "%02u" : "%02u\n", i); strcat(options, n); }
    alarm_minute = lv_roller_create(screens[ALARM_EDIT]);
    style_roller(alarm_minute);
    lv_roller_set_options(alarm_minute, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_minute, 3); lv_obj_set_width(alarm_minute, 112);
    lv_obj_align(alarm_minute, LV_ALIGN_TOP_MID, 68, 112);
    lv_obj_set_style_text_font(alarm_hour, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_font(alarm_hour, &lv_font_montserrat_28, LV_PART_SELECTED);
    lv_obj_set_style_text_font(alarm_minute, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_font(alarm_minute, &lv_font_montserrat_28, LV_PART_SELECTED);
    label_at(screens[ALARM_EDIT], ":", 150, &lv_font_montserrat_24);
    label_at(screens[ALARM_EDIT], "SAAT            DAKIKA", 242, &lv_font_montserrat_16);
    alarm_repeat_label = button_at(screens[ALARM_EDIT], "", 0, 277, 294, alarm_repeat, NULL);
    button_at(screens[ALARM_EDIT], "Kaydet", 0, 351, 260, alarm_save, NULL);

}
