#ifndef WATCH_CLOCK_SERVICE_H
#define WATCH_CLOCK_SERVICE_H
#include "models/watch_datetime.h"
typedef void (*watch_clock_observer_t)(bool valid, const watch_datetime_t *time);
void watch_clock_service_subscribe(watch_clock_observer_t observer);
bool watch_clock_set_datetime(uint16_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second);
bool watch_clock_is_valid(void);
/* Platform ports: do not call View or ViewModel functions here. */
void watch_platform_clock_init(void);
bool watch_platform_clock_set(watch_datetime_t time);
bool watch_platform_clock_read(watch_datetime_t *time);
bool watch_platform_clock_valid(void);
#endif
