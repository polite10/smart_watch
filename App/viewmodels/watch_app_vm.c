#include "viewmodels/watch_viewmodels.h"
#include "services/watch_repository.h"
#include "services/watch_clock_service.h"
static watch_app_state_t state;
static watch_vm_observer_t changed;
static void clock_changed(bool valid, const watch_datetime_t *time)
{
    if(valid && time) watch_app_vm_time_changed(*time);
    else watch_app_vm_set_valid(false);
}
void watch_app_vm_init(watch_vm_observer_t observer)
{
    watch_model_init();
    state = (watch_app_state_t){.time={2026,10,4,0,0,0}, .storage_ok=true};
    changed=observer; watch_clock_service_subscribe(clock_changed);
    watch_calculator_vm_init(); watch_calendar_vm_init(state.time.year,state.time.month);
}
const watch_app_state_t *watch_app_vm_state(void) { return &state; }
bool watch_app_vm_save(void) { state.storage_ok=watch_model_save(); return state.storage_ok; }
void watch_app_vm_set_valid(bool valid)
{
    state.clock_valid=valid;
    if(changed) changed(WATCH_VM_CLOCK);
}
void watch_app_vm_set_datetime(watch_datetime_t time)
{
    if(!state.clock_valid || !watch_datetime_valid(time.year,time.month,time.day,time.hour,time.minute,time.second)) return;
    state.time=time;
    if(watch_model_tick(watch_day_key(time.year,time.month,time.day),
        watch_epoch(time.year,time.month,time.day,time.hour,time.minute,time.second),time.hour,time.minute)) watch_app_vm_save();
    if(changed) changed(WATCH_VM_TIME);
}
void watch_app_vm_time_changed(watch_datetime_t time)
{
    if(!watch_datetime_valid(time.year,time.month,time.day,time.hour,time.minute,time.second)) return;
    watch_model_time_changed(watch_day_key(time.year,time.month,time.day),
        watch_epoch(time.year,time.month,time.day,time.hour,time.minute,time.second),time.hour,time.minute);
    watch_app_vm_set_valid(true);
    watch_app_vm_set_datetime(time);
    watch_calendar_vm_init(time.year,time.month);
    watch_app_vm_save();
    if(changed) changed(WATCH_VM_TIME_CHANGED);
}
void watch_app_vm_set_rtc_source(bool crystal) { state.rtc_crystal=crystal; }
