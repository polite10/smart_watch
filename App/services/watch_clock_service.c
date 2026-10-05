#include "services/watch_clock_service.h"
static watch_clock_observer_t changed;
void watch_clock_service_subscribe(watch_clock_observer_t observer) { changed=observer; }
bool watch_clock_is_valid(void) { return watch_platform_clock_valid(); }
bool watch_clock_set_datetime(uint16_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second)
{
    watch_datetime_t time={year,month,day,hour,minute,second};
    if(!watch_datetime_valid(year,month,day,hour,minute,second)) return false;
    bool ok=watch_platform_clock_set(time);
    if(changed) changed(ok,ok ? &time : 0);
    return ok;
}
