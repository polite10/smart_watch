#ifndef WATCH_MOTION_SERVICE_H
#define WATCH_MOTION_SERVICE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { int16_t x,y,z; } watch_acceleration_t;
/* Nonblocking data-ready read; returns false for absent or stale samples. */
bool watch_motion_start(void);
bool watch_motion_read(watch_acceleration_t *sample);
void watch_motion_stop(void);
#endif
