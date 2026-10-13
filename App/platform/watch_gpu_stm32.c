/* NeoChrom draws LVGL tasks; DMA2D flushes them into the hidden LCD buffer. */
#include "platform/watch_gpu_stm32.h"
#include "smartwatch.h"
GPU2D_HandleTypeDef hgpu2d;
volatile uint32_t smartwatch_gpu_completions,smartwatch_gpu_timeouts;

void watch_gpu_init(void)
{
    __HAL_RCC_GPU2D_CLK_ENABLE();
    __HAL_RCC_GPU2D_FORCE_RESET();
    __HAL_RCC_GPU2D_RELEASE_RESET();
    hgpu2d.Instance=GPU2D;
    if(HAL_GPU2D_Init(&hgpu2d)!=HAL_OK) Error_Handler();
    HAL_NVIC_SetPriority(GPU2D_IRQn,5,0);
    HAL_NVIC_EnableIRQ(GPU2D_IRQn);
}

void GPU2D_IRQHandler(void)
{
    HAL_GPU2D_IRQHandler(&hgpu2d);
}
