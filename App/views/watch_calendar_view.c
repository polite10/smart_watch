#include "views/watch_view_internal.h"
static lv_obj_t *calendar,*day_label,*calendar_month_label,*calendar_labels[42],*calendar_arrows[2];
static uint8_t calendar_numbers[42];

static void selected_date(lv_event_t *e) { watch_calendar_vm_select((unsigned)(uintptr_t)lv_event_get_user_data(e)); calendar_refresh(); }

static void calendar_move(lv_event_t *e) { watch_calendar_vm_move((uintptr_t)lv_event_get_user_data(e)!=0); calendar_refresh(); }

void calendar_refresh(void)
{
    lv_obj_set_style_text_color(calendar,lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC),0);
    watch_calendar_vm_refresh();
    const watch_calendar_state_t *state=watch_calendar_vm_state();
    char value[32]; set_label(calendar_month_label,state->title); set_label(day_label,state->selection);
    for(unsigned i=0; i<42; ++i) {
        lv_obj_t *button=lv_obj_get_parent(calendar_labels[i]);
        bool valid=state->numbers[i]!=0;
        calendar_numbers[i]=state->numbers[i];
        if(valid) {
            snprintf(value,sizeof value,"%u",calendar_numbers[i]);
            set_label(calendar_labels[i],value);
            lv_obj_remove_state(button,LV_STATE_DISABLED);
        } else { set_label(calendar_labels[i],""); lv_obj_add_state(button,LV_STATE_DISABLED); }
        bool today=state->today[i];
        bool selected=valid && calendar_numbers[i]==state->selected;
        lv_obj_set_style_bg_opa(button,valid ? LV_OPA_COVER : LV_OPA_TRANSP,0);
        lv_obj_set_style_bg_color(button,lv_color_hex(selected ? 0x86CDB8 : 0x26344C),0);
        lv_obj_set_style_text_color(calendar_labels[i],lv_color_hex(selected ? 0x152C2C : 0xF0F4FC),0);
        lv_obj_set_style_border_width(button,today ? 2 : 0,0);
        lv_obj_set_style_border_color(button,lv_color_hex(0x82D9F4),0);
    }
    for(unsigned i=0; i<2; ++i) {
        bool disabled=i==0 ? state->previous_disabled : state->next_disabled;
        if(disabled) lv_obj_add_state(calendar_arrows[i],LV_STATE_DISABLED);
        else lv_obj_remove_state(calendar_arrows[i],LV_STATE_DISABLED);
    }
}

void calendar_init(void)
{
    calendar=lv_obj_create(screens[CALENDAR]);
    lv_obj_set_size(calendar,480,480); lv_obj_set_pos(calendar,0,0);
    lv_obj_set_style_pad_all(calendar,0,0); lv_obj_set_style_border_width(calendar,0,0);
    lv_obj_set_style_bg_opa(calendar,LV_OPA_TRANSP,0);
    lv_obj_remove_flag(calendar,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    calendar_month_label=label_at(calendar,"",102,&lv_font_montserrat_20);
    calendar_arrows[0]=lv_obj_get_parent(round_action(calendar,LV_SYMBOL_LEFT,-126,92,48,0x37465F,calendar_move,(void *)0));
    calendar_arrows[1]=lv_obj_get_parent(round_action(calendar,LV_SYMBOL_RIGHT,126,92,48,0x37465F,calendar_move,(void *)1));
    static const char *days[]={"Pt","Sa","Ca","Pe","Cu","Ct","Pa"};
    for(unsigned i=0; i<7; ++i) {
        lv_obj_t *label=label_at(calendar,days[i],151,&lv_font_montserrat_14);
        lv_obj_align(label,LV_ALIGN_TOP_MID,((int)i-3)*46,151);
    }
    const int curve[]={8,5,2,-2,-5,-8};
    for(unsigned i=0; i<42; ++i) {
        int col=(int)(i%7)-3, row=i/7;
        int y=175+row*44+curve[row]*(col<0 ? -col : col)/3;
        calendar_labels[i]=round_action(calendar,"",col*46,y,40,0x26344C,selected_date,(void *)(uintptr_t)i);
        lv_obj_set_style_text_font(calendar_labels[i],&lv_font_montserrat_20,0);
    }
    day_label=label_at(calendar,"Gun secin",445,&lv_font_montserrat_20);
    calendar_refresh();
}
