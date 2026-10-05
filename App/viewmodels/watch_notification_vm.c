#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
watch_notification_state_t watch_notification_vm_state(void)
{
    watch_notification_state_t state={.selected=-2};
    if(!watch_app_vm_state()->clock_valid) return state;
    int selected=watch_alarm_next();
    if(selected<0) selected=watch_water_reminder_due() ? -1 : -2;
    state.selected=selected;
    if(selected>=0) {
        const watch_alarm_t *a=&watch_model_data()->alarms[selected];
        snprintf(state.title,sizeof state.title,"ALARM");
        snprintf(state.text,sizeof state.text,"Alarm %u - %02u:%02u\nEkran ve LED uyarisi",selected+1,a->hour,a->minute);
        snprintf(state.first,sizeof state.first,"5 dk ertele"); snprintf(state.second,sizeof state.second,"Durdur");
    } else if(selected==-1) {
        snprintf(state.title,sizeof state.title,"SU ZAMANI");
        snprintf(state.text,sizeof state.text,"Bir bardak su ictiniz mi?\n1 bardak = 200 ml");
        snprintf(state.first,sizeof state.first,"+200 ml"); snprintf(state.second,sizeof state.second,"Daha sonra");
    }
    return state;
}
void watch_notification_vm_action(bool first)
{
    int selected=watch_notification_vm_state().selected;
    if(selected>=0) watch_alarm_ack(selected,first);
    else if(selected==-1) {
        if(first) watch_water_vm_add(1);
        else watch_reminder_set(watch_model_data()->reminder_minutes);
    }
}
bool watch_notification_vm_alarm_active(void)
{ return watch_app_vm_state()->clock_valid && watch_alarm_active(); }
