#include "views/watch_view_internal.h"
static lv_obj_t *clock_day,*clock_month,*clock_year,*clock_hour,*clock_minute,*clock_hint,*clock_result;

static void clock_options(lv_obj_t *roller, unsigned first, unsigned count, bool year)
{
    char options[512]={0};
    for(unsigned i=0; i<count; ++i) {
        char value[8];
        snprintf(value,sizeof value,year ? "%04u%s" : "%02u%s",first+i,i+1==count ? "" : "\n");
        strcat(options,value);
    }
    lv_roller_set_options(roller,options,LV_ROLLER_MODE_NORMAL);
}

static void clock_date_changed(lv_event_t *e)
{
    (void)e; watch_clock_vm_date(2000+lv_roller_get_selected(clock_year),1+lv_roller_get_selected(clock_month),1+lv_roller_get_selected(clock_day));
    const watch_clock_editor_t *state=watch_clock_vm_state();
    clock_options(clock_day,1,state->days,false);
    lv_roller_set_selected(clock_day,state->draft.day-1,LV_ANIM_OFF); set_label(clock_result,state->result);
}

void clock_editor_open(void)
{
    watch_clock_vm_open(); const watch_clock_editor_t *state=watch_clock_vm_state();
    lv_roller_set_selected(clock_year,state->draft.year-2000,LV_ANIM_OFF);
    lv_roller_set_selected(clock_month,state->draft.month-1,LV_ANIM_OFF);
    clock_options(clock_day,1,state->days,false);
    lv_roller_set_selected(clock_day,state->draft.day-1,LV_ANIM_OFF);
    lv_roller_set_selected(clock_hour,state->draft.hour,LV_ANIM_OFF);
    lv_roller_set_selected(clock_minute,state->draft.minute,LV_ANIM_OFF);
    set_label(clock_hint,state->hint); set_label(clock_result,state->result);
    watch_navigation_show(CLOCK_EDIT);
}

static void clock_save(lv_event_t *e)
{
    (void)e; watch_datetime_t time={2000+lv_roller_get_selected(clock_year),1+lv_roller_get_selected(clock_month),1+lv_roller_get_selected(clock_day),lv_roller_get_selected(clock_hour),lv_roller_get_selected(clock_minute),0};
    if(watch_clock_vm_save(time)) watch_navigation_show(HOME);
    else set_label(clock_result,watch_clock_vm_state()->result);
}

static lv_obj_t *clock_roller(int x,int y,int width,unsigned first,unsigned count,bool year)
{
    lv_obj_t *roller=lv_roller_create(screens[CLOCK_EDIT]);
    style_roller(roller);
    lv_obj_set_style_text_font(roller,&lv_font_montserrat_24,LV_PART_MAIN);
    lv_obj_set_style_text_font(roller,&lv_font_montserrat_24,LV_PART_SELECTED);
    lv_obj_set_style_text_line_space(roller,6,LV_PART_MAIN);
    clock_options(roller,first,count,year);
    lv_roller_set_visible_row_count(roller,3);
    lv_obj_set_width(roller,width);
    lv_obj_align(roller,LV_ALIGN_TOP_MID,x,y);
    return roller;
}

void clock_editor_init(void)
{
    page_header(CLOCK_EDIT,"Saat ve tarih",MENU);
    clock_hint=label_at(screens[CLOCK_EDIT],"",96,&lv_font_montserrat_14);
    lv_obj_set_width(clock_hint,330); lv_obj_set_style_text_align(clock_hint,LV_TEXT_ALIGN_CENTER,0);
    label_at(screens[CLOCK_EDIT],"GUN       AY           YIL",137,&lv_font_montserrat_14);
    clock_day=clock_roller(-100,157,72,1,31,false);
    clock_month=clock_roller(-18,157,72,1,12,false);
    clock_year=clock_roller(89,157,98,2000,100,true);
    lv_obj_add_event_cb(clock_month,clock_date_changed,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(clock_year,clock_date_changed,LV_EVENT_VALUE_CHANGED,NULL);
    label_at(screens[CLOCK_EDIT],"SAAT          DAKIKA",270,&lv_font_montserrat_14);
    clock_hour=clock_roller(-58,292,82,0,24,false);
    clock_minute=clock_roller(58,292,82,0,60,false);
    label_at(screens[CLOCK_EDIT],":",317,&lv_font_montserrat_24);
    clock_result=label_at(screens[CLOCK_EDIT],"",395,&lv_font_montserrat_14);
    lv_obj_set_width(clock_result,330); lv_obj_set_style_text_align(clock_result,LV_TEXT_ALIGN_CENTER,0);
    button_at(screens[CLOCK_EDIT],"Kaydet",0,412,144,clock_save,NULL);
}
