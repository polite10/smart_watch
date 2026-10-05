#ifndef WATCH_DISPLAY_SERVICE_H
#define WATCH_DISPLAY_SERVICE_H
#include <stdbool.h>
void watch_platform_display_init(void);
/* Also retained as the emulator's panel-power port. */
void watch_ui_display_power(bool on);
#endif
