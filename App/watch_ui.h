#ifndef WATCH_UI_H
#define WATCH_UI_H
#include <stdint.h>
#include <stdbool.h>
/* Call after lv_init(), display and touch registration, on the UI task. */
void watch_ui_init(void);
/* Feed from RTC on the same task as lv_timer_handler(). */
void watch_ui_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second);
/* valid=false renders unavailable rather than a fabricated measurement. */
void watch_ui_set_battery(uint8_t percent, bool valid);
void watch_ui_set_activity(uint16_t move, uint16_t exercise, uint16_t stand);
#endif
