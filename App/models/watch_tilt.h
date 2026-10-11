#ifndef WATCH_TILT_H
#define WATCH_TILT_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    int32_t origin_x,origin_y,filtered_x,filtered_y,sum_x,sum_y;
    unsigned samples;
    bool calibrated;
} watch_tilt_t;
void watch_tilt_reset(watch_tilt_t *tilt);
/* Stationary neutral calibration, low-pass filtering, dead zone and axis bias.
 * Outputs a direction vector in screen coordinates (positive y is down). */
bool watch_tilt_sample(watch_tilt_t *tilt,int x,int y,int z,int *dx,int *dy);
#endif
