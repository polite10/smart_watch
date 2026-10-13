#ifndef WATCH_FLAPPY_SCENERY_H
#define WATCH_FLAPPY_SCENERY_H
#include <stdint.h>
enum { WATCH_SCENERY_COUNT=8, WATCH_SCENERY_BYTES=178992 };
extern const uint8_t watch_flappy_scenery[];
extern const uint32_t watch_flappy_scenery_size;
extern const uint16_t watch_scenery_widths[WATCH_SCENERY_COUNT],watch_scenery_heights[WATCH_SCENERY_COUNT];
#endif
