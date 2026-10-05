#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
#include <string.h>
static watch_calendar_state_t state;
const watch_calendar_state_t *watch_calendar_vm_state(void) { return &state; }
void watch_calendar_vm_init(uint16_t year, uint8_t month)
{
    if(year<2000 || year>2099 || month<1 || month>12) return;
    state.year=year; state.month=month; state.selected=0; watch_calendar_vm_refresh();
}
void watch_calendar_vm_refresh(void)
{
    static const char *months[]={"Ocak","Subat","Mart","Nisan","Mayis","Haziran","Temmuz","Agustos","Eylul","Ekim","Kasim","Aralik"};
    snprintf(state.title,sizeof state.title,"%s %u",months[state.month-1],state.year);
    if(state.selected) snprintf(state.selection,sizeof state.selection,"%02u.%02u.%u",state.selected,state.month,state.year);
    else snprintf(state.selection,sizeof state.selection,"Gun secin");
    unsigned first=(watch_epoch(state.year,state.month,1,0,0,0)/86400U+5U)%7U;
    unsigned days=watch_days_in_month(state.year,state.month);
    const watch_app_state_t *app=watch_app_vm_state();
    for(unsigned i=0;i<42;++i) {
        state.numbers[i]=i>=first && i<first+days ? i-first+1 : 0;
        state.today[i]=state.numbers[i] && app->clock_valid && state.year==app->time.year && state.month==app->time.month && state.numbers[i]==app->time.day;
    }
    state.previous_disabled=state.year==2000 && state.month==1;
    state.next_disabled=state.year==2099 && state.month==12;
}
void watch_calendar_vm_move(bool next)
{
    if(next) { if(state.next_disabled) return; if(++state.month==13) { state.month=1; ++state.year; } }
    else { if(state.previous_disabled) return; if(--state.month==0) { state.month=12; --state.year; } }
    state.selected=0; watch_calendar_vm_refresh();
}
void watch_calendar_vm_select(unsigned slot)
{ if(slot<42 && state.numbers[slot]) { state.selected=state.numbers[slot]; watch_calendar_vm_refresh(); } }
