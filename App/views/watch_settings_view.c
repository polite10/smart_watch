#include "views/watch_view_internal.h"
static lv_obj_t *format_button_label,*theme_button_label,*settings_rows[4],*settings_switch,*rtc_source_label,*clock_status_label;
#include "watch_icons.h"

static void format_clicked(lv_event_t *e)
{
    (void)e;
    watch_settings_vm_toggle_format();
    settings_refresh();
    refresh_time();
    storage_refresh();
}

static void theme_clicked(lv_event_t *e)
{ (void)e; watch_settings_vm_toggle_theme(); theme_apply(); settings_refresh(); calendar_refresh(); watch_faces_apply_saved(); storage_refresh(); }

lv_obj_t *settings_row(unsigned index, const char *title, const char *symbol,
                              const lv_image_dsc_t *image, uint32_t accent,
                              int y, int width, lv_event_cb_t callback, void *data)
{
    lv_obj_t *title_label=button_at(screens[SETTINGS], title, 0, y, width, callback, data);
    lv_obj_t *row=lv_obj_get_parent(title_label); settings_rows[index]=row;
    lv_obj_set_height(row,64); lv_obj_set_style_radius(row,32,0);
    lv_obj_align(title_label,LV_ALIGN_LEFT_MID,68,0);
    lv_obj_t *badge=lv_obj_create(row);
    lv_obj_set_size(badge,40,40); lv_obj_align(badge,LV_ALIGN_LEFT_MID,14,0);
    lv_obj_set_style_pad_all(badge,0,0); lv_obj_set_style_border_width(badge,0,0);
    lv_obj_set_style_radius(badge,LV_RADIUS_CIRCLE,0);
    lv_obj_set_style_bg_color(badge,lv_color_hex(accent),0);
    lv_obj_remove_flag(badge,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    if(image) {
        lv_obj_t *icon=lv_image_create(badge); lv_image_set_src(icon,image);
        lv_image_set_scale(icon,128); lv_obj_center(icon);
    } else {
        lv_obj_t *icon=lv_label_create(badge); set_label(icon,symbol); lv_obj_center(icon);
        lv_obj_set_style_text_font(icon,&lv_font_montserrat_20,0);
        lv_obj_set_style_text_color(icon,lv_color_hex(0x182C2D),0);
    }
    return row;
}

void settings_refresh(void)
{
    for(unsigned i=0; i<4; ++i) {
        lv_obj_set_style_bg_color(settings_rows[i],lv_color_hex(light_theme ? 0xFFFFFF : 0x152033),0);
        lv_obj_set_style_bg_color(settings_rows[i],lv_color_hex(light_theme ? 0xD4E3F0 : 0x344A64),LV_STATE_PRESSED);
        lv_obj_set_style_text_color(settings_rows[i],lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC),0);
        lv_obj_set_style_border_width(settings_rows[i],1,0);
        lv_obj_set_style_border_color(settings_rows[i],lv_color_hex(light_theme ? 0xCDD8E6 : 0x2A3B52),0);
    }
    set_label(format_button_label,twelve_hour ? "12 saat" : "24 saat");
    set_label(theme_button_label,light_theme ? "Acik" : "Koyu");
    if(light_theme) lv_obj_add_state(settings_switch,LV_STATE_CHECKED);
    else lv_obj_remove_state(settings_switch,LV_STATE_CHECKED);
}

void theme_apply(void)
{    for(unsigned i = 0; i < SCREEN_COUNT; ++i) {
        if(i == HOME || i == FACES || i == MENU || i == FLAPPY || i == SNAKE || i == IDA) continue;
        lv_obj_set_style_bg_color(screens[i], lv_color_hex(light_theme ? 0xEAF0F8 : 0x070D18), 0);
        lv_obj_set_style_text_color(screens[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
        if(page_titles[i]) lv_obj_set_style_text_color(page_titles[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
    }
    puzzle_refresh();
}

void settings_init(void)
{    page_header(SETTINGS, "Ayarlar", MENU);
    lv_obj_t *row=settings_row(0,"Saat ve tarih",NULL,&icon_clock,0x9CAFFF,106,300,navigate,(void *)(uintptr_t)CLOCK_EDIT);
    lv_obj_t *arrow=lv_label_create(row); set_label(arrow,LV_SYMBOL_RIGHT); lv_obj_align(arrow,LV_ALIGN_RIGHT_MID,-18,0);
    row=settings_row(1,"Saat bicimi","12",NULL,0x96E1C2,180,328,format_clicked,NULL);
    lv_obj_t *value=lv_obj_create(row); lv_obj_set_size(value,84,34); lv_obj_align(value,LV_ALIGN_RIGHT_MID,-16,0);
    lv_obj_set_style_bg_color(value,lv_color_hex(0x96E1C2),0); lv_obj_set_style_radius(value,17,0);
    lv_obj_set_style_border_width(value,0,0); lv_obj_set_style_pad_all(value,0,0);
    lv_obj_remove_flag(value,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    format_button_label=lv_label_create(value); lv_obj_center(format_button_label);
    lv_obj_set_style_text_font(format_button_label,&lv_font_montserrat_16,0);
    lv_obj_set_style_text_color(format_button_label,lv_color_hex(0x182C2D),0);
    row=settings_row(2,"Tema",LV_SYMBOL_EYE_OPEN,NULL,0x82D9F4,254,328,theme_clicked,NULL);
    theme_button_label=lv_label_create(row); lv_obj_align(theme_button_label,LV_ALIGN_RIGHT_MID,-74,0);
    lv_obj_set_style_text_font(theme_button_label,&lv_font_montserrat_16,0);
    settings_switch=lv_switch_create(row); lv_obj_set_size(settings_switch,46,26);
    lv_obj_align(settings_switch,LV_ALIGN_RIGHT_MID,-16,0);
    lv_obj_remove_flag(settings_switch,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_anim_duration(settings_switch,120,0);
    lv_obj_set_style_bg_color(settings_switch,lv_color_hex(0x435675),LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_switch,lv_color_hex(0x96E1C2),LV_PART_INDICATOR|LV_STATE_CHECKED);
    row=settings_row(3,"Saat arayuzleri",NULL,&icon_faces,0xD7AFF7,328,300,navigate,(void *)(uintptr_t)FACES);
    arrow=lv_label_create(row); set_label(arrow,LV_SYMBOL_RIGHT); lv_obj_align(arrow,LV_ALIGN_RIGHT_MID,-18,0);
    rtc_source_label = label_at(screens[SETTINGS], "RTC", 407, &lv_font_montserrat_14);
    clock_status_label = label_at(screens[SETTINGS], "Saat ayari gerekli", 429, &lv_font_montserrat_14);
    settings_refresh();
}

void watch_ui_set_rtc_source(bool crystal) { watch_app_vm_set_rtc_source(crystal); set_label(rtc_source_label,crystal ? "Saat: LSE kristal" : "Saat: LSI / zaman kayabilir"); }
void clock_status_refresh(void) { set_label(clock_status_label,ui_clock_valid ? "Saat ayarlandi" : "Saat ayari gerekli"); }
