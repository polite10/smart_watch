#ifndef WATCH_FACES_H
#define WATCH_FACES_H
#include "lvgl.h"
#include <stdbool.h>
void watch_faces_init(lv_obj_t *home, lv_obj_t *picker, lv_event_cb_t navigate, void *menu);
void watch_faces_open(void);
void watch_faces_apply_saved(void);
void watch_faces_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second, bool twelve_hour);
void watch_faces_set_battery(uint8_t percent, bool valid);
void watch_faces_refresh(void);
#endif
