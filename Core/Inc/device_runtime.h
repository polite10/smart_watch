#ifndef DEVICE_RUNTIME_H
#define DEVICE_RUNTIME_H
#include "stm32u5xx_hal.h"
typedef const char *(*device_command_handler_t)(const char *command);
void device_runtime_init(UART_HandleTypeDef *uart,const char *app,device_command_handler_t handler);
void device_runtime_rx_irq(void);
void device_runtime_poll(void);
#endif
