#ifndef WATCH_IDA_VM_H
#define WATCH_IDA_VM_H
#include <stdbool.h>
#include <stdint.h>
#define WATCH_IDA_TRACE_POINTS 32
typedef enum { WATCH_IDA_READY, WATCH_IDA_RUNNING, WATCH_IDA_PAUSED, WATCH_IDA_ARRIVED } watch_ida_phase_t;
typedef struct { float x,y; } watch_ida_point_t;
/* Local simulation in metres, north is negative Y. Never represents sensor data. */
typedef struct {
    float x,y,target_x,target_y,heading_degrees,distance_m;
    uint32_t eta_seconds;
    uint16_t speed_tenths,range_m,sweep_degrees;
    uint8_t trace_count;
    bool has_target,eta_valid;
    watch_ida_phase_t phase;
    watch_ida_point_t trace[WATCH_IDA_TRACE_POINTS];
    char time[8],heading[12],speed[16],distance[16],range[16],eta[24],status[24];
    const char *mode;
} watch_ida_state_t;
const watch_ida_state_t *watch_ida_vm_state(void);
void watch_ida_vm_init(void);
void watch_ida_vm_reset(void);
void watch_ida_vm_clock(uint8_t hour,uint8_t minute,bool valid);
/* Radar-relative pixel coordinates: radius 152 maps to the selected range. */
bool watch_ida_vm_target(int dx,int dy);
void watch_ida_vm_clear_target(void);
void watch_ida_vm_speed(int delta_tenths);
void watch_ida_vm_range(void);
void watch_ida_vm_toggle(void);
void watch_ida_vm_pause(void);
void watch_ida_vm_step(uint32_t elapsed_ms);
#endif
