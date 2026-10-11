#ifndef WATCH_RECORDS_H
#define WATCH_RECORDS_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t version, puzzle_moves;
    uint16_t snake_score, flappy_score;
} watch_records_t;
typedef enum { WATCH_RECORD_SNAKE, WATCH_RECORD_FLAPPY, WATCH_RECORD_PUZZLE } watch_record_game_t;
void watch_records_reset(const watch_records_t *saved);
const watch_records_t *watch_records_state(void);
bool watch_records_update(watch_record_game_t game, uint32_t result);
#endif
