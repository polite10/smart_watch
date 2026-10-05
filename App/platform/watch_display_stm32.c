#include "smartwatch.h"
#include "stm32u5x9j_discovery_lcd.h"
#include "stm32u5x9j_discovery_ts.h"
#include "watch_ui.h"
#include <string.h>
static uint32_t draw_buffer[480 * 40] __attribute__((aligned(16)));
static uint32_t back_framebuffer[184320] __attribute__((aligned(16)));
static unsigned front_buffer;
static volatile bool frame_presented;
volatile uint32_t smartwatch_frame_count, smartwatch_frame_ms, smartwatch_frame_max_ms;
static uint32_t frame_started;
static bool drawing_frame;
static DMA2D_HandleTypeDef display_dma;

static void display_copy(uint32_t source, uint32_t destination, uint32_t width,
                         uint32_t height, uint32_t output_offset)
{
    /* Blocking transfer: LVGL may reuse the small draw buffer immediately after flush. */
    display_dma.Instance = DMA2D;
    display_dma.Init.Mode = DMA2D_M2M;
    display_dma.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
    display_dma.Init.OutputOffset = output_offset;
    display_dma.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
    display_dma.LayerCfg[1].InputAlpha = 255;
    __DSB();
    if(HAL_DMA2D_Init(&display_dma) != HAL_OK ||
       HAL_DMA2D_ConfigLayer(&display_dma, 1) != HAL_OK ||
       HAL_DMA2D_Start(&display_dma, source, destination, width, height) != HAL_OK ||
       HAL_DMA2D_PollForTransfer(&display_dma, 100) != HAL_OK) Error_Handler();
    __DSB();
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *ltdc)
{
    if(ltdc == &hlcd_ltdc) frame_presented = true;
}

static void display_buffers_init(void)
{
    __HAL_RCC_DMA2D_CLK_ENABLE();
    GFXMMU_BuffersTypeDef buffers = hlcd_gfxmmu.Init.Buffers;
    buffers.Buf1Address = (uint32_t)back_framebuffer;
    if(HAL_GFXMMU_ModifyBuffers(&hlcd_gfxmmu, &buffers) != HAL_OK) Error_Handler();
    hlcd_gfxmmu.Init.Buffers = buffers;
    memcpy(back_framebuffer, (void *)buffers.Buf0Address, sizeof back_framebuffer);
}

void watch_ui_display_power(bool on)
{
    /* Keep touch and RTC running so a tap can wake the display. */
    if(on) BSP_LCD_DisplayOn(0);
    else BSP_LCD_DisplayOff(0);
}

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    if(!drawing_frame) { frame_started = HAL_GetTick(); drawing_frame = true; }
    uint32_t width = (uint32_t)(area->x2-area->x1+1);
    uint32_t destination = front_buffer ? GFXMMU_VIRTUAL_BUFFER0_BASE : GFXMMU_VIRTUAL_BUFFER1_BASE;
    /* Render into the hidden buffer; GFXMMU has a 768-pixel virtual stride. */
    display_copy((uint32_t)pixels, destination + 4U * (area->y1 * 768U + area->x1),
                 width, area->y2-area->y1+1, 768U-width);
    if(lv_display_flush_is_last(display)) {
        frame_presented = false;
        if(HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, destination, 0) != HAL_OK ||
           HAL_LTDC_Reload(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK) Error_Handler();
        uint32_t wait_started = HAL_GetTick();
        while(!frame_presented) {
            if(HAL_GetTick() - wait_started > 100) Error_Handler();
        }
        front_buffer ^= 1U;
        /* Synchronize the hidden physical buffer for the next partial refresh. */
        void *front = (void *)(front_buffer ? hlcd_gfxmmu.Init.Buffers.Buf1Address : hlcd_gfxmmu.Init.Buffers.Buf0Address);
        void *back = (void *)(front_buffer ? hlcd_gfxmmu.Init.Buffers.Buf0Address : hlcd_gfxmmu.Init.Buffers.Buf1Address);
        display_copy((uint32_t)front, (uint32_t)back, 512, 360, 0);
        smartwatch_frame_ms = HAL_GetTick() - frame_started;
        if(smartwatch_frame_ms > smartwatch_frame_max_ms) smartwatch_frame_max_ms = smartwatch_frame_ms;
        ++smartwatch_frame_count;
        drawing_frame = false;
    }
    lv_display_flush_ready(display);
}

static void read_touch(lv_indev_t *input, lv_indev_data_t *data)
{
    (void)input;
    TS_State_t state={0};
    if(BSP_TS_GetState(0,&state)==BSP_ERROR_NONE && state.TouchDetected) {
        data->point.x=(int16_t)state.TouchX;
        data->point.y=(int16_t)state.TouchY;
        data->state=LV_INDEV_STATE_PRESSED;
    } else data->state=LV_INDEV_STATE_RELEASED;
    watch_ui_touch(data, HAL_GetTick());
}
void watch_platform_display_init(void)
{    if(BSP_LCD_Init(0,LCD_ORIENTATION_PORTRAIT)!=BSP_ERROR_NONE) Error_Handler();
    BSP_LCD_SetActiveLayer(0,0);
    BSP_LCD_FillRect(0,0,0,480,480,LCD_COLOR_BLACK);
    BSP_LCD_DisplayOn(0);
    display_buffers_init();
    smartwatch_status=2;
    TS_Init_t touch={.Width=480,.Height=480,.Orientation=TS_ORIENTATION_PORTRAIT,.Accuracy=2};
    if(BSP_TS_Init(0,&touch)!=BSP_ERROR_NONE) Error_Handler();
    smartwatch_status=3;
    lv_init();
    lv_tick_set_cb(HAL_GetTick);
    lv_display_t *display=lv_display_create(480,480);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    lv_indev_t *input=lv_indev_create();
    lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,read_touch);
    lv_indev_set_long_press_time(input, 700);
    lv_timer_set_period(lv_indev_get_read_timer(input), 16);
}
