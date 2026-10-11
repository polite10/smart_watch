#ifndef LED_BUTTON_DEMO_H
#define LED_BUTTON_DEMO_H

#include "stm32u5xx_hal.h"

/* Dedicated peripherals; the IRQ handlers forward to these HAL handles. */
extern TIM_HandleTypeDef led_demo_timer;
extern UART_HandleTypeDef led_demo_uart;
extern DMA_HandleTypeDef led_demo_uart_tx_dma;

void led_button_demo_init(void);
void led_button_demo_poll(void);

#endif
