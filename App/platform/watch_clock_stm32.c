#include "services/watch_clock_service.h"
#include "smartwatch.h"
static RTC_HandleTypeDef rtc;
static bool clock_valid;
#define RTC_TIME_SET_MAGIC 0x54494D32U
void watch_platform_clock_init(void)
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
bool watch_platform_clock_set(watch_datetime_t time)
{
    uint16_t year=time.year; uint8_t month=time.month,day=time.day,hour=time.hour,minute=time.minute,second=time.second;

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
        return false;
    }
    HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR1,RTC_TIME_SET_MAGIC);
    clock_valid=true;
    return true;
}
bool watch_platform_clock_read(watch_datetime_t *time)
{
    if(!clock_valid) return false;
    RTC_TimeTypeDef t={0}; RTC_DateTypeDef d={0};
    HAL_StatusTypeDef ts=HAL_RTC_GetTime(&rtc,&t,RTC_FORMAT_BIN);
    HAL_StatusTypeDef ds=HAL_RTC_GetDate(&rtc,&d,RTC_FORMAT_BIN);
    if(ts!=HAL_OK || ds!=HAL_OK || !watch_datetime_valid(2000+d.Year,d.Month,d.Date,t.Hours,t.Minutes,t.Seconds)) {
        clock_valid=false;
        HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR1,0);
        return false;
    }
    *time=(watch_datetime_t){2000+d.Year,d.Month,d.Date,t.Hours,t.Minutes,t.Seconds};
    return true;
}
bool watch_platform_clock_valid(void) { return clock_valid; }
