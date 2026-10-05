#include "models/watch_model.h"
#include <string.h>

watch_data_t watch_data;
static uint32_t now_seconds, current_date, next_water;
static uint32_t snooze_until[WATCH_ALARMS];
static uint8_t pending_alarms;
static bool water_due;

void watch_model_reset(const watch_data_t *saved)
{
    memset(&watch_data, 0, sizeof watch_data);
    if(saved) watch_data = *saved;
    if(!saved || watch_data.version != 1) {
        memset(&watch_data, 0, sizeof watch_data);
        watch_data.version = 1;
        watch_data.water_goal = 2000;
        for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
            watch_data.alarms[i].hour = 7 + i;
            watch_data.alarms[i].daily = 1;
        }
    }
    if(watch_data.water_goal < 200 || watch_data.water_goal > 6000) watch_data.water_goal = 2000;
    if(watch_data.face_style >= WATCH_FACE_COUNT) watch_data.face_style = 0;
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

static void reset_reminder(void)
{
    water_due = false;
    next_water = watch_data.reminder_minutes ? now_seconds + watch_data.reminder_minutes * 60U : 0;
}
void watch_model_time_changed(uint32_t day, uint32_t now, uint8_t hour, uint8_t minute)
{
    /* Do not retain deadlines or ring immediately when the user sets a clock. */
    now_seconds = now;
    current_date = 0;
    pending_alarms = 0;
    memset(snooze_until, 0, sizeof snooze_until);
    reset_reminder();
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        watch_alarm_t *a = &watch_data.alarms[i];
        if(a->hour == hour && a->minute == minute) a->fired_day = day;
    }
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
            unsigned index=1;
            while(index<WATCH_HISTORY && watch_data.water[index].day!=day) ++index;
            /* Correcting the date back to a recorded day must retain its water. */
            watch_water_day_t record = index<WATCH_HISTORY ? watch_data.water[index] :
                (watch_water_day_t){day, 0, watch_data.water_goal};
            unsigned moved=index<WATCH_HISTORY ? index : WATCH_HISTORY-1;
            memmove(&watch_data.water[1], &watch_data.water[0], moved * sizeof(watch_water_day_t));
            watch_data.water[0] = record;
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

const watch_data_t *watch_model_data(void) { return &watch_data; }
bool watch_model_alarm_toggle(unsigned index)
{
    if(index >= WATCH_ALARMS) return false;
    watch_data.alarms[index].enabled = !watch_data.alarms[index].enabled;
    watch_alarm_changed(index); return true;
}
bool watch_model_alarm_set(unsigned index, uint8_t hour, uint8_t minute, bool daily)
{
    if(index >= WATCH_ALARMS || hour > 23 || minute > 59) return false;
    watch_alarm_t *a = &watch_data.alarms[index];
    a->hour = hour; a->minute = minute; a->daily = daily; a->enabled = 1;
    watch_alarm_changed(index); return true;
}
bool watch_model_note_set(unsigned index, const char *text)
{
    if(index >= WATCH_NOTES || !text || strlen(text) >= WATCH_NOTE_SIZE) return false;
    memcpy(watch_data.notes[index], text, strlen(text) + 1); return true;
}
void watch_model_set_format(bool twelve) { watch_data.twelve_hour = twelve; }
void watch_model_set_theme(bool light) { watch_data.light_theme = light; }
bool watch_model_set_face(unsigned style)
{
    if(style >= WATCH_FACE_COUNT) return false;
    watch_data.face_style = style; return true;
}
_Static_assert(sizeof(watch_data_t) == 916, "Keep the version-1 user-data layout");
