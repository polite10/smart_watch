#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
static watch_faces_state_t state={.datetime={2026,10,4,0,0,0}, .clock_valid=true};
const watch_faces_state_t *watch_faces_vm_state(void) { return &state; }
unsigned watch_faces_vm_style(void) { return watch_model_data()->face_style; }
void watch_faces_vm_open(void) { state.draft_style=watch_faces_vm_style(); state.save_ok=true; }
void watch_faces_vm_choose(unsigned style) { if(style<WATCH_FACE_COUNT) state.draft_style=style; }
bool watch_faces_vm_apply(void)
{
    unsigned old=watch_faces_vm_style();
    if(!watch_model_set_face(state.draft_style)) return false;
    state.save_ok=watch_app_vm_save();
    if(!state.save_ok) watch_model_set_face(old);
    return state.save_ok;
}
void watch_faces_vm_refresh(watch_datetime_t t, bool twelve, bool valid)
{
    static const char *months[]={"OCAK","SUBAT","MART","NISAN","MAYIS","HAZIRAN","TEMMUZ","AGUSTOS","EYLUL","EKIM","KASIM","ARALIK"};
    state.datetime=t; state.twelve=twelve;
    valid=valid && watch_datetime_valid(t.year,t.month,t.day,t.hour,t.minute,t.second);
    state.clock_valid=valid;
    if(valid) {
        unsigned h=twelve ? (t.hour%12 ? t.hour%12 : 12) : t.hour;
        snprintf(state.time,sizeof state.time,"%02u:%02u",h,t.minute);
        snprintf(state.period,sizeof state.period,"%s",twelve ? (t.hour<12 ? "AM" : "PM") : "");
        snprintf(state.seconds,sizeof state.seconds,"%02u",t.second);
        snprintf(state.orbit_seconds,sizeof state.orbit_seconds,"%02u sn",t.second);
        snprintf(state.date,sizeof state.date,"%u %s %u",t.day,months[t.month-1],t.year);
    } else {
        snprintf(state.time,sizeof state.time,"--:--"); state.period[0]=0;
        snprintf(state.seconds,sizeof state.seconds,"--"); snprintf(state.orbit_seconds,sizeof state.orbit_seconds,"-- sn");
        snprintf(state.date,sizeof state.date,"SAATI AYARLAYIN");
    }
    state.hour_angle=(t.hour%12)*30+t.minute*.5f;
    state.minute_angle=t.minute*6+t.second*.1f; state.second_angle=t.second*6;
    int next=-1; unsigned closest=1441;
    const watch_data_t *data=watch_model_data();
    for(unsigned i=0;i<WATCH_ALARMS;++i) if(data->alarms[i].enabled) {
        unsigned target=data->alarms[i].hour*60+data->alarms[i].minute;
        unsigned distance=(target+1440-t.hour*60-t.minute)%1440;
        if(distance<closest) { closest=distance; next=i; }
    }
    if(!valid) snprintf(state.alarm,sizeof state.alarm,"Alarm icin saati ayarlayin");
    else if(next>=0) snprintf(state.alarm,sizeof state.alarm,"Alarm  %02u:%02u",data->alarms[next].hour,data->alarms[next].minute);
    else snprintf(state.alarm,sizeof state.alarm,"Alarm kapali");
}

void watch_faces_vm_set_battery(uint8_t percent, bool valid)
{
    state.battery_valid=valid;
    if(valid) snprintf(state.battery,sizeof state.battery,"Pil %u%%",percent>100 ? 100 : percent);
    else snprintf(state.battery,sizeof state.battery,"Pil --");
}
