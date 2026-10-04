#ifndef WATCH_CLOCK_H
#define WATCH_CLOCK_H
#include <stdbool.h>
#include <stdint.h>
/* Hardware port. Call on the same task as the UI, after watch_ui_init(). */
bool watch_clock_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second);
bool watch_clock_is_valid(void);
#endif
