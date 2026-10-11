#include "views/watch_view_internal.h"
#include "viewmodels/watch_ida_vm.h"
#include "views/watch_art.h"
#include <math.h>

enum { IDA_SPEED=1,IDA_TARGET,IDA_MISSION,IDA_CLOSE,IDA_MINUS,IDA_PLUS,IDA_RUN,IDA_RESET,IDA_CLEAR,IDA_RANGE };
static lv_obj_t *ida_radar_layer,*ida_time_label,*ida_heading_label,*ida_speed_label,*ida_distance_label;
static lv_obj_t *ida_range_label,*ida_status_label,*ida_task_label,*ida_waypoint_label;
static lv_obj_t *ida_panel,*ida_panel_title,*ida_panel_value,*ida_panel_detail,*ida_run_label;
static lv_obj_t *ida_minus,*ida_plus,*ida_clear,*ida_run,*ida_reset;
static unsigned ida_panel_kind;
static uint32_t ida_last_tick;

static lv_obj_t *ida_text(const char *text,int x,int y,const lv_font_t *font,uint32_t color)
{
    lv_obj_t *label=label_at(screens[IDA],text,y,font);lv_obj_align(label,LV_ALIGN_TOP_MID,x,y);
    lv_obj_set_style_text_color(label,lv_color_hex(color),0);return label;
}
static void visible(lv_obj_t *object,bool show)
{ if(show) lv_obj_remove_flag(object,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(object,LV_OBJ_FLAG_HIDDEN); }
static const char *task_action(const watch_ida_state_t *s)
{ return s->phase==WATCH_IDA_RUNNING ? "Duraklat" : s->phase==WATCH_IDA_PAUSED ? "Devam" : "Baslat"; }
static void panel_refresh(void)
{
    const watch_ida_state_t *s=watch_ida_vm_state();
    visible(ida_minus,ida_panel_kind==IDA_SPEED);visible(ida_plus,ida_panel_kind==IDA_SPEED);
    visible(ida_clear,ida_panel_kind==IDA_TARGET);visible(ida_run,ida_panel_kind==IDA_MISSION);visible(ida_reset,ida_panel_kind==IDA_MISSION);
    if(ida_panel_kind==IDA_SPEED) {
        set_label(ida_panel_title,"Hiz ayari");set_label(ida_panel_value,s->speed);set_label(ida_panel_detail,"0 - 20 kn / SIM");
    } else if(ida_panel_kind==IDA_TARGET) {
        char value[40],detail[48];snprintf(value,sizeof value,"W1  %s  %s",s->distance,s->heading);
        snprintf(detail,sizeof detail,"Varis: %s",s->eta);set_label(ida_panel_title,"Hedef bilgisi");
        set_label(ida_panel_value,s->has_target ? value : "Radara dokun, hedef sec");set_label(ida_panel_detail,s->has_target ? detail : "SIM / sensor verisi yok");
        if(s->has_target) lv_obj_remove_state(ida_clear,LV_STATE_DISABLED);else lv_obj_add_state(ida_clear,LV_STATE_DISABLED);
    } else if(ida_panel_kind==IDA_MISSION) {
        set_label(ida_panel_title,"Gorev kontrolu");set_label(ida_panel_value,s->status);set_label(ida_panel_detail,"SIM / sensor verisi yok");set_label(ida_run_label,task_action(s));
        if(s->has_target && s->phase!=WATCH_IDA_ARRIVED) lv_obj_remove_state(ida_run,LV_STATE_DISABLED);else lv_obj_add_state(ida_run,LV_STATE_DISABLED);
    }
}
void ida_refresh(void)
{
    watch_ida_vm_clock(current_hour,current_minute,ui_clock_valid);
    if(!ida_time_label) return;
    const watch_ida_state_t *s=watch_ida_vm_state();
    set_label(ida_time_label,s->time);set_label(ida_heading_label,s->heading);set_label(ida_speed_label,s->speed);
    set_label(ida_distance_label,s->distance);set_label(ida_range_label,s->range);set_label(ida_status_label,s->status);
    set_label(ida_task_label,s->phase==WATCH_IDA_ARRIVED ? "Varildi" : task_action(s));
    visible(ida_waypoint_label,s->has_target);
    if(s->has_target) {
        float scale=152.0f/s->range_m,dx=(s->target_x-s->x)*scale,dy=(s->target_y-s->y)*scale;
        float length=sqrtf(dx*dx+dy*dy);if(length>145) { dx*=145/length;dy*=145/length; }
        set_label(ida_waypoint_label,s->distance_m>s->range_m ? "W1 >" : "W1");
        lv_obj_set_pos(ida_waypoint_label,240+(int)dx+(dx>0 ? -40 : 12),240+(int)dy+10);
    }
    if(ida_panel_kind) panel_refresh();
}
static void line(lv_layer_t *layer,int x1,int y1,int x2,int y2,uint32_t color,unsigned opacity,unsigned width,bool dashed)
{
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.p1=(lv_point_precise_t){x1,y1};d.p2=(lv_point_precise_t){x2,y2};
    d.color=lv_color_hex(color);d.opa=opacity;d.width=width;d.round_start=d.round_end=1;
    if(dashed) { d.dash_width=3;d.dash_gap=6; }lv_draw_line(layer,&d);
}
static void dot(lv_layer_t *layer,int x,int y,int radius,uint32_t color,unsigned opacity)
{
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(color);d.bg_opa=opacity;d.radius=LV_RADIUS_CIRCLE;
    lv_area_t area={x-radius,y-radius,x+radius,y+radius};lv_draw_rect(layer,&d,&area);
}
static void radar_draw(lv_event_t *e)
{
    lv_layer_t *layer=lv_event_get_layer(e);lv_area_t area;lv_obj_get_coords(ida_radar_layer,&area);
    int cx=area.x1+152,cy=area.y1+152;const watch_ida_state_t *s=watch_ida_vm_state();
    for(int i=12;i>=0;--i) {
        float angle=(s->sweep_degrees-i*2)*0.017453293f;
        line(layer,cx,cy,cx+(int)(150*sinf(angle)),cy-(int)(150*cosf(angle)),0x4DE6C8,i ? 60-i*4 : 175,2,false);
    }
    /* Two fictional contacts fade after the sweep; SIM is always visible. */
    const watch_ida_point_t contacts[]={{-330,-340},{460,165}};
    for(unsigned i=0;i<2;++i) {
        float dx=contacts[i].x-s->x,dy=contacts[i].y-s->y,scale=152.0f/s->range_m;
        if(dx*dx+dy*dy>s->range_m*(float)s->range_m) continue;
        float angle=atan2f(dx,-dy)*57.2957795f;if(angle<0) angle+=360;
        unsigned age=((unsigned)s->sweep_degrees+360-(unsigned)angle)%360;
        dot(layer,cx+(int)(dx*scale),cy+(int)(dy*scale),3,0x83EBD5,age<60 ? 255-age*3 : 55);
    }
    for(unsigned i=0;i<s->trace_count;++i) {
        float dx=(s->trace[i].x-s->x)*152/s->range_m,dy=(s->trace[i].y-s->y)*152/s->range_m;
        if(dx*dx+dy*dy<150*150) dot(layer,cx+(int)dx,cy+(int)dy,2,0x7BC8B9,65+190*i/WATCH_IDA_TRACE_POINTS);
    }
    if(s->has_target) {
        float dx=(s->target_x-s->x)*152/s->range_m,dy=(s->target_y-s->y)*152/s->range_m,length=sqrtf(dx*dx+dy*dy);
        if(length>145) { dx*=145/length;dy*=145/length; }
        int x=cx+(int)dx,y=cy+(int)dy;line(layer,cx,cy,x,y,0xFFD18B,200,2,true);
        dot(layer,x,y,6,0xFFD18B,255);dot(layer,x,y,3,0x04131E,255);
    }
    float a=s->heading_degrees*0.017453293f,sa=sinf(a),ca=cosf(a);
    const int points[5][2]={{0,-12},{7,8},{0,4},{-7,8},{0,-12}};
    for(unsigned i=1;i<5;++i) {
        int x1=cx+(int)(points[i-1][0]*ca-points[i-1][1]*sa),y1=cy+(int)(points[i-1][0]*sa+points[i-1][1]*ca);
        int x2=cx+(int)(points[i][0]*ca-points[i][1]*sa),y2=cy+(int)(points[i][0]*sa+points[i][1]*ca);
        line(layer,x1,y1,x2,y2,0x99F7DE,255,3,false);
    }
}
static void radar_clicked(lv_event_t *e)
{
    (void)e;lv_indev_t *input=lv_indev_active();if(!input || ida_panel_kind) return;
    lv_point_t p;lv_indev_get_point(input,&p);
    if(watch_ida_vm_target(p.x-240,p.y-240)) { ida_refresh();lv_obj_invalidate(ida_radar_layer); }
}
static void command(lv_event_t *e)
{
    unsigned action=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(action==IDA_SPEED || action==IDA_TARGET || action==IDA_MISSION) { ida_panel_kind=action;visible(ida_panel,true); }
    else if(action==IDA_CLOSE) { ida_panel_kind=0;visible(ida_panel,false); }
    else if(action==IDA_MINUS || action==IDA_PLUS) watch_ida_vm_speed(action==IDA_PLUS ? 5 : -5);
    else if(action==IDA_RANGE) watch_ida_vm_range();
    else {
        if(action==IDA_RUN) watch_ida_vm_toggle();
        else if(action==IDA_RESET) watch_ida_vm_reset();
        else if(action==IDA_CLEAR) watch_ida_vm_clear_target();
        ida_panel_kind=0;visible(ida_panel,false);
    }
    ida_last_tick=lv_tick_get();ida_refresh();lv_obj_invalidate(ida_radar_layer);
}
static void page_event(lv_event_t *e)
{
    ida_last_tick=lv_tick_get();ida_panel_kind=0;visible(ida_panel,false);
    if(lv_event_get_code(e)==LV_EVENT_SCREEN_UNLOADED) watch_ida_vm_pause();
    else { ida_refresh();lv_obj_invalidate(ida_radar_layer); }
}
static void ida_tick(lv_timer_t *timer)
{
    (void)timer;uint32_t now=lv_tick_get(),elapsed=now-ida_last_tick;ida_last_tick=now;
    if(lv_screen_active()!=screens[IDA] || !display_awake || displayed_notification!=-2) return;
    watch_ida_vm_step(elapsed>200 ? 200 : elapsed);ida_refresh();lv_obj_invalidate(ida_radar_layer);
}
static lv_obj_t *tap_area(int x,int y,int width,int height,unsigned action)
{
    lv_obj_t *b=lv_button_create(screens[IDA]);lv_obj_remove_style_all(b);lv_obj_set_pos(b,x,y);lv_obj_set_size(b,width,height);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x14333D),LV_STATE_PRESSED);lv_obj_set_style_bg_opa(b,LV_OPA_60,LV_STATE_PRESSED);
    lv_obj_set_style_radius(b,12,0);lv_obj_add_event_cb(b,command,LV_EVENT_CLICKED,(void *)(uintptr_t)action);return b;
}
static lv_obj_t *panel_button(const char *text,int x,int y,int width,unsigned action,lv_obj_t **label)
{
    lv_obj_t *l=button_at(ida_panel,text,x,y,width,command,(void *)(uintptr_t)action),*b=lv_obj_get_parent(l);
    lv_obj_set_height(b,48);lv_obj_set_style_text_font(l,&lv_font_montserrat_16,0);
    button_color_motion(b);if(label) *label=l;return b;
}
void ida_init(void)
{
    watch_ida_vm_init();page_header(IDA,"",MENU);
    lv_obj_t *radar=lv_image_create(screens[IDA]);lv_image_set_src(radar,&ida_radar);lv_obj_set_pos(radar,0,0);lv_obj_remove_flag(radar,LV_OBJ_FLAG_CLICKABLE);
    ida_radar_layer=lv_obj_create(screens[IDA]);lv_obj_remove_style_all(ida_radar_layer);lv_obj_set_pos(ida_radar_layer,88,88);lv_obj_set_size(ida_radar_layer,304,304);
    lv_obj_remove_flag(ida_radar_layer,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_event_cb(ida_radar_layer,radar_draw,LV_EVENT_DRAW_MAIN,NULL);
    lv_obj_add_event_cb(ida_radar_layer,radar_clicked,LV_EVENT_CLICKED,NULL);
    ida_text("N",0,16,&lv_font_montserrat_20,0xB1F4F5);ida_text("W",-207,228,&lv_font_montserrat_20,0xB1F4F5);
    ida_text("E",207,228,&lv_font_montserrat_20,0xB1F4F5);ida_text("S",0,447,&lv_font_montserrat_16,0xB1F4F5);
    ida_time_label=ida_text("--:--",0,48,&lv_font_montserrat_28,0xB1F4F5);
    lv_obj_t *mode=ida_text("SIM",0,84,&lv_font_montserrat_16,0xFFD18B);lv_obj_set_style_pad_hor(mode,9,0);lv_obj_set_style_pad_ver(mode,3,0);
    lv_obj_set_style_border_width(mode,1,0);lv_obj_set_style_border_color(mode,lv_color_hex(0xFFD18B),0);lv_obj_set_style_radius(mode,7,0);
    ida_heading_label=ida_text("",107,116,&lv_font_montserrat_24,0xB1F4F5);
    ida_waypoint_label=ida_text("W1",110,145,&lv_font_montserrat_14,0xFFD18B);
    lv_obj_set_align(ida_waypoint_label,LV_ALIGN_TOP_LEFT);
    ida_status_label=ida_text("HAZIR",0,330,&lv_font_montserrat_14,0xFFD18B);
    ida_range_label=ida_text("600 m",0,357,&lv_font_montserrat_14,0xB1F4F5);
    const char *labels[]={"HIZ","HEDEF","GOREV"};
    for(unsigned i=0;i<3;++i) ida_text(labels[i],((int)i-1)*96,391,&lv_font_montserrat_14,0xA8B9D2);
    ida_speed_label=ida_text("",-96,412,&lv_font_montserrat_20,0xB1F4F5);
    ida_distance_label=ida_text("",0,412,&lv_font_montserrat_20,0xB1F4F5);
    ida_task_label=ida_text("Baslat",96,412,&lv_font_montserrat_16,0xB1F4F5);
    tap_area(102,387,84,52,IDA_SPEED);tap_area(198,387,84,52,IDA_TARGET);tap_area(294,387,84,52,IDA_MISSION);tap_area(192,346,96,40,IDA_RANGE);
    ida_panel=lv_obj_create(screens[IDA]);lv_obj_set_pos(ida_panel,74,155);lv_obj_set_size(ida_panel,332,252);
    lv_obj_remove_flag(ida_panel,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(ida_panel,0,0);lv_obj_set_style_radius(ida_panel,22,0);
    lv_obj_set_style_bg_color(ida_panel,lv_color_hex(0x0C2630),0);lv_obj_set_style_bg_opa(ida_panel,LV_OPA_COVER,0);
    lv_obj_set_style_text_color(ida_panel,lv_color_hex(0xB1F4F5),0);
    lv_obj_set_style_border_color(ida_panel,lv_color_hex(0x438B92),0);lv_obj_set_style_border_width(ida_panel,1,0);
    ida_panel_title=label_at(ida_panel,"",18,&lv_font_montserrat_20);ida_panel_value=label_at(ida_panel,"",55,&lv_font_montserrat_20);
    ida_panel_detail=label_at(ida_panel,"",91,&lv_font_montserrat_14);
    ida_minus=panel_button("-",-72,126,56,IDA_MINUS,NULL);ida_plus=panel_button("+",72,126,56,IDA_PLUS,NULL);
    ida_clear=panel_button("Hedefi kaldir",0,126,180,IDA_CLEAR,NULL);
    ida_run=panel_button("Baslat",-76,126,132,IDA_RUN,&ida_run_label);ida_reset=panel_button("Sifirla",76,126,132,IDA_RESET,NULL);
    panel_button("Radara don",0,188,180,IDA_CLOSE,NULL);visible(ida_panel,false);
    lv_obj_add_event_cb(screens[IDA],page_event,LV_EVENT_SCREEN_LOADED,NULL);lv_obj_add_event_cb(screens[IDA],page_event,LV_EVENT_SCREEN_UNLOADED,NULL);
    ida_last_tick=lv_tick_get();ida_refresh();lv_timer_create(ida_tick,50,NULL);
}
