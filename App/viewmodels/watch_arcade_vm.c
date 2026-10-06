#include "viewmodels/watch_arcade_vm.h"
static watch_snake_state_t snake;
static watch_flappy_state_t flappy;
static uint32_t generation;
const watch_snake_state_t *watch_snake_vm_state(void) { return &snake; }
const watch_flappy_state_t *watch_flappy_vm_state(void) { return &flappy; }
void watch_snake_vm_new(uint32_t seed) { watch_snake_new(&snake,seed^(++generation*0x9E3779B9u)); }
void watch_snake_vm_start(void) { if(snake.phase==WATCH_GAME_READY) snake.phase=WATCH_GAME_RUNNING; }
bool watch_snake_vm_turn(int dx,int dy) { return watch_snake_turn(&snake,dx,dy); }
void watch_snake_vm_step(void) { watch_snake_step(&snake); }
void watch_snake_vm_pause(void) { watch_snake_pause(&snake); }
void watch_flappy_vm_new(uint32_t seed) { watch_flappy_new(&flappy,seed^(++generation*0x9E3779B9u)); }
void watch_flappy_vm_flap(void) { watch_flappy_flap(&flappy); }
void watch_flappy_vm_step(void) { watch_flappy_step(&flappy); }
void watch_flappy_vm_pause(void) { watch_flappy_pause(&flappy); }
