#include "viewmodels/watch_viewmodels.h"
#include "services/watch_clock_service.h"
#include "build_time.h"
#include <stdio.h>
static watch_clock_editor_t state;
const watch_clock_editor_t *watch_clock_vm_state(void) { return &state; }
void watch_clock_vm_open(void)
{
    const watch_app_state_t *app=watch_app_vm_state();
    state.draft=app->clock_valid ? app->time : (watch_datetime_t){BUILD_YEAR,BUILD_MONTH,BUILD_DAY,0,0,0};
    state.days=watch_days_in_month(state.draft.year,state.draft.month);
    snprintf(state.hint,sizeof state.hint,"%s",app->clock_valid ? "Telefonunuzdaki saat ve tarihi girin.\nSaat bicimi: 24 saat" : "Saat bilinmiyor. Telefonunuza bakin.\nPrizden saat bilgisi alinmaz.");
    snprintf(state.result,sizeof state.result,"Kaydet: saniye 00 olur.");
}
void watch_clock_vm_date(uint16_t year, uint8_t month, uint8_t day)
{
    state.days=watch_days_in_month(year,month); if(!state.days) return;
    state.draft.year=year; state.draft.month=month; state.draft.day=day>state.days ? state.days : day<1 ? 1 : day;
    snprintf(state.result,sizeof state.result,"Kaydet: saniye 00 olur.");
}
bool watch_clock_vm_save(watch_datetime_t time)
{
    time.second=0;
    if(!watch_clock_set_datetime(time.year,time.month,time.day,time.hour,time.minute,0)) {
        snprintf(state.result,sizeof state.result,"Saat kaydedilemedi. Tekrar deneyin."); return false;
    }
    state.draft=time; return true;
}
