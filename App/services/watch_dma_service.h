#ifndef WATCH_DMA_SERVICE_H
#define WATCH_DMA_SERVICE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t completed,bytes,errors,message_bytes,toggle_ms;
    bool led_on,busy;
} watch_dma_status_t;
/* Read-only telemetry from the existing LED timer / UART TX DMA demo. */
watch_dma_status_t watch_dma_service_status(void);
#endif
