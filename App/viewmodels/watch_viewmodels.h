#ifndef WATCH_VIEWMODELS_H
#define WATCH_VIEWMODELS_H
#include "models/watch_datetime.h"
#include "models/watch_puzzle.h"

const watch_puzzle_state_t *watch_puzzle_vm_state(void);
void watch_puzzle_vm_new(uint32_t entropy);
bool watch_puzzle_vm_move(unsigned slot);
typedef struct { watch_datetime_t time; bool clock_valid, storage_ok, rtc_crystal; } watch_app_state_t;
typedef enum { WATCH_VM_TIME, WATCH_VM_CLOCK, WATCH_VM_TIME_CHANGED } watch_vm_event_t;
typedef void (*watch_vm_observer_t)(watch_vm_event_t event);
void watch_app_vm_init(watch_vm_observer_t observer);
const watch_app_state_t *watch_app_vm_state(void);
bool watch_app_vm_save(void);
void watch_app_vm_set_valid(bool valid);
void watch_app_vm_set_datetime(watch_datetime_t time);
void watch_app_vm_time_changed(watch_datetime_t time);
void watch_app_vm_set_rtc_source(bool crystal);

bool watch_settings_vm_twelve(void);
bool watch_settings_vm_light(void);
void watch_settings_vm_toggle_format(void);
void watch_settings_vm_toggle_theme(void);

typedef struct { uint16_t year; uint8_t month, selected, numbers[42]; bool today[42], previous_disabled, next_disabled; char title[32], selection[32]; } watch_calendar_state_t;
const watch_calendar_state_t *watch_calendar_vm_state(void);
void watch_calendar_vm_init(uint16_t year, uint8_t month);
void watch_calendar_vm_refresh(void);
void watch_calendar_vm_move(bool next);
void watch_calendar_vm_select(unsigned slot);

typedef struct { uint8_t hour, minute; bool daily; char time[8], repeat[32], toggle[8]; bool enabled; } watch_alarm_row_t;
typedef struct { unsigned index; uint8_t hour, minute; bool daily; char repeat[32]; } watch_alarm_editor_t;
watch_alarm_row_t watch_alarm_vm_row(unsigned index);
const watch_alarm_editor_t *watch_alarm_vm_editor(void);
void watch_alarm_vm_open(unsigned index);
void watch_alarm_vm_repeat(void);
bool watch_alarm_vm_toggle(unsigned index);
bool watch_alarm_vm_save(uint8_t hour, uint8_t minute);

typedef struct { char expression[48], text[48]; bool result, small; } watch_calculator_state_t;
const watch_calculator_state_t *watch_calculator_vm_state(void);
void watch_calculator_vm_init(void);
void watch_calculator_vm_key(const char *key);

typedef struct { unsigned index, mode; bool uppercase, confirm_delete; char draft[WATCH_NOTE_SIZE], hint[64]; } watch_notes_state_t;
const watch_notes_state_t *watch_notes_vm_state(void);
void watch_notes_vm_open(unsigned index);
void watch_notes_vm_change(const char *text);
void watch_notes_vm_next_mode(void);
void watch_notes_vm_case(void);
char watch_notes_vm_character(unsigned index);
void watch_notes_vm_preview(unsigned index, char *text, unsigned size);
bool watch_notes_vm_save(void);
bool watch_notes_vm_delete(void);

typedef struct { unsigned progress; char total[24], remaining[48], goal[40], reminder[40], summary[48]; } watch_water_state_t;
watch_water_state_t watch_water_vm_state(void);
void watch_water_vm_history(unsigned index, char *text, unsigned size);
bool watch_water_vm_add(int glasses);
bool watch_water_vm_goal(int delta);
void watch_water_vm_reminder(void);

typedef struct { int selected; char title[24], text[96], first[24], second[24]; } watch_notification_state_t;
watch_notification_state_t watch_notification_vm_state(void);
void watch_notification_vm_action(bool first);
bool watch_notification_vm_alarm_active(void);

typedef struct { watch_datetime_t draft; unsigned days; char hint[128], result[64]; } watch_clock_editor_t;
const watch_clock_editor_t *watch_clock_vm_state(void);
void watch_clock_vm_open(void);
void watch_clock_vm_date(uint16_t year, uint8_t month, uint8_t day);
bool watch_clock_vm_save(watch_datetime_t time);

typedef struct { watch_datetime_t datetime; unsigned draft_style; bool save_ok, twelve, clock_valid, battery_valid; char battery[24], time[16], period[4], date[40], seconds[4], orbit_seconds[12], alarm[48]; float hour_angle, minute_angle, second_angle; } watch_faces_state_t;
const watch_faces_state_t *watch_faces_vm_state(void);
void watch_faces_vm_refresh(watch_datetime_t time, bool twelve, bool valid);
void watch_faces_vm_set_battery(uint8_t percent, bool valid);
void watch_faces_vm_open(void);
void watch_faces_vm_choose(unsigned style);
bool watch_faces_vm_apply(void);
unsigned watch_faces_vm_style(void);
#endif
