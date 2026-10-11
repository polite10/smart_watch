#include "models/watch_records.h"
static watch_records_t records;
void watch_records_reset(const watch_records_t *saved)
{
    records = saved && saved->version == 1 ? *saved : (watch_records_t){.version=1};
    if(records.snake_score > 320) records.snake_score=0;
}
const watch_records_t *watch_records_state(void) { return &records; }
bool watch_records_update(watch_record_game_t game, uint32_t result)
{
    if(!result) return false;
    if(game==WATCH_RECORD_PUZZLE) {
        if(records.puzzle_moves && result>=records.puzzle_moves) return false;
        records.puzzle_moves=result;
    } else if(game==WATCH_RECORD_SNAKE) {
        if(result>320 || result<=records.snake_score) return false;
        records.snake_score=(uint16_t)result;
    } else if(game==WATCH_RECORD_FLAPPY) {
        if(result>UINT16_MAX || result<=records.flappy_score) return false;
        records.flappy_score=(uint16_t)result;
    } else return false;
    return true;
}
_Static_assert(sizeof(watch_records_t)==12,"Journal record extension size");
