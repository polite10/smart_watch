#include "watch_model.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

watch_data_t watch_data;
static uint32_t now_seconds, current_date, next_water;
static uint32_t snooze_until[WATCH_ALARMS];
static uint8_t pending_alarms;
static bool water_due;

void watch_model_init(void)
{
    memset(&watch_data, 0, sizeof watch_data);
    if(!watch_storage_load(&watch_data) || watch_data.version != 1) {
        memset(&watch_data, 0, sizeof watch_data);
        watch_data.version = 1;
        watch_data.water_goal = 2000;
        for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
            watch_data.alarms[i].hour = 7 + i;
            watch_data.alarms[i].daily = 1;
        }
    }
    if(watch_data.water_goal < 200 || watch_data.water_goal > 6000) watch_data.water_goal = 2000;
    if(watch_data.face_style >= 3) watch_data.face_style = 0;
    if(watch_data.face_color >= 3) watch_data.face_color = 0;
    if(watch_data.reminder_minutes != 0 && watch_data.reminder_minutes != 30 &&
       watch_data.reminder_minutes != 60 && watch_data.reminder_minutes != 120) watch_data.reminder_minutes = 0;
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        if(watch_data.alarms[i].hour > 23 || watch_data.alarms[i].minute > 59)
            memset(&watch_data.alarms[i], 0, sizeof(watch_alarm_t));
    }
    for(unsigned i = 0; i < WATCH_NOTES; ++i) watch_data.notes[i][WATCH_NOTE_SIZE - 1] = '\0';
    memset(snooze_until, 0, sizeof snooze_until);
    current_date = now_seconds = next_water = pending_alarms = 0;
    water_due = false;
}
bool watch_model_save(void) { return watch_storage_save(&watch_data); }
uint32_t watch_day_key(uint16_t year, uint8_t month, uint8_t day)
{ return (uint32_t)year * 10000 + (uint32_t)month * 100 + day; }
uint32_t watch_epoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    static const uint16_t before[] = {0,31,59,90,120,151,181,212,243,273,304,334};
    uint32_t days = 0;
    for(unsigned y = 2000; y < year; ++y) days += 365 + (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    days += before[month - 1] + day - 1;
    if(month > 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ++days;
    return days * 86400 + (uint32_t)hour * 3600 + (uint32_t)minute * 60 + second;
}
static void reset_reminder(void)
{
    water_due = false;
    next_water = watch_data.reminder_minutes ? now_seconds + watch_data.reminder_minutes * 60U : 0;
}
bool watch_model_tick(uint32_t day, uint32_t now, uint8_t hour, uint8_t minute)
{
    bool changed = false;
    bool clock_back = now_seconds && now < now_seconds;
    now_seconds = now;
    if(current_date != day) {
        current_date = day;
        /* Retain only recorded days; a gap does not fabricate consumption. */
        if(watch_data.water[0].day != day) {
            memmove(&watch_data.water[1], &watch_data.water[0], (WATCH_HISTORY - 1) * sizeof(watch_water_day_t));
            watch_data.water[0] = (watch_water_day_t){day, 0, watch_data.water_goal};
            changed = true;
        }
        reset_reminder();
    }
    if(clock_back) reset_reminder();
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        watch_alarm_t *a = &watch_data.alarms[i];
        if(a->enabled && a->hour == hour && a->minute == minute && a->fired_day != day) {
            pending_alarms |= 1U << i;
            a->fired_day = day;
            if(!a->daily) a->enabled = 0;
            changed = true;
        }
        if(snooze_until[i] && now >= snooze_until[i]) {
            snooze_until[i] = 0;
            pending_alarms |= 1U << i;
        }
    }
    /* Quiet hours are 22:00-08:00; reaching the goal suppresses reminders. */
    if(next_water && now >= next_water) {
        if(hour >= 8 && hour < 22 && watch_data.water[0].ml < watch_data.water_goal) water_due = true;
        next_water = now + watch_data.reminder_minutes * 60U;
    }
    return changed;
}
void watch_water_add(int glasses)
{
    int ml = watch_data.water[0].ml + glasses * (int)WATCH_GLASS_ML;
    watch_data.water[0].ml = ml < 0 ? 0 : ml > 20000 ? 20000 : ml;
    reset_reminder();
}
void watch_water_goal(int delta)
{
    int goal = watch_data.water_goal + delta;
    watch_data.water_goal = goal < 200 ? 200 : goal > 6000 ? 6000 : goal;
    watch_data.water[0].goal = watch_data.water_goal;
    if(watch_data.water[0].ml >= watch_data.water_goal) water_due = false;
}
void watch_reminder_set(uint16_t minutes) { watch_data.reminder_minutes = minutes; reset_reminder(); }
bool watch_water_reminder_due(void) { return water_due; }
bool watch_alarm_active(void) { return pending_alarms != 0; }
int watch_alarm_next(void)
{
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) if(pending_alarms & (1U << i)) return i;
    return -1;
}
void watch_alarm_ack(int index, bool snooze)
{
    if(index < 0 || index >= WATCH_ALARMS) return;
    pending_alarms &= ~(1U << index);
    snooze_until[index] = snooze ? now_seconds + 300 : 0;
}
void watch_alarm_changed(int index)
{
    if(index < 0 || index >= WATCH_ALARMS) return;
    snooze_until[index] = 0;
    pending_alarms &= ~(1U << index);
    /* An edited/enabled alarm matching this minute starts on the next day. */
    watch_alarm_t *a = &watch_data.alarms[index];
    uint32_t minute_of_day = (now_seconds % 86400) / 60;
    a->fired_day = minute_of_day == a->hour * 60U + a->minute ? current_date : 0;
}
static double number(const char **p, bool *valid)
{
    char *end;
    double value = strtod(*p, &end);
    if(end == *p || !isfinite(value)) { *valid = false; return 0; }
    *p = end;
    return value;
}
static double term(const char **p, bool *valid)
{
    double v = number(p, valid);
    while(*valid && (**p == '*' || **p == '/')) {
        char op = *(*p)++;
        double rhs = number(p, valid);
        if(op == '/' && rhs == 0) { *valid = false; return 0; }
        v = op == '*' ? v * rhs : v / rhs;
    }
    return v;
}
bool watch_calculate(const char *expression, double *result)
{
    /* Keypad inputs are decimal numbers only, not C's hex/NaN notation. */
    for(const char *s = expression; *s; ++s)
        if(!isdigit((unsigned char)*s) && !strchr(".+-*/", *s)) return false;
    bool valid = true;
    const char *p = expression;
    double value = term(&p, &valid);
    while(valid && (*p == '+' || *p == '-')) {
        char op = *p++;
        double rhs = term(&p, &valid);
        value = op == '+' ? value + rhs : value - rhs;
    }
    if(!valid || *p || !isfinite(value) || fabs(value) > 1e12) return false;
    *result = value;
    return true;
}
