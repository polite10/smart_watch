#ifndef WATCH_ARCADE_H
#define WATCH_ARCADE_H
#include <stdbool.h>
#include <stdint.h>

enum { WATCH_SNAKE_SIDE=18, WATCH_SNAKE_CELLS=324, WATCH_FLAPPY_PIPES=3 };
typedef enum { WATCH_GAME_READY, WATCH_GAME_RUNNING, WATCH_GAME_PAUSED, WATCH_GAME_OVER, WATCH_GAME_WON } watch_game_phase_t;
typedef enum { WATCH_RIGHT, WATCH_DOWN, WATCH_LEFT, WATCH_UP } watch_direction_t;
typedef struct { uint8_t x,y; } watch_cell_t;
typedef struct {
    watch_cell_t body[WATCH_SNAKE_CELLS], food;
    uint16_t length,score;
    watch_direction_t direction,queued;
    bool turn_queued;
    watch_game_phase_t phase;
    uint32_t random;
} watch_snake_state_t;
void watch_snake_new(watch_snake_state_t *s,uint32_t seed);
bool watch_snake_turn(watch_snake_state_t *s,int dx,int dy);
void watch_snake_step(watch_snake_state_t *s);
void watch_snake_pause(watch_snake_state_t *s);

typedef struct { float x,gap_y; bool scored; } watch_pipe_t;
typedef struct {
    float y,velocity;
    watch_pipe_t pipes[WATCH_FLAPPY_PIPES];
    uint16_t score;
    watch_game_phase_t phase;
    uint32_t random;
} watch_flappy_state_t;
void watch_flappy_new(watch_flappy_state_t *s,uint32_t seed);
void watch_flappy_flap(watch_flappy_state_t *s);
void watch_flappy_step(watch_flappy_state_t *s);
void watch_flappy_pause(watch_flappy_state_t *s);
#endif
