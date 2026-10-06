#ifndef WATCH_VIEW_INTERNAL_H
#define WATCH_VIEW_INTERNAL_H
#include "watch_ui.h"
#include "watch_faces.h"
#include "viewmodels/watch_viewmodels.h"
#include "services/watch_display_service.h"
#include <stdio.h>
#include <string.h>
enum { HOME, MENU, CALENDAR, SETTINGS, ALARMS, ALARM_EDIT, CALCULATOR, NOTES, NOTE_EDIT, FACES, WATER, WATER_SETTINGS, HISTORY, CLOCK_EDIT, PUZZLE, FLAPPY, SNAKE, IDA, SCREEN_COUNT };
extern lv_obj_t *screens[SCREEN_COUNT], *page_titles[SCREEN_COUNT];
extern unsigned back_targets[SCREEN_COUNT];
void button_motion(lv_obj_t *button);
void watch_navigation_show(unsigned screen);
void watch_menu_init(void);
void watch_menu_reset(void);
bool watch_menu_back(void);
void arcade_init(void);
void flappy_open(void);
void snake_open(void);
void arcade_leave(unsigned screen);
void ida_init(void);
extern bool display_awake;
extern int displayed_notification;
#define ui_clock_valid (watch_app_vm_state()->clock_valid)
#define current_year (watch_app_vm_state()->time.year)
#define current_month (watch_app_vm_state()->time.month)
#define current_day (watch_app_vm_state()->time.day)
#define current_hour (watch_app_vm_state()->time.hour)
#define current_minute (watch_app_vm_state()->time.minute)
#define current_second (watch_app_vm_state()->time.second)
#define twelve_hour (watch_settings_vm_twelve())
#define light_theme (watch_settings_vm_light())
void set_label(lv_obj_t *label,const char *value);
lv_obj_t *label_at(lv_obj_t *parent,const char *text,int y,const lv_font_t *font);
lv_obj_t *button_at(lv_obj_t *parent,const char *text,int x,int y,int width,lv_event_cb_t callback,void *data);
lv_obj_t *round_action(lv_obj_t *parent,const char *text,int x,int y,int diameter,uint32_t color,lv_event_cb_t callback,void *data);
void style_roller(lv_obj_t *roller);
void page_header(unsigned screen,const char *title,unsigned back);
void navigate(lv_event_t *e);
void refresh_time(void);
void apps_refresh(void);
void save_data(void);
void storage_refresh(void);
void settings_init(void);
void settings_refresh(void);
void theme_apply(void);
void calendar_init(void);
void calendar_refresh(void);
void clock_editor_init(void);
void clock_editor_open(void);
void clock_status_refresh(void);
void alarms_init(void);
void alarms_refresh(void);
void calculator_init(void);
void notes_init(void);
void notes_refresh(void);
void water_init(void);
void water_refresh(void);
void notifications_init(void);
void notifications_refresh(void);
void watch_navigation_init(void);
void puzzle_init(void);
void puzzle_open(void);
void puzzle_refresh(void);
#endif
