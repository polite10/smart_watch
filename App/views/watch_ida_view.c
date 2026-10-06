#include "views/watch_view_internal.h"
#include "viewmodels/watch_ida_vm.h"
#include "views/watch_art.h"
static lv_obj_t *ida_text(const char *text,int x,int y,const lv_font_t *font,uint32_t color)
{
    lv_obj_t *label=label_at(screens[IDA],text,y,font);lv_obj_align(label,LV_ALIGN_TOP_MID,x,y);
    lv_obj_set_style_text_color(label,lv_color_hex(color),0);return label;
}
void ida_init(void)
{
    page_header(IDA,"",MENU);
    lv_obj_t *radar=lv_image_create(screens[IDA]);lv_image_set_src(radar,&ida_radar);lv_obj_set_pos(radar,0,0);
    lv_obj_remove_flag(radar,LV_OBJ_FLAG_CLICKABLE);
    const watch_ida_state_t *s=watch_ida_vm_state();
    ida_text("N",0,16,&lv_font_montserrat_20,0xB1F4F5);
    ida_text("W",-207,228,&lv_font_montserrat_20,0xB1F4F5);
    ida_text("E",207,228,&lv_font_montserrat_20,0xB1F4F5);
    ida_text("S",0,447,&lv_font_montserrat_16,0xB1F4F5);
    ida_text(s->time,0,48,&lv_font_montserrat_28,0xB1F4F5);
    lv_obj_t *mode=ida_text(s->mode,0,84,&lv_font_montserrat_16,0xFFD18B);
    lv_obj_set_style_pad_hor(mode,9,0);lv_obj_set_style_pad_ver(mode,3,0);
    lv_obj_set_style_border_width(mode,1,0);lv_obj_set_style_border_color(mode,lv_color_hex(0xFFD18B),0);lv_obj_set_style_radius(mode,7,0);
    ida_text(s->link,81,86,&lv_font_montserrat_14,0xA8B9D2);
    ida_text(s->heading,107,116,&lv_font_montserrat_24,0xB1F4F5);
    ida_text("W1",110,145,&lv_font_montserrat_16,0xFFD18B);
    ida_text(s->range,0,357,&lv_font_montserrat_14,0xB1F4F5);
    const char *labels[]={"HIZ","HEDEF","PIL"};const char *values[]={s->speed,s->distance,s->battery};
    for(unsigned i=0;i<3;++i) {
        int x=((int)i-1)*84;ida_text(labels[i],x,391,&lv_font_montserrat_14,0xA8B9D2);
        ida_text(values[i],x,412,&lv_font_montserrat_24,0xB1F4F5);
    }
}
