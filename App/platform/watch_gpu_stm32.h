#ifndef WATCH_GPU_STM32_H
#define WATCH_GPU_STM32_H
#include "stm32u5xx_hal.h"
extern GPU2D_HandleTypeDef hgpu2d;
extern volatile uint32_t smartwatch_gpu_completions,smartwatch_gpu_timeouts;
void watch_gpu_init(void);
#endif
