#include "views/watch_view_internal.h"
#include "watch_icons.h"
lv_obj_t *screens[SCREEN_COUNT],*page_titles[SCREEN_COUNT];
unsigned back_targets[SCREEN_COUNT];

void refresh_time(void)
{
    watch_faces_set_datetime(current_year, current_month, current_day,
                             current_hour, current_minute, current_second, twelve_hour);
}

void apps_refresh(void)
{
    lv_obj_t *active=lv_screen_active();
    if(active==screens[HOME] || active==screens[FACES]) watch_faces_refresh();
    else if(active==screens[ALARMS] || active==screens[ALARM_EDIT]) alarms_refresh();
    else if(active==screens[NOTES] || active==screens[NOTE_EDIT]) notes_refresh();
    else if(active==screens[WATER] || active==screens[WATER_SETTINGS] || active==screens[HISTORY]) water_refresh();
    else if(active==screens[CALENDAR]) calendar_refresh();
    else if(active==screens[SETTINGS]) settings_refresh();
}
static void screen_loaded(lv_event_t *e) { (void)e; refresh_time(); apps_refresh(); }
static void state_changed(watch_vm_event_t event)
{
    static bool previous_clock_valid;
    static uint32_t previous_day;
    if(event==WATCH_VM_CLOCK) {
        bool lost=previous_clock_valid && !ui_clock_valid;
        previous_clock_valid=ui_clock_valid;
        watch_faces_set_clock_valid(ui_clock_valid); clock_status_refresh();
        if(lv_screen_active()==screens[CALENDAR]) calendar_refresh();
        notifications_refresh();
        if(!ui_clock_valid && (lost || lv_screen_active()==screens[HOME])) clock_editor_open();
        apps_refresh();
    } else {
        refresh_time();
        uint32_t today=watch_day_key(current_year,current_month,current_day);
        if(today!=previous_day && lv_screen_active()==screens[CALENDAR]) calendar_refresh();
        previous_day=today;
        if(current_second==0 || event==WATCH_VM_TIME_CHANGED) apps_refresh();
        notifications_refresh(); storage_refresh();
    }
}
void watch_ui_init(void)
{
    watch_app_vm_init(state_changed); watch_navigation_init();
    for(unsigned i = 0; i < SCREEN_COUNT; ++i) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_remove_flag(screens[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(screens[i], lv_color_hex(light_theme ? 0xEAF0F8 : 0x070D18), 0);
        lv_obj_set_style_text_color(screens[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
        lv_obj_set_style_text_font(screens[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_pad_all(screens[i], 0, 0);
        lv_obj_set_style_border_width(screens[i], 0, 0);
        lv_obj_add_event_cb(screens[i], screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    }
    watch_faces_init(screens[HOME], screens[FACES], navigate, (void *)(uintptr_t)MENU);
    watch_menu_init();

    page_header(CALENDAR,"Takvim",MENU); calendar_init(); settings_init();
    alarms_init(); calculator_init(); notes_init(); water_init(); puzzle_init(); arcade_init(); ida_init(); notifications_init(); clock_editor_init();
    apps_refresh(); refresh_time(); lv_screen_load(screens[HOME]);
}
void watch_ui_set_datetime(uint16_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second)
{ watch_app_vm_set_datetime((watch_datetime_t){year,month,day,hour,minute,second}); }
void watch_ui_set_clock_valid(bool valid) { watch_app_vm_set_valid(valid); }
void watch_ui_datetime_changed(uint16_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second)
{ watch_app_vm_time_changed((watch_datetime_t){year,month,day,hour,minute,second}); }
void watch_ui_set_battery(uint8_t percent,bool valid) { watch_faces_set_battery(percent,valid); }
