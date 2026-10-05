#ifndef WATCH_MODEL_H
#define WATCH_MODEL_H
#include <stdbool.h>
#include <stdint.h>
#define WATCH_ALARMS 3
#define WATCH_NOTES 4
#define WATCH_NOTE_SIZE 192
#define WATCH_HISTORY 14
#define WATCH_GLASS_ML 200U
#define WATCH_FACE_COUNT 4
typedef struct { uint8_t hour, minute, enabled, daily; uint32_t fired_day; } watch_alarm_t;
typedef struct { uint32_t day; uint16_t ml, goal; } watch_water_day_t;
typedef struct {
    uint32_t version;
    watch_alarm_t alarms[WATCH_ALARMS];
    char notes[WATCH_NOTES][WATCH_NOTE_SIZE];
    watch_water_day_t water[WATCH_HISTORY]; /* index 0 is the active calendar day */
    uint16_t water_goal, reminder_minutes;
    uint8_t twelve_hour, light_theme;
    /* Reuse the old reserved bytes to retain the version-1 flash layout. */
    uint8_t face_style, face_color;
} watch_data_t;
extern watch_data_t watch_data;
void watch_model_init(void);
bool watch_model_save(void);
uint8_t watch_days_in_month(uint16_t year, uint8_t month);
bool watch_datetime_valid(uint16_t year, uint8_t month, uint8_t day,
                          uint8_t hour, uint8_t minute, uint8_t second);
void watch_model_time_changed(uint32_t day, uint32_t now, uint8_t hour, uint8_t minute);
uint32_t watch_day_key(uint16_t year, uint8_t month, uint8_t day);
uint32_t watch_epoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
bool watch_model_tick(uint32_t day, uint32_t now, uint8_t hour, uint8_t minute);
void watch_water_add(int glasses);
void watch_water_goal(int delta);
void watch_reminder_set(uint16_t minutes);
bool watch_water_reminder_due(void);
bool watch_alarm_active(void);
int watch_alarm_next(void);
void watch_alarm_ack(int index, bool snooze);
void watch_alarm_changed(int index);
bool watch_calculate(const char *expression, double *result);
/* Platform implements the persistent flash journal. */
bool watch_storage_load(watch_data_t *data);
bool watch_storage_save(const watch_data_t *data);
#endif
