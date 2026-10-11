#include "smartwatch.h"
#include "stm32u5x9j_discovery_lcd.h"
#include "stm32u5x9j_discovery_ts.h"
#include "watch_ui.h"
#include <string.h>
#include "services/watch_display_service.h"
#include "platform/watch_dirty_rows.h"
static uint32_t draw_buffer[480 * 40] __attribute__((aligned(16)));
static uint32_t back_framebuffer[184320] __attribute__((aligned(16)));
static unsigned front_buffer;
static volatile bool frame_presented;
volatile uint32_t smartwatch_frame_count, smartwatch_frame_ms, smartwatch_frame_max_ms;
static uint32_t frame_started;
static bool drawing_frame;
static DMA2D_HandleTypeDef display_dma;
static watch_dirty_rows_t dirty_rows;
static lv_display_t *pending_display;
static uint32_t present_started;
volatile uint32_t smartwatch_sync_bytes,smartwatch_frame_total_ms;
static uint32_t render_started;

static void display_copy(uint32_t source, uint32_t destination, uint32_t width,
                         uint32_t height, uint32_t input_offset, uint32_t output_offset)
{
    /* Blocking transfer: LVGL may reuse the small draw buffer immediately after flush. */
    display_dma.Instance = DMA2D;
    display_dma.Init.Mode = DMA2D_M2M;
    display_dma.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
    display_dma.Init.OutputOffset = output_offset;
    display_dma.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
    display_dma.LayerCfg[1].InputOffset=input_offset;
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
    if(!drawing_frame) { frame_started=HAL_GetTick();drawing_frame=true;watch_dirty_reset(&dirty_rows); }
    watch_dirty_add(&dirty_rows,area->x1,area->y1,area->x2,area->y2);
    uint32_t width = (uint32_t)(area->x2-area->x1+1);
    uint32_t destination = front_buffer ? GFXMMU_VIRTUAL_BUFFER0_BASE : GFXMMU_VIRTUAL_BUFFER1_BASE;
    /* Render into the hidden buffer; GFXMMU has a 768-pixel virtual stride. */
    display_copy((uint32_t)pixels, destination + 4U * (area->y1 * 768U + area->x1),
                 width, area->y2-area->y1+1, 0, 768U-width);
    if(lv_display_flush_is_last(display)) {
        frame_presented = false;
        if(HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, destination, 0) != HAL_OK ||
           HAL_LTDC_Reload(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK) Error_Handler();
        present_started=HAL_GetTick();
        pending_display=display;
        /* The last partial buffer stays owned by the driver until presentation.
         * The main loop polls completion; touch and game timers keep running. */
        lv_timer_pause(lv_display_get_refr_timer(display));
        return;
    }
    lv_display_flush_ready(display);
}

void watch_platform_display_poll(void)
{
    if(!pending_display) return;
    if(!frame_presented) {
        if(HAL_GetTick()-present_started>100) Error_Handler();
        return;
    }
    front_buffer^=1U;
    uint32_t front=front_buffer ? GFXMMU_VIRTUAL_BUFFER1_BASE : GFXMMU_VIRTUAL_BUFFER0_BASE;
    uint32_t back=front_buffer ? GFXMMU_VIRTUAL_BUFFER0_BASE : GFXMMU_VIRTUAL_BUFFER1_BASE;
    smartwatch_sync_bytes=0;
    if(dirty_rows.pixels>=184320) {
        uint32_t physical_front=front_buffer ? hlcd_gfxmmu.Init.Buffers.Buf1Address : hlcd_gfxmmu.Init.Buffers.Buf0Address;
        uint32_t physical_back=front_buffer ? hlcd_gfxmmu.Init.Buffers.Buf0Address : hlcd_gfxmmu.Init.Buffers.Buf1Address;
        display_copy(physical_front,physical_back,512,360,0,0);
        smartwatch_sync_bytes=sizeof back_framebuffer;
    } else for(unsigned y=0;y<480;) {
        if(dirty_rows.right[y]<dirty_rows.left[y]) { ++y;continue; }
        unsigned last=y;
        while(last+1<480 && dirty_rows.left[last+1]==dirty_rows.left[y] && dirty_rows.right[last+1]==dirty_rows.right[y]) ++last;
        unsigned width=dirty_rows.right[y]-dirty_rows.left[y]+1,height=last-y+1;
        uint32_t offset=4U*(y*768U+dirty_rows.left[y]);
        display_copy(front+offset,back+offset,width,height,768U-width,768U-width);
        smartwatch_sync_bytes+=width*height*4U;
        y=last+1;
    }
    smartwatch_frame_ms=HAL_GetTick()-frame_started;
    smartwatch_frame_total_ms=HAL_GetTick()-render_started;
    if(smartwatch_frame_ms>smartwatch_frame_max_ms) smartwatch_frame_max_ms=smartwatch_frame_ms;
    ++smartwatch_frame_count;drawing_frame=false;
    lv_display_t *display=pending_display;pending_display=NULL;
    lv_display_flush_ready(display);
    lv_timer_resume(lv_display_get_refr_timer(display));
}
static void display_refresh(lv_timer_t *timer)
{
    /* Invalidation can resume the refresh timer while waiting for vblank.
     * Gate that callback so it cannot reuse the hidden framebuffer early. */
    if(pending_display) { lv_timer_pause(timer);return; }
    render_started=HAL_GetTick();
    lv_display_refr_timer(timer);
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
    lv_timer_set_cb(lv_display_get_refr_timer(display),display_refresh);
    lv_indev_t *input=lv_indev_create();
    lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,read_touch);
    lv_indev_set_long_press_time(input, 700);
    lv_timer_set_period(lv_indev_get_read_timer(input), 16);
}
