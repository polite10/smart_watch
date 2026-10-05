/* Clock setup adapted from ST STM32CubeU5 BSP example (ST license). */
#include "smartwatch.h"
#include "stm32u5x9j_discovery.h"
#include "stm32u5x9j_discovery_lcd.h"
#include "stm32u5x9j_discovery_ts.h"
#include "lvgl.h"
#include "watch_ui.h"
#include "build_time.h"
#include "watch_model.h"
#include "watch_clock.h"
#include <string.h>

volatile uint32_t smartwatch_status, smartwatch_heartbeat;
volatile uint32_t smartwatch_rtc_lse;
static RTC_HandleTypeDef rtc;
static bool clock_valid;
#define RTC_TIME_SET_MAGIC 0x54494D32U
static uint32_t draw_buffer[480 * 40] __attribute__((aligned(16)));
static uint32_t back_framebuffer[184320] __attribute__((aligned(16)));
static unsigned front_buffer;
static volatile bool frame_presented;
volatile uint32_t smartwatch_frame_count, smartwatch_frame_ms, smartwatch_frame_max_ms;
static uint32_t frame_started;
static bool drawing_frame;
static DMA2D_HandleTypeDef display_dma;
static void SystemClock_Config(void);

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
static void RTC_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    RCC_OscInitTypeDef osc={0};
    /* The board has a 32.768 kHz crystal. Use it for stable alarm timing. */
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);
    osc.OscillatorType=RCC_OSCILLATORTYPE_LSE;
    osc.LSEState=RCC_LSE_ON;
    osc.PLL.PLLState=RCC_PLL_NONE;
    smartwatch_rtc_lse = HAL_RCC_OscConfig(&osc)==HAL_OK;
    if(!smartwatch_rtc_lse) {
        osc.OscillatorType=RCC_OSCILLATORTYPE_LSI;
        osc.LSIState=RCC_LSI_ON;
        if(HAL_RCC_OscConfig(&osc)!=HAL_OK) Error_Handler();
    }
    RCC_PeriphCLKInitTypeDef periph={0};
    periph.PeriphClockSelection=RCC_PERIPHCLK_RTC;
    periph.RTCClockSelection=smartwatch_rtc_lse ? RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI;
    if(HAL_RCCEx_PeriphCLKConfig(&periph)!=HAL_OK) Error_Handler();
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    rtc.Instance=RTC;
    rtc.Init.HourFormat=RTC_HOURFORMAT_24;
    rtc.Init.AsynchPrediv=127;
    rtc.Init.SynchPrediv=smartwatch_rtc_lse ? 255 : 249;
    rtc.Init.OutPut=RTC_OUTPUT_DISABLE;
    rtc.Init.OutPutRemap=RTC_OUTPUT_REMAP_NONE;
    rtc.Init.OutPutPolarity=RTC_OUTPUT_POLARITY_HIGH;
    rtc.Init.OutPutType=RTC_OUTPUT_TYPE_OPENDRAIN;
    rtc.Init.OutPutPullUp=RTC_OUTPUT_PULLUP_NONE;
    if(HAL_RTC_Init(&rtc)!=HAL_OK) Error_Handler();
    if(HAL_RTCEx_BKUPRead(&rtc,RTC_BKP_DR0)!=0x53574D32) {
        RTC_TimeTypeDef t={0}; RTC_DateTypeDef d={0};
        /* Neutral calendar only; a build timestamp is never trusted wall time. */
        t.Hours=0; t.Minutes=0; t.Seconds=0;
        t.DayLightSaving=RTC_DAYLIGHTSAVING_NONE; t.StoreOperation=RTC_STOREOPERATION_RESET;
        d.Year=0; d.Month=1; d.Date=1; d.WeekDay=6;
        if(HAL_RTC_SetTime(&rtc,&t,RTC_FORMAT_BIN)!=HAL_OK ||
           HAL_RTC_SetDate(&rtc,&d,RTC_FORMAT_BIN)!=HAL_OK) Error_Handler();
        HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR0,0x53574D32);
        HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR1,0);
    }
    /* Old firmware's initialized marker does not imply an accurate clock. */
    clock_valid = HAL_RTCEx_BKUPRead(&rtc,RTC_BKP_DR1) == RTC_TIME_SET_MAGIC;
}
bool watch_clock_is_valid(void) { return clock_valid; }
bool watch_clock_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second)
{
    if(!watch_datetime_valid(year, month, day, hour, minute, second)) return false;
    RTC_TimeTypeDef t={0}; RTC_DateTypeDef d={0};
    t.Hours=hour; t.Minutes=minute; t.Seconds=second;
    t.DayLightSaving=RTC_DAYLIGHTSAVING_NONE; t.StoreOperation=RTC_STOREOPERATION_RESET;
    d.Year=year-2000; d.Month=month; d.Date=day;
    d.WeekDay=(watch_epoch(year,month,day,0,0,0)/86400U + 5U)%7U + 1U;
    HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR1,0);
    if(HAL_RTC_SetDate(&rtc,&d,RTC_FORMAT_BIN)!=HAL_OK ||
       HAL_RTC_SetTime(&rtc,&t,RTC_FORMAT_BIN)!=HAL_OK) {
        clock_valid=false;
        watch_ui_set_clock_valid(false);
        return false;
    }
    HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR1,RTC_TIME_SET_MAGIC);
    clock_valid=true;
    watch_ui_datetime_changed(year,month,day,hour,minute,second);
    return true;
}
/* Sync-Time.ps1 stops at this UI boundary even in an optimized build. */
static __attribute__((noinline)) void refresh_rtc(void)
{
    if(!clock_valid) return;
    RTC_TimeTypeDef t={0}; RTC_DateTypeDef d={0};
    HAL_StatusTypeDef ts=HAL_RTC_GetTime(&rtc,&t,RTC_FORMAT_BIN);
    HAL_StatusTypeDef ds=HAL_RTC_GetDate(&rtc,&d,RTC_FORMAT_BIN);
    if(ts!=HAL_OK || ds!=HAL_OK || !watch_datetime_valid(2000+d.Year,d.Month,d.Date,t.Hours,t.Minutes,t.Seconds)) {
        clock_valid=false;
        HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR1,0);
        watch_ui_set_clock_valid(false);
        return;
    }
    watch_ui_set_datetime(2000+d.Year,d.Month,d.Date,t.Hours,t.Minutes,t.Seconds);
}
int main(void)
{
    smartwatch_status=1;
    HAL_Init();
    SystemClock_Config();
    /* Flash instruction fetches otherwise stall the software renderer. */
    if(HAL_ICACHE_WaitForInvalidateComplete() != HAL_OK || HAL_ICACHE_Enable() != HAL_OK) Error_Handler();
    BSP_LED_Init(LED_RED);
    if(BSP_LCD_Init(0,LCD_ORIENTATION_PORTRAIT)!=BSP_ERROR_NONE) Error_Handler();
    BSP_LCD_SetActiveLayer(0,0);
    BSP_LCD_FillRect(0,0,0,480,480,LCD_COLOR_BLACK);
    BSP_LCD_DisplayOn(0);
    display_buffers_init();
    smartwatch_status=2;
    TS_Init_t touch={.Width=480,.Height=480,.Orientation=TS_ORIENTATION_PORTRAIT,.Accuracy=2};
    if(BSP_TS_Init(0,&touch)!=BSP_ERROR_NONE) Error_Handler();
    smartwatch_status=3;
    RTC_Init();
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
    watch_ui_init();
    watch_ui_set_rtc_source(smartwatch_rtc_lse != 0);
    watch_ui_set_clock_valid(clock_valid);
    smartwatch_status=4;
    uint32_t last_update=HAL_GetTick()-1000;
    while(1) {
        if(HAL_GetTick()-last_update>=1000) {
            refresh_rtc();
            last_update=HAL_GetTick();
            ++smartwatch_heartbeat;
        }
        lv_timer_handler();
        if(clock_valid && watch_alarm_active()) {
            if((HAL_GetTick() / 300) % 2) BSP_LED_On(LED_RED);
            else BSP_LED_Off(LED_RED);
        } else BSP_LED_Off(LED_RED);
        HAL_Delay(5);
    }
}
void Error_Handler(void)
{
    smartwatch_status |= 0x80000000;
    while(1) { BSP_LED_Toggle(LED_RED); HAL_Delay(200); }
}
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line; Error_Handler();
}
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* Enable voltage range 1 for frequency above 100 Mhz */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Switch to SMPS regulator instead of LDO */
  HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY);

  __HAL_RCC_PWR_CLK_DISABLE();

  /* MSI Oscillator enabled at reset (4Mhz), activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLFRACN= 0;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_PCLK3);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

