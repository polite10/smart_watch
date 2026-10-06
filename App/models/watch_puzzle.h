#ifndef WATCH_PUZZLE_H
#define WATCH_PUZZLE_H
#include <stdbool.h>
#include <stdint.h>

enum { WATCH_PUZZLE_SIDE = 4, WATCH_PUZZLE_CELLS = 16 };
typedef struct {
    uint8_t tiles[WATCH_PUZZLE_CELLS]; /* Row-major; zero is the empty cell. */
    uint8_t blank;
    bool won;
    uint32_t moves;
} watch_puzzle_state_t;

void watch_puzzle_new(watch_puzzle_state_t *state, uint32_t seed);
bool watch_puzzle_move(watch_puzzle_state_t *state, unsigned slot);
#endif
