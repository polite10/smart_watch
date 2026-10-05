#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
watch_water_state_t watch_water_vm_state(void)
{
    watch_water_state_t state={0}; const watch_data_t *data=watch_model_data();
    unsigned ml=data->water[0].ml, goal=data->water_goal;
    bool valid=watch_app_vm_state()->clock_valid;
    state.progress=!valid ? 0 : ml>=goal ? 100 : ml*100/goal;
    if(valid) {
        snprintf(state.total,sizeof state.total,"%u ml",ml);
        if(ml>=goal) snprintf(state.remaining,sizeof state.remaining,"Hedef tamamlandi");
        else snprintf(state.remaining,sizeof state.remaining,"Kalan: %u ml",goal-ml);
        snprintf(state.summary,sizeof state.summary,"%u / %u ml",ml,goal);
    } else {
        snprintf(state.total,sizeof state.total,"-- ml");
        snprintf(state.remaining,sizeof state.remaining,"Once saat ve tarihi ayarlayin");
        snprintf(state.summary,sizeof state.summary,"Saat ayari gerekli");
    }
    snprintf(state.goal,sizeof state.goal,"Gunluk hedef: %u ml",goal);
    if(data->reminder_minutes) snprintf(state.reminder,sizeof state.reminder,"Hatirlatma: %u dk",data->reminder_minutes);
    else snprintf(state.reminder,sizeof state.reminder,"Hatirlatma: Kapali");
    return state;
}
void watch_water_vm_history(unsigned index, char *text, unsigned size)
{
    if(index>=WATCH_HISTORY) { if(size) text[0]=0; return; }
    const watch_water_day_t *d=&watch_model_data()->water[index];
    if(d->day) snprintf(text,size,"%02u.%02u.%04u  %u / %u ml",d->day%100,d->day/100%100,d->day/10000,d->ml,d->goal);
    else snprintf(text,size,"--");
}
bool watch_water_vm_add(int glasses)
{
    if(!watch_app_vm_state()->clock_valid) return false;
    watch_water_add(glasses); return watch_app_vm_save();
}
bool watch_water_vm_goal(int delta)
{
    if(!watch_app_vm_state()->clock_valid) return false;
    watch_water_goal(delta); return watch_app_vm_save();
}
void watch_water_vm_reminder(void)
{
    uint16_t m=watch_model_data()->reminder_minutes;
    watch_reminder_set(m==0 ? 30 : m==30 ? 60 : m==60 ? 120 : 0); watch_app_vm_save();
}
