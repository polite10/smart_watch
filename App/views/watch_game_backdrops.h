#ifndef WATCH_GAME_BACKDROPS_H
#define WATCH_GAME_BACKDROPS_H
#include <stdint.h>
enum { WATCH_GAME_BACKDROP_SIZE=320, WATCH_GAME_BACKDROP_BYTES=204800 };
extern const uint8_t *const watch_game_backdrops[2];
extern const uint32_t watch_game_backdrop_sizes[2];
#endif
