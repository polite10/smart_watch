#ifndef SMARTWATCH_H
#define SMARTWATCH_H
#include "stm32u5xx_hal.h"
void Error_Handler(void);
/* Debug status: 1 boot, 2 LCD, 3 touch, 4 UI running; high bit indicates error. */
extern volatile uint32_t smartwatch_status;
extern volatile uint32_t smartwatch_heartbeat;
extern volatile uint32_t smartwatch_rtc_lse;
#endif
