#ifndef WATCH_UI_H
#define WATCH_UI_H
#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"
/* Call after lv_init(), display and touch registration, on the UI task. */
void watch_ui_init(void);
/* Feed from RTC on the same task as lv_timer_handler(). */
void watch_ui_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second);
/* valid=false renders unavailable rather than a fabricated measurement. */
void watch_ui_set_battery(uint8_t percent, bool valid);
void watch_ui_set_rtc_source(bool crystal);
void watch_ui_set_clock_valid(bool valid);
void watch_ui_datetime_changed(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hour, uint8_t minute, uint8_t second);
/* Raw gesture filter, including single/double-tap arbitration before clicks. */
void watch_ui_touch(lv_indev_data_t *data, uint32_t now);
/* Board implementation switches the panel while keeping touch/RTC alive. */
void watch_ui_display_power(bool on);
void watch_ui_header(lv_obj_t *page, const char *title, lv_event_cb_t back, void *data);
#endif
