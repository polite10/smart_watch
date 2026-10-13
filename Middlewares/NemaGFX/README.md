# NemaGFX for STM32U5A9 NeoChrom

Unmodified headers and the Cortex-M33 revC hard-float static library are vendored
from `lvgl/lvgl`, tag `v9.3.0`, directory `libs/nema_gfx`:
https://github.com/lvgl/lvgl/tree/v9.3.0/libs/nema_gfx

The library's Git blob SHA-1 is `79c37a7e91f9236feac60553c055495011bd9fa1`.
Think Silicon copyright and permission notices are retained in every header.
LVGL's STM32 HAL adapter provides the memory pool and GPU command completion;
`App/platform/watch_gpu_stm32.c` initializes the hardware and interrupt.
DMA2D retains the LCD copy/presentation path. CPU D-cache is not enabled.
