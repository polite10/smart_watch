#include "viewmodels/watch_arcade_vm.h"
#include "services/watch_records_service.h"
static watch_snake_state_t snake;
static watch_flappy_state_t flappy;
static uint32_t generation;
static watch_snake_control_t control;
static watch_tilt_t tilt;
const watch_snake_state_t *watch_snake_vm_state(void) { return &snake; }
const watch_flappy_state_t *watch_flappy_vm_state(void) { return &flappy; }
void watch_snake_vm_new(uint32_t seed)
{
    watch_snake_new(&snake,seed^(++generation*0x9E3779B9u));
    watch_tilt_reset(&tilt);
}
void watch_snake_vm_start(void) { if(snake.phase==WATCH_GAME_READY) snake.phase=WATCH_GAME_RUNNING; }
bool watch_snake_vm_turn(int dx,int dy) { return watch_snake_turn(&snake,dx,dy); }
void watch_snake_vm_step(void) { watch_snake_step(&snake); watch_records_service_update(WATCH_RECORD_SNAKE,snake.score); }
void watch_snake_vm_pause(void) { watch_snake_pause(&snake); }
void watch_flappy_vm_new(uint32_t seed) { watch_flappy_new(&flappy,seed^(++generation*0x9E3779B9u)); }
void watch_flappy_vm_flap(void) { watch_flappy_flap(&flappy); }
void watch_flappy_vm_step(void) { watch_flappy_step(&flappy); watch_records_service_update(WATCH_RECORD_FLAPPY,flappy.score); }
void watch_flappy_vm_pause(void) { watch_flappy_pause(&flappy); }

watch_snake_control_t watch_snake_vm_control(void) { return control; }
void watch_snake_vm_choose_control(watch_snake_control_t chosen)
{
    control=chosen;watch_tilt_reset(&tilt);
}
bool watch_snake_vm_calibrated(void) { return tilt.calibrated; }
void watch_snake_vm_recalibrate(void) { watch_tilt_reset(&tilt); }
void watch_snake_vm_tilt(int x,int y,int z)
{
    if(control!=WATCH_SNAKE_CONTROL_TILT || snake.phase==WATCH_GAME_PAUSED ||
       snake.phase==WATCH_GAME_OVER || snake.phase==WATCH_GAME_WON) return;
    int dx,dy;
    bool turning=watch_tilt_sample(&tilt,x,y,z,&dx,&dy);
    if(tilt.calibrated) watch_snake_vm_start();
    if(turning) watch_snake_turn(&snake,dx,dy);
}
