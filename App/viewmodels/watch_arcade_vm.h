#ifndef WATCH_ARCADE_VM_H
#define WATCH_ARCADE_VM_H
#include "models/watch_arcade.h"
#include "models/watch_tilt.h"
typedef enum { WATCH_SNAKE_CONTROL_CHOOSE, WATCH_SNAKE_CONTROL_TOUCH, WATCH_SNAKE_CONTROL_TILT } watch_snake_control_t;
watch_snake_control_t watch_snake_vm_control(void);
void watch_snake_vm_choose_control(watch_snake_control_t control);
bool watch_snake_vm_calibrated(void);
void watch_snake_vm_tilt(int x,int y,int z);
void watch_snake_vm_recalibrate(void);
const watch_snake_state_t *watch_snake_vm_state(void);
const watch_flappy_state_t *watch_flappy_vm_state(void);
void watch_snake_vm_new(uint32_t seed);
void watch_snake_vm_start(void);
bool watch_snake_vm_turn(int dx,int dy);
void watch_snake_vm_step(void);
void watch_snake_vm_pause(void);
void watch_flappy_vm_new(uint32_t seed);
void watch_flappy_vm_flap(void);
void watch_flappy_vm_step(void);
void watch_flappy_vm_pause(void);
#endif
