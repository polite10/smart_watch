#include "viewmodels/watch_ida_vm.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static watch_ida_state_t state;
static uint32_t sweep_ms;
static void update(void)
{
    float dx=state.target_x-state.x,dy=state.target_y-state.y;
    state.distance_m=state.has_target ? sqrtf(dx*dx+dy*dy) : 0;
    if(state.has_target && state.distance_m>0.001f) {
        state.heading_degrees=atan2f(dx,-dy)*57.2957795f;
        if(state.heading_degrees<0) state.heading_degrees+=360;
    }
    state.eta_valid=state.has_target && (state.speed_tenths>0 || state.distance_m==0);
    state.eta_seconds=state.eta_valid && state.distance_m>0 ? (uint32_t)ceilf(state.distance_m/(state.speed_tenths*0.0514444f)) : 0;
    snprintf(state.heading,sizeof state.heading,state.has_target ? "%03u\xC2\xB0" : "---",(unsigned)(state.heading_degrees+0.5f)%360);
    snprintf(state.speed,sizeof state.speed,"%u.%u kn",state.speed_tenths/10,state.speed_tenths%10);
    if(state.has_target) snprintf(state.distance,sizeof state.distance,"%u m",(unsigned)(state.distance_m+0.5f));
    else strcpy(state.distance,"-- m");
    snprintf(state.range,sizeof state.range,"%u m",state.range_m);
    if(state.eta_valid) snprintf(state.eta,sizeof state.eta,"%lu dk %02lu sn",(unsigned long)(state.eta_seconds/60),(unsigned long)(state.eta_seconds%60));
    else strcpy(state.eta,"--");
    const char *status=state.phase==WATCH_IDA_RUNNING ? "AKTIF" : state.phase==WATCH_IDA_PAUSED ? "DURAKLATILDI" : state.phase==WATCH_IDA_ARRIVED ? "HEDEFE VARILDI" : "HAZIR";
    strcpy(state.status,status);
}
const watch_ida_state_t *watch_ida_vm_state(void) { return &state; }
void watch_ida_vm_reset(void)
{
    char time[sizeof state.time];memcpy(time,state.time,sizeof time);
    memset(&state,0,sizeof state);memcpy(state.time,time,sizeof time);
    state.mode="SIM";state.speed_tenths=64;state.range_m=600;state.trace_count=1;sweep_ms=0;update();
}
void watch_ida_vm_init(void)
{
    memset(&state,0,sizeof state);strcpy(state.time,"--:--");watch_ida_vm_reset();
    state.target_x=296.98485f;state.target_y=-296.98485f;state.has_target=true;update();
}
void watch_ida_vm_clock(uint8_t hour,uint8_t minute,bool valid)
{
    if(valid && hour<24 && minute<60) snprintf(state.time,sizeof state.time,"%02u:%02u",hour,minute);
    else strcpy(state.time,"--:--");
}
bool watch_ida_vm_target(int dx,int dy)
{
    if(dx<-152 || dx>152 || dy<-152 || dy>152 || dx*dx+dy*dy>152*152 || dx*dx+dy*dy<8*8) return false;
    float x=state.x+dx*(state.range_m/152.0f),y=state.y+dy*(state.range_m/152.0f);
    if(fabsf(x)>100000 || fabsf(y)>100000) return false;
    state.target_x=x;state.target_y=y;state.has_target=true;
    /* Retargeting changes the route immediately; a running task keeps running. */
    if(state.phase!=WATCH_IDA_RUNNING) state.phase=WATCH_IDA_READY;
    update();return true;
}
void watch_ida_vm_clear_target(void) { state.has_target=false;state.phase=WATCH_IDA_READY;update(); }
void watch_ida_vm_speed(int delta_tenths)
{
    /* Clamp the argument before addition, including malformed external commands. */
    if(delta_tenths>200) delta_tenths=200;
    if(delta_tenths<-200) delta_tenths=-200;
    int value=state.speed_tenths+delta_tenths;state.speed_tenths=value<0 ? 0 : value>200 ? 200 : value;update();
}
void watch_ida_vm_range(void) { state.range_m=state.range_m==200 ? 600 : state.range_m==600 ? 1200 : 200;update(); }
void watch_ida_vm_toggle(void)
{
    if(!state.has_target || state.phase==WATCH_IDA_ARRIVED) return;
    state.phase=state.phase==WATCH_IDA_RUNNING ? WATCH_IDA_PAUSED : WATCH_IDA_RUNNING;update();
}
void watch_ida_vm_pause(void) { if(state.phase==WATCH_IDA_RUNNING) { state.phase=WATCH_IDA_PAUSED;update(); } }
static void trace(bool final)
{
    watch_ida_point_t last=state.trace[state.trace_count-1];float dx=state.x-last.x,dy=state.y-last.y;
    if(dx*dx+dy*dy<9 && !final) return;
    if(state.trace_count==WATCH_IDA_TRACE_POINTS) { memmove(state.trace,state.trace+1,(WATCH_IDA_TRACE_POINTS-1)*sizeof *state.trace);--state.trace_count; }
    state.trace[state.trace_count++]=(watch_ida_point_t){state.x,state.y};
}
void watch_ida_vm_step(uint32_t elapsed_ms)
{
    /* View supplies visible-screen time only. Do not catch up after a stalled UI. */
    if(elapsed_ms>1000) elapsed_ms=1000;
    sweep_ms=(sweep_ms+elapsed_ms)%4000;state.sweep_degrees=(uint16_t)(sweep_ms*360/4000);
    if(state.phase!=WATCH_IDA_RUNNING || !state.has_target || !state.speed_tenths || !elapsed_ms) return;
    float travel=state.speed_tenths*0.0514444f*(elapsed_ms/1000.0f);
    bool arrived=travel>=state.distance_m;
    if(arrived) { state.x=state.target_x;state.y=state.target_y;state.phase=WATCH_IDA_ARRIVED; }
    else {
        float ratio=travel/state.distance_m;state.x+=(state.target_x-state.x)*ratio;state.y+=(state.target_y-state.y)*ratio;
    }
    trace(arrived);update();
}
