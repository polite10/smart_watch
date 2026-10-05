#include "watch_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "watch_model.h"
#include "watch_faces.h"
#include "watch_clock.h"
#include "build_time.h"
#include "watch_icons.h"

enum { HOME, MENU, CALENDAR, SETTINGS, ALARMS, ALARM_EDIT, CALCULATOR,
       NOTES, NOTE_EDIT, FACES, WATER, WATER_SETTINGS, HISTORY, CLOCK_EDIT, SCREEN_COUNT };
static lv_obj_t *screens[SCREEN_COUNT];
static const lv_style_prop_t no_transition_properties[] = {0};
static const lv_style_transition_dsc_t no_transition = {.props = no_transition_properties};
static lv_obj_t *calendar, *format_button_label, *theme_button_label;
static lv_obj_t *settings_rows[4], *settings_switch;
static lv_obj_t *day_label, *rtc_source_label;
static lv_obj_t *calendar_month_label, *calendar_labels[42], *calendar_arrows[2];
static uint8_t calendar_numbers[42], calendar_month = 10, calendar_selected;
static uint16_t calendar_year = 2026;
static void calendar_init(void);
static void calendar_refresh(void);
static void settings_refresh(void);
static lv_obj_t *clock_day, *clock_month, *clock_year, *clock_hour, *clock_minute;
static lv_obj_t *clock_hint, *clock_result, *clock_status_label;
static bool ui_clock_valid;
static bool display_awake = true;
static lv_obj_t *page_titles[SCREEN_COUNT];
static unsigned back_targets[SCREEN_COUNT];
static void page_header(unsigned screen, const char *title, unsigned back);
static void clock_editor_init(void);
static void clock_editor_open(void);
static bool twelve_hour, light_theme;
static void apps_init(void);
static void apps_refresh(void);
static void notifications_refresh(void);
static void save_data(void);
static void refresh_time(void);
static uint16_t current_year = 2026;
static uint8_t current_month = 10, current_day = 4, current_hour, current_minute, current_second;
static void set_label(lv_obj_t *label, const char *value)
{
    /* An unchanged label must not allocate text or invalidate the display. */
    if(strcmp(lv_label_get_text(label), value)) lv_label_set_text(label, value);
}
static void screen_loaded(lv_event_t *e)
{
    (void)e;
    refresh_time();
    apps_refresh();
}
static lv_obj_t *label_at(lv_obj_t *parent, const char *text, int y, const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    set_label(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
    return l;
}
static void navigate(lv_event_t *e)
{
    unsigned screen = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(screen == FACES) watch_faces_open();
    else if(screen == CLOCK_EDIT) clock_editor_open();
    else lv_screen_load(screens[screen]);
}
static lv_obj_t *button_at(lv_obj_t *parent, const char *text, int x, int y,
                           int width, lv_event_cb_t callback, void *data)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, width, 60);
    lv_obj_align(b, LV_ALIGN_TOP_MID, x, y);
    lv_obj_set_style_radius(b, 30, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x26344C), 0);
    lv_obj_set_style_text_color(b, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_style_text_font(b, &lv_font_montserrat_20, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_style_transition(b, &no_transition, LV_STATE_DEFAULT);
    lv_obj_set_style_transition(b, &no_transition, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x435675), LV_STATE_PRESSED);
    lv_obj_t *l = lv_label_create(b);
    set_label(l, text);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, callback, LV_EVENT_CLICKED, data);
    return l;
}
static lv_obj_t *round_action(lv_obj_t *parent, const char *text, int x, int y,
                             int diameter, uint32_t color, lv_event_cb_t callback, void *data)
{
    lv_obj_t *label = button_at(parent, text, x, y, diameter, callback, data);
    lv_obj_t *button = lv_obj_get_parent(label);
    lv_obj_set_height(button, diameter);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    return label;
}
static void style_roller(lv_obj_t *roller)
{
    lv_obj_set_style_radius(roller, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x152033), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xD9E4F6), LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x96E1C2), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_hex(0x182C2D), LV_PART_SELECTED);
}
void watch_ui_header(lv_obj_t *page, const char *title, lv_event_cb_t back, void *data)
{
    (void)back;
    lv_obj_t *label = label_at(page, title, 44, &lv_font_montserrat_24);
    lv_obj_set_width(label, 290);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
    for(unsigned i=0; i<SCREEN_COUNT; ++i) if(screens[i] == page) {
        page_titles[i] = label;
        back_targets[i] = (unsigned)(uintptr_t)data;
    }
}
static void refresh_time(void)
{
    watch_faces_set_datetime(current_year, current_month, current_day,
                             current_hour, current_minute, current_second, twelve_hour);
}
static void format_clicked(lv_event_t *e)
{
    (void)e;
    twelve_hour = !twelve_hour;
    settings_refresh();
    refresh_time();
    watch_data.twelve_hour = twelve_hour;
    save_data();
}
static void theme_clicked(lv_event_t *e)
{
    (void)e;
    light_theme = !light_theme;
    for(unsigned i = 0; i < SCREEN_COUNT; ++i) {
        if(i == HOME || i == FACES) continue;
        lv_obj_set_style_bg_color(screens[i], lv_color_hex(light_theme ? 0xEAF0F8 : 0x070D18), 0);
        lv_obj_set_style_text_color(screens[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
        if(page_titles[i]) lv_obj_set_style_text_color(page_titles[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
    }
    settings_refresh();
    calendar_refresh();
    watch_data.light_theme = light_theme;
    watch_faces_apply_saved();
    save_data();
}
static lv_obj_t *settings_row(unsigned index, const char *title, const char *symbol,
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
static void settings_refresh(void)
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
static void selected_date(lv_event_t *e)
{
    unsigned i = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(i >= 42 || !calendar_numbers[i]) return;
    calendar_selected = calendar_numbers[i];
    char text[32];
    snprintf(text,sizeof text,"%02u.%02u.%u",calendar_selected,calendar_month,calendar_year);
    set_label(day_label,text);
    calendar_refresh();
}
static void calendar_refresh(void)
{
    lv_obj_set_style_text_color(calendar,lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC),0);
    static const char *months[] = {"Ocak","Subat","Mart","Nisan","Mayis","Haziran",
        "Temmuz","Agustos","Eylul","Ekim","Kasim","Aralik"};
    char value[32];
    snprintf(value,sizeof value,"%s %u",months[calendar_month-1],calendar_year);
    set_label(calendar_month_label,value);
    unsigned first=(watch_epoch(calendar_year,calendar_month,1,0,0,0)/86400U+5U)%7U;
    unsigned days=watch_days_in_month(calendar_year,calendar_month);
    for(unsigned i=0; i<42; ++i) {
        lv_obj_t *button=lv_obj_get_parent(calendar_labels[i]);
        bool valid=i>=first && i<first+days;
        calendar_numbers[i]=valid ? i-first+1 : 0;
        if(valid) {
            snprintf(value,sizeof value,"%u",calendar_numbers[i]);
            set_label(calendar_labels[i],value);
            lv_obj_remove_state(button,LV_STATE_DISABLED);
        } else { set_label(calendar_labels[i],""); lv_obj_add_state(button,LV_STATE_DISABLED); }
        bool today=valid && ui_clock_valid && calendar_year==current_year &&
            calendar_month==current_month && calendar_numbers[i]==current_day;
        bool selected=valid && calendar_numbers[i]==calendar_selected;
        lv_obj_set_style_bg_opa(button,valid ? LV_OPA_COVER : LV_OPA_TRANSP,0);
        lv_obj_set_style_bg_color(button,lv_color_hex(selected ? 0x86CDB8 : 0x26344C),0);
        lv_obj_set_style_text_color(calendar_labels[i],lv_color_hex(selected ? 0x152C2C : 0xF0F4FC),0);
        lv_obj_set_style_border_width(button,today ? 2 : 0,0);
        lv_obj_set_style_border_color(button,lv_color_hex(0x82D9F4),0);
    }
    for(unsigned i=0; i<2; ++i) {
        bool disabled=i==0 ? calendar_year==2000 && calendar_month==1 : calendar_year==2099 && calendar_month==12;
        if(disabled) lv_obj_add_state(calendar_arrows[i],LV_STATE_DISABLED);
        else lv_obj_remove_state(calendar_arrows[i],LV_STATE_DISABLED);
    }
}
static void calendar_move(lv_event_t *e)
{
    bool next=(uintptr_t)lv_event_get_user_data(e)!=0;
    if(next) {
        if(calendar_year==2099 && calendar_month==12) return;
        if(++calendar_month==13) { calendar_month=1; ++calendar_year; }
    } else {
        if(calendar_year==2000 && calendar_month==1) return;
        if(--calendar_month==0) { calendar_month=12; --calendar_year; }
    }
    calendar_selected=0; set_label(day_label,"Gun secin"); calendar_refresh();
}
static void calendar_init(void)
{
    calendar=lv_obj_create(screens[CALENDAR]);
    lv_obj_set_size(calendar,480,480); lv_obj_set_pos(calendar,0,0);
    lv_obj_set_style_pad_all(calendar,0,0); lv_obj_set_style_border_width(calendar,0,0);
    lv_obj_set_style_bg_opa(calendar,LV_OPA_TRANSP,0);
    lv_obj_remove_flag(calendar,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    calendar_year=current_year; calendar_month=current_month;
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
void watch_ui_init(void)
{
    watch_model_init();
    twelve_hour = watch_data.twelve_hour;
    light_theme = watch_data.light_theme;
    for(unsigned i = 0; i < SCREEN_COUNT; ++i) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_remove_flag(screens[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(screens[i], lv_color_hex(light_theme ? 0xEAF0F8 : 0x070D18), 0);
        lv_obj_set_style_text_color(screens[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
        lv_obj_set_style_text_font(screens[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_pad_all(screens[i], 0, 0);
        lv_obj_set_style_border_width(screens[i], 0, 0);
        lv_obj_add_event_cb(screens[i], screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    }
    watch_faces_init(screens[HOME], screens[FACES], navigate, (void *)(uintptr_t)MENU);
    page_header(MENU, "Uygulamalar", HOME);
    const unsigned targets[] = {HOME, ALARMS, CALCULATOR, NOTES, FACES, WATER, CALENDAR, SETTINGS};
    const lv_image_dsc_t *icons[] = {&icon_clock, &icon_alarm, &icon_calculator, &icon_notes,
                                   &icon_faces, &icon_water, &icon_calendar, &icon_settings};
    const uint32_t colors[] = {0x9CAFFF,0xFF9B9E,0x96E1C2,0xE5C191,0xD7AFF7,0x82D9F4,0xFFC5A2,0xADC4D6};
    const int xs[] = {-106,0,106,-53,53,-106,0,106};
    const int ys[] = {139,105,139,223,223,307,341,307};
    for(unsigned i = 0; i < 8; ++i) {
        lv_obj_t *label = round_action(screens[MENU], "", xs[i], ys[i], 92, colors[i], navigate, (void *)(uintptr_t)targets[i]);
        lv_obj_t *image = lv_image_create(lv_obj_get_parent(label));
        lv_image_set_src(image, icons[i]); lv_obj_center(image);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);
    }

    page_header(CALENDAR, "Takvim", MENU);
    calendar_init();
    page_header(SETTINGS, "Ayarlar", MENU);
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
    lv_obj_set_style_anim_duration(settings_switch,0,0);
    lv_obj_set_style_bg_color(settings_switch,lv_color_hex(0x435675),LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_switch,lv_color_hex(0x96E1C2),LV_PART_INDICATOR|LV_STATE_CHECKED);
    row=settings_row(3,"Saat arayuzleri",NULL,&icon_faces,0xD7AFF7,328,300,navigate,(void *)(uintptr_t)FACES);
    arrow=lv_label_create(row); set_label(arrow,LV_SYMBOL_RIGHT); lv_obj_align(arrow,LV_ALIGN_RIGHT_MID,-18,0);
    rtc_source_label = label_at(screens[SETTINGS], "RTC", 407, &lv_font_montserrat_14);
    clock_status_label = label_at(screens[SETTINGS], "Saat ayari gerekli", 429, &lv_font_montserrat_14);
    settings_refresh();
    apps_init();
    clock_editor_init();
    refresh_time();
    lv_screen_load(screens[0]);
}
void watch_ui_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second)
{
    if(!ui_clock_valid || !watch_datetime_valid(year,month,day,hour,minute,second)) return;
    current_year = year; current_month = month; current_day = day;
    current_hour = hour; current_minute = minute; current_second = second;
    refresh_time();
    static uint32_t last_calendar_day;
    uint32_t today = watch_day_key(year,month,day);
    if(today != last_calendar_day) { calendar_refresh(); last_calendar_day = today; }
    if(watch_model_tick(watch_day_key(year, month, day), watch_epoch(year, month, day, hour, minute, second), hour, minute)) save_data();
    if(second == 0) apps_refresh();
    notifications_refresh();
}
void watch_ui_set_battery(uint8_t percent, bool valid)
{
    watch_faces_set_battery(percent, valid);
}
void watch_ui_set_rtc_source(bool crystal)
{
    set_label(rtc_source_label, crystal ? "Saat: LSE kristal" : "Saat: LSI / zaman kayabilir");
}

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
    (void)e;
    unsigned selected=lv_roller_get_selected(clock_day);
    unsigned days=watch_days_in_month(2000+lv_roller_get_selected(clock_year),1+lv_roller_get_selected(clock_month));
    clock_options(clock_day,1,days,false);
    lv_roller_set_selected(clock_day,selected<days ? selected : days-1,LV_ANIM_OFF);
    set_label(clock_result, "Kaydet: saniye 00 olur.");
}
static void clock_editor_open(void)
{
    uint16_t year=ui_clock_valid ? current_year : BUILD_YEAR;
    uint8_t month=ui_clock_valid ? current_month : BUILD_MONTH;
    uint8_t day=ui_clock_valid ? current_day : BUILD_DAY;
    lv_roller_set_selected(clock_year,year-2000,LV_ANIM_OFF);
    lv_roller_set_selected(clock_month,month-1,LV_ANIM_OFF);
    clock_date_changed(NULL);
    lv_roller_set_selected(clock_day,day-1,LV_ANIM_OFF);
    lv_roller_set_selected(clock_hour,ui_clock_valid ? current_hour : 0,LV_ANIM_OFF);
    lv_roller_set_selected(clock_minute,ui_clock_valid ? current_minute : 0,LV_ANIM_OFF);
    set_label(clock_hint, ui_clock_valid ? "Telefonunuzdaki saat ve tarihi girin.\nSaat bicimi: 24 saat" : "Saat bilinmiyor. Telefonunuza bakin.\nPrizden saat bilgisi alinmaz.");
    set_label(clock_result, "Kaydet: saniye 00 olur.");
    lv_screen_load(screens[CLOCK_EDIT]);
}
static void clock_save(lv_event_t *e)
{
    (void)e;
    if(!watch_clock_set_datetime(2000+lv_roller_get_selected(clock_year),1+lv_roller_get_selected(clock_month),
        1+lv_roller_get_selected(clock_day),lv_roller_get_selected(clock_hour),lv_roller_get_selected(clock_minute),0)) {
        set_label(clock_result,"Saat kaydedilemedi. Tekrar deneyin.");
        return;
    }
    lv_screen_load(screens[HOME]);
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
static void clock_editor_init(void)
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

static lv_obj_t *storage_notice, *alarm_labels[WATCH_ALARMS], *alarm_toggle_labels[WATCH_ALARMS];
static lv_obj_t *alarm_repeats[WATCH_ALARMS], *alarm_switches[WATCH_ALARMS];
static lv_obj_t *alarm_hour, *alarm_minute, *alarm_repeat_label;
static lv_obj_t *calc_label, *note_labels[WATCH_NOTES], *note_text, *note_hint, *note_delete_label;
static lv_obj_t *note_keys[26], *note_mode_label, *note_case_label;
static unsigned note_mode;
static bool note_uppercase;
static const char *note_characters[] = {"abcdefghijklmnopqrstuvwxyz", "0123456789.,!?;:-_+*/=()@#", "\n'"};
static void note_keyboard_refresh(void)
{
    const char *characters = note_characters[note_mode];
    size_t count = strlen(characters);
    for(unsigned i=0; i<26; ++i) {
        char c = i<count ? characters[i] : 0;
        if(note_uppercase && c>='a' && c<='z') c -= 'a'-'A';
        char value[2] = {c,0};
        set_label(note_keys[i],c == '\n' ? LV_SYMBOL_NEW_LINE : value);
        lv_obj_t *button=lv_obj_get_parent(note_keys[i]);
        if(c) lv_obj_remove_state(button,LV_STATE_DISABLED);
        else lv_obj_add_state(button,LV_STATE_DISABLED);
    }
    set_label(note_mode_label,note_mode == 0 ? "123" : note_mode == 1 ? "#+=" : "ABC");
    set_label(note_case_label,note_uppercase ? "a" : "A");
}
static void note_key(lv_event_t *e)
{
    unsigned i = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(i == 27) lv_textarea_delete_char(note_text);
    else if(i == 26) lv_textarea_add_char(note_text,' ');
    else {
        const char *characters=note_characters[note_mode];
        if(i>=strlen(characters)) return;
        char c=characters[i];
        if(note_uppercase && c>='a' && c<='z') c -= 'a'-'A';
        lv_textarea_add_char(note_text,c);
    }
}
static void note_next_mode(lv_event_t *e)
{
    (void)e; note_mode=(note_mode+1)%3; note_keyboard_refresh();
}
static void note_case(lv_event_t *e)
{
    (void)e; note_uppercase=!note_uppercase; note_keyboard_refresh();
}
static lv_obj_t *water_ring, *water_total, *water_remaining, *water_goal_label, *water_reminder_label;
static lv_obj_t *history_labels[WATCH_HISTORY];
static lv_obj_t *notification, *notification_title, *notification_text, *notification_first, *notification_second;
static unsigned editing_alarm, editing_note;
static bool edit_daily, confirm_delete, calc_result;
static char calc_expression[48];
static int displayed_notification = -2; /* -2 none, -1 water, >=0 alarm */

void watch_ui_touch(lv_indev_data_t *data, uint32_t now)
{
    static bool down, consumed, home_contact, moved, pending, edge_contact;
    static unsigned contact_screen;
    static lv_point_t origin, last;
    static uint32_t started, released;
    bool pressed = data->state == LV_INDEV_STATE_PRESSED;
    /* Notification controls keep their usual taps and cannot be dismissed by locking. */
    if(displayed_notification != -2) {
        down = pending = consumed = false;
        if(!display_awake) { display_awake = true; watch_ui_display_power(true); }
        return;
    }
    if(pending && pressed && !down) {
        int dx = data->point.x-last.x, dy = data->point.y-last.y;
        if(now-released <= 260 && dx*dx+dy*dy <= 48*48) {
            /* First release is still withheld: neither tap activates an app control. */
            pending = false; consumed = true; down = true;
            lv_indev_reset(lv_indev_active(), NULL);
            lv_screen_load(screens[HOME]);
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        /* Deliver the first release before accepting a separate contact. */
        pending = false;
        data->point = last; data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    if(pressed && !down) {
        origin = last = data->point; started = now;
        down = true; moved = false;
        bool waking = !display_awake;
        home_contact = !waking && lv_screen_active() == screens[HOME];
        consumed = waking || home_contact;
        contact_screen = SCREEN_COUNT;
        for(unsigned i=0; i<SCREEN_COUNT; ++i) if(lv_screen_active() == screens[i]) contact_screen = i;
        edge_contact = !consumed && contact_screen < SCREEN_COUNT && origin.x <= 64;
        if(!display_awake) { display_awake = true; watch_ui_display_power(true); }
    }
    if(pressed) {
        last = data->point;
        int dx = last.x-origin.x, dy = last.y-origin.y;
        if(edge_contact && !consumed && dx >= 64 && dy*dy <= dx*dx/2) {
            consumed = moved = true; pending = false; edge_contact = false;
            lv_indev_reset(lv_indev_active(),NULL);
            lv_screen_load(screens[back_targets[contact_screen]]);
        }
        if(dx*dx+dy*dy >= 24*24) {
            if(!moved && home_contact && lv_screen_active() == screens[HOME])
                lv_screen_load(screens[MENU]);
            moved = true;
        }
        if(consumed) data->state = LV_INDEV_STATE_RELEASED;
    } else if(down) {
        down = false;
        if(consumed) {
            if(home_contact && !moved && lv_screen_active() == screens[HOME] && now-started < 500) {
                display_awake = false; watch_ui_display_power(false);
            }
            consumed = home_contact = false;
        } else if(!moved && now-started < 200) {
            pending = true; released = now;
            data->point = last; data->state = LV_INDEV_STATE_PRESSED;
        }
    } else if(pending) {
        data->point = last;
        if(now-released < 260) data->state = LV_INDEV_STATE_PRESSED;
        else pending = false;
    }
}

void watch_ui_set_clock_valid(bool valid)
{
    bool lost=ui_clock_valid && !valid;
    ui_clock_valid=valid;
    calendar_refresh();
    watch_faces_set_clock_valid(valid);
    set_label(clock_status_label, valid ? "Saat ayarlandi" : "Saat ayari gerekli");
    if(!valid) {
        calendar_refresh();
        lv_obj_add_flag(notification,LV_OBJ_FLAG_HIDDEN);
        displayed_notification=-2;
        if(lost || lv_screen_active()==screens[HOME]) clock_editor_open();
    }
    apps_refresh();
}

static void save_data(void)
{
    bool ok = watch_model_save();
    if(storage_notice) {
        if(ok) lv_obj_add_flag(storage_notice, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(storage_notice, LV_OBJ_FLAG_HIDDEN);
    }
}
void watch_ui_datetime_changed(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hour, uint8_t minute, uint8_t second)
{
    watch_model_time_changed(watch_day_key(year,month,day),
        watch_epoch(year,month,day,hour,minute,second),hour,minute);
    watch_ui_set_clock_valid(true);
    watch_ui_set_datetime(year,month,day,hour,minute,second);
    calendar_year = year; calendar_month = month; calendar_selected = 0; calendar_refresh();
    save_data();
}
static void page_header(unsigned screen, const char *title, unsigned back)
{
    watch_ui_header(screens[screen], title, navigate, (void *)(uintptr_t)back);

}
static void alarm_toggle(lv_event_t *e)
{
    unsigned i = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    watch_data.alarms[i].enabled = !watch_data.alarms[i].enabled;
    watch_alarm_changed(i);
    save_data(); apps_refresh();
}
static void alarm_edit(lv_event_t *e)
{
    editing_alarm = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    watch_alarm_t *a = &watch_data.alarms[editing_alarm];
    lv_roller_set_selected(alarm_hour, a->hour, LV_ANIM_OFF);
    lv_roller_set_selected(alarm_minute, a->minute, LV_ANIM_OFF);
    edit_daily = a->daily;
    set_label(alarm_repeat_label, edit_daily ? "Tekrar: Her gun" : "Tekrar: Bir kez");
    lv_screen_load(screens[ALARM_EDIT]);
}
static void alarm_repeat(lv_event_t *e)
{
    (void)e; edit_daily = !edit_daily;
    set_label(alarm_repeat_label, edit_daily ? "Tekrar: Her gun" : "Tekrar: Bir kez");
}
static void alarm_save(lv_event_t *e)
{
    (void)e;
    watch_alarm_t *a = &watch_data.alarms[editing_alarm];
    a->hour = lv_roller_get_selected(alarm_hour);
    a->minute = lv_roller_get_selected(alarm_minute);
    a->daily = edit_daily; a->enabled = 1;
    watch_alarm_changed(editing_alarm);
    save_data(); apps_refresh(); lv_screen_load(screens[ALARMS]);
}
static void calculator_key(lv_event_t *e)
{
    const char *key = lv_event_get_user_data(e);
    if(!key) return;
    size_t len = strlen(calc_expression);
    if(!strcmp(key, "C")) { calc_expression[0] = '\0'; calc_result = false; }
    else if(!strcmp(key, "DEL")) { if(len) calc_expression[len - 1] = '\0'; calc_result = false; }
    else if(!strcmp(key, "=")) {
        double value;
        if(watch_calculate(calc_expression, &value)) {
            /* Plain decimal output can be used for the next calculation. */
            snprintf(calc_expression, sizeof calc_expression, "%.6f", value);
            char *dot = strchr(calc_expression, '.');
            if(dot) {
                char *tail = calc_expression + strlen(calc_expression) - 1;
                while(tail > dot && *tail == '0') *tail-- = '\0';
                if(tail == dot) *tail = '\0';
            }
            calc_result = true;
        } else {
            lv_obj_set_style_text_font(calc_label, &lv_font_montserrat_20, 0);
            set_label(calc_label, "Gecersiz islem");
            calc_expression[0] = '\0'; calc_result = false;
            return;
        }
    } else {
        if(calc_result && (key[0] == '.' || (key[0] >= '0' && key[0] <= '9'))) {
            calc_expression[0] = '\0'; len = 0;
        }
        calc_result = false;
        if(len + strlen(key) < sizeof calc_expression) strcat(calc_expression, key);
    }
    set_label(calc_label, calc_expression[0] ? calc_expression : "0");
    lv_obj_set_style_text_font(calc_label, strlen(calc_expression)>10 ? &lv_font_montserrat_20 : &lv_font_montserrat_32, 0);
}
static void note_edit(lv_event_t *e)
{
    editing_note = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    lv_textarea_set_text(note_text, watch_data.notes[editing_note]);
    confirm_delete = false;
    set_label(note_delete_label, LV_SYMBOL_TRASH);
    set_label(note_hint, "En fazla 191 karakter");
    note_mode = 0; note_uppercase = false; note_keyboard_refresh();
    lv_screen_load(screens[NOTE_EDIT]);
}
static void note_save(lv_event_t *e)
{
    (void)e;
    snprintf(watch_data.notes[editing_note], WATCH_NOTE_SIZE, "%s", lv_textarea_get_text(note_text));
    save_data(); apps_refresh(); lv_screen_load(screens[NOTES]);
}
static void note_delete(lv_event_t *e)
{
    (void)e;
    if(!confirm_delete) {
        confirm_delete = true;
        set_label(note_delete_label, "?");
        set_label(note_hint, "Silmek icin tekrar basin");
        return;
    }
    watch_data.notes[editing_note][0] = '\0';
    save_data(); apps_refresh(); lv_screen_load(screens[NOTES]);
}
static void note_changed(lv_event_t *e)
{
    (void)e; confirm_delete = false;
    set_label(note_delete_label, LV_SYMBOL_TRASH);
    set_label(note_hint, "En fazla 191 karakter");
}
static void water_add(lv_event_t *e)
{
    if(!ui_clock_valid) { clock_editor_open(); return; }
    watch_water_add((int)(intptr_t)lv_event_get_user_data(e));
    save_data(); apps_refresh(); notifications_refresh();
}
static void water_goal(lv_event_t *e)
{
    if(!ui_clock_valid) { clock_editor_open(); return; }
    watch_water_goal((int)(intptr_t)lv_event_get_user_data(e));
    save_data(); apps_refresh();
}
static void water_reminder(lv_event_t *e)
{
    (void)e;
    uint16_t m = watch_data.reminder_minutes;
    watch_reminder_set(m == 0 ? 30 : m == 30 ? 60 : m == 60 ? 120 : 0);
    save_data(); apps_refresh(); notifications_refresh();
}
static void notification_action(lv_event_t *e)
{
    bool first = (uintptr_t)lv_event_get_user_data(e) == 0;
    if(displayed_notification >= 0) watch_alarm_ack(displayed_notification, first);
    else if(displayed_notification == -1) {
        if(first) { watch_water_add(1); save_data(); apps_refresh(); }
        else watch_reminder_set(watch_data.reminder_minutes);
    }
    notifications_refresh();
}
static void notifications_refresh(void)
{
    int selected = ui_clock_valid ? watch_alarm_next() : -2;
    if(!ui_clock_valid) {
        lv_obj_add_flag(notification,LV_OBJ_FLAG_HIDDEN); displayed_notification=-2; return;
    }
    if(selected < 0) selected = watch_water_reminder_due() ? -1 : -2;
    if(selected == -2) {
        lv_obj_add_flag(notification, LV_OBJ_FLAG_HIDDEN);
        displayed_notification = -2;
        return;
    }
    if(selected != displayed_notification) {
        if(!display_awake) { display_awake = true; watch_ui_display_power(true); }
        if(selected >= 0) {
            char text[80];
            snprintf(text, sizeof text, "Alarm %u - %02u:%02u\nEkran ve LED uyarisi", selected + 1,
                     watch_data.alarms[selected].hour, watch_data.alarms[selected].minute);
            set_label(notification_title, "ALARM");
            set_label(notification_text, text);
            set_label(notification_first, "5 dk ertele");
            set_label(notification_second, "Durdur");
        } else {
            set_label(notification_title, "SU ZAMANI");
            set_label(notification_text, "Bir bardak su ictiniz mi?\n1 bardak = 200 ml");
            set_label(notification_first, "+200 ml");
            set_label(notification_second, "Daha sonra");
        }
        displayed_notification = selected;
    }
    lv_obj_remove_flag(notification, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(notification);
}
static void apps_refresh(void)
{
    watch_faces_refresh();
    char text[96];
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        watch_alarm_t *a = &watch_data.alarms[i];
        snprintf(text, sizeof text, "%02u:%02u", a->hour, a->minute);
        set_label(alarm_labels[i], text);
        snprintf(text,sizeof text,"%u / %s",i+1,a->daily ? "Her gun" : "Bir kez");
        set_label(alarm_repeats[i],text);
        set_label(alarm_toggle_labels[i], a->enabled ? "Acik" : "Kapali");
        lv_obj_t *card=lv_obj_get_parent(alarm_labels[i]);
        lv_obj_set_style_bg_color(card,lv_color_hex(light_theme ? 0xFFFFFF : 0x152033),0);
        lv_obj_set_style_text_color(card,lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC),0);
        lv_obj_set_style_text_color(alarm_repeats[i],lv_color_hex(light_theme ? 0x586C83 : 0xA8B9D2),0);
        lv_obj_set_style_border_color(card,lv_color_hex(a->enabled ? 0x66BBA1 : light_theme ? 0xCDD8E6 : 0x2A3B52),0);
        lv_obj_set_style_bg_color(lv_obj_get_parent(alarm_toggle_labels[i]), lv_color_hex(a->enabled ? 0x1E685F : 0x253147), 0);
        if(a->enabled) lv_obj_add_state(alarm_switches[i],LV_STATE_CHECKED);
        else lv_obj_remove_state(alarm_switches[i],LV_STATE_CHECKED);
    }
    for(unsigned i = 0; i < WATCH_NOTES; ++i) {
        char preview[23];
        snprintf(preview, sizeof preview, "%.22s", watch_data.notes[i]);
        for(char *p = preview; *p; ++p) if(*p == '\n') *p = ' ';
        snprintf(text, sizeof text, "%u. %s", i + 1, preview[0] ? preview : "Yeni not");
        set_label(note_labels[i], text);
    }
    unsigned ml = watch_data.water[0].ml, goal = watch_data.water_goal;
    lv_arc_set_value(water_ring, !ui_clock_valid ? 0 : ml >= goal ? 100 : ml * 100 / goal);
    if(ui_clock_valid) snprintf(text, sizeof text, "%u ml", ml);
    else snprintf(text, sizeof text, "-- ml");
    set_label(water_total, text);
    if(!ui_clock_valid) snprintf(text, sizeof text, "Once saat ve tarihi ayarlayin");
    else snprintf(text, sizeof text, ml >= goal ? "Hedef tamamlandi" : "Kalan: %u ml", ml >= goal ? 0 : goal - ml);
    set_label(water_remaining, text);
    snprintf(text, sizeof text, "Gunluk hedef: %u ml", goal); set_label(water_goal_label, text);
    if(watch_data.reminder_minutes) snprintf(text, sizeof text, "Hatirlatma: %u dk", watch_data.reminder_minutes);
    else snprintf(text, sizeof text, "Hatirlatma: Kapali");
    set_label(water_reminder_label, text);
    for(unsigned i = 0; i < WATCH_HISTORY; ++i) {
        watch_water_day_t *d = &watch_data.water[i];
        if(d->day) snprintf(text, sizeof text, "%02u.%02u.%04u  %u / %u ml", d->day % 100, d->day / 100 % 100, d->day / 10000, d->ml, d->goal);
        else snprintf(text, sizeof text, "--");
        set_label(history_labels[i], text);
    }
}
static void apps_init(void)
{
    storage_notice = label_at(lv_layer_top(), "Kayit basarisiz. Tekrar deneyin.", 405, &lv_font_montserrat_14);
    lv_obj_set_width(storage_notice,260);
    lv_obj_set_style_text_align(storage_notice,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(storage_notice, lv_color_hex(0xFF596F), 0);
    lv_obj_add_flag(storage_notice, LV_OBJ_FLAG_HIDDEN);
    page_header(ALARMS, "Alarmlar", MENU);
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        int width=i==1 ? 332 : 308;
        alarm_labels[i]=button_at(screens[ALARMS],"",0,112+i*104,width,alarm_edit,(void *)(uintptr_t)i);
        lv_obj_t *card=lv_obj_get_parent(alarm_labels[i]); lv_obj_set_height(card,88);
        lv_obj_set_style_radius(card,32,0); lv_obj_set_style_border_width(card,1,0);
        lv_obj_set_style_text_font(alarm_labels[i],&lv_font_montserrat_32,0);
        lv_obj_align(alarm_labels[i],LV_ALIGN_LEFT_MID,22,-12);
        alarm_repeats[i]=lv_label_create(card);
        lv_obj_set_style_text_font(alarm_repeats[i],&lv_font_montserrat_14,0);
        lv_obj_align(alarm_repeats[i],LV_ALIGN_BOTTOM_LEFT,24,-12);
        alarm_toggle_labels[i]=button_at(card,"",width/2-52,16,76,alarm_toggle,(void *)(uintptr_t)i);
        lv_obj_t *toggle=lv_obj_get_parent(alarm_toggle_labels[i]); lv_obj_set_height(toggle,56);
        lv_obj_set_style_radius(toggle,24,0); lv_obj_set_style_text_font(alarm_toggle_labels[i],&lv_font_montserrat_14,0);
        lv_obj_align(alarm_toggle_labels[i],LV_ALIGN_BOTTOM_MID,0,-7);
        alarm_switches[i]=lv_switch_create(toggle); lv_obj_set_size(alarm_switches[i],36,18);
        lv_obj_align(alarm_switches[i],LV_ALIGN_TOP_MID,0,8);
        lv_obj_remove_flag(alarm_switches[i],LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_anim_duration(alarm_switches[i],0,0);
        lv_obj_set_style_bg_color(alarm_switches[i],lv_color_hex(0x96E1C2),LV_PART_INDICATOR|LV_STATE_CHECKED);
    }
    label_at(screens[ALARMS], "Saate dokunarak duzenleyin", 429, &lv_font_montserrat_14);
    page_header(ALARM_EDIT, "Alarm ayari", ALARMS);
    char options[180] = {0};
    for(unsigned i = 0; i < 24; ++i) { char n[5]; snprintf(n, sizeof n, i == 23 ? "%02u" : "%02u\n", i); strcat(options, n); }
    alarm_hour = lv_roller_create(screens[ALARM_EDIT]);
    style_roller(alarm_hour);
    lv_roller_set_options(alarm_hour, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_hour, 3); lv_obj_set_width(alarm_hour, 112);
    lv_obj_align(alarm_hour, LV_ALIGN_TOP_MID, -68, 112);
    options[0] = '\0';
    for(unsigned i = 0; i < 60; ++i) { char n[5]; snprintf(n, sizeof n, i == 59 ? "%02u" : "%02u\n", i); strcat(options, n); }
    alarm_minute = lv_roller_create(screens[ALARM_EDIT]);
    style_roller(alarm_minute);
    lv_roller_set_options(alarm_minute, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_minute, 3); lv_obj_set_width(alarm_minute, 112);
    lv_obj_align(alarm_minute, LV_ALIGN_TOP_MID, 68, 112);
    lv_obj_set_style_text_font(alarm_hour, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_font(alarm_hour, &lv_font_montserrat_28, LV_PART_SELECTED);
    lv_obj_set_style_text_font(alarm_minute, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_font(alarm_minute, &lv_font_montserrat_28, LV_PART_SELECTED);
    label_at(screens[ALARM_EDIT], ":", 150, &lv_font_montserrat_24);
    label_at(screens[ALARM_EDIT], "SAAT            DAKIKA", 242, &lv_font_montserrat_16);
    alarm_repeat_label = button_at(screens[ALARM_EDIT], "", 0, 277, 294, alarm_repeat, NULL);
    button_at(screens[ALARM_EDIT], "Kaydet", 0, 351, 260, alarm_save, NULL);

    page_header(CALCULATOR, "Hesap", MENU);
    calc_label = label_at(screens[CALCULATOR], "0", 96, &lv_font_montserrat_32);
    lv_obj_set_width(calc_label, 172); lv_label_set_long_mode(calc_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(calc_label, LV_TEXT_ALIGN_RIGHT, 0);
    round_action(screens[CALCULATOR], "C", -132, 86, 54, 0x885366, calculator_key, "C");
    round_action(screens[CALCULATOR], LV_SYMBOL_BACKSPACE, 132, 86, 54, 0x37465F, calculator_key, "DEL");
    static const char *keys[] = {"7","8","9","/","4","5","6","*","1","2","3","-","0",".","=","+"};
    for(unsigned i=0; i<16; ++i) {
        int x = -114 + (i%4)*76, y = 144 + (i/4)*76;
        bool operation = i%4 == 3 || i == 14;
        lv_obj_t *label = round_action(screens[CALCULATOR], keys[i], x, y, 68,
            operation ? 0x86CDB8 : 0x26344C, calculator_key, (void *)keys[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
        if(operation) lv_obj_set_style_text_color(label, lv_color_hex(0x152C2C), 0);
    }

    page_header(NOTES, "Notlar", MENU);
    for(unsigned i = 0; i < WATCH_NOTES; ++i)
        note_labels[i] = button_at(screens[NOTES], "", 0, 108 + i * 76, i == 0 || i == 3 ? 280 : 324, note_edit, (void *)(uintptr_t)i);
    page_header(NOTE_EDIT, "Not duzenle", NOTES);
    note_hint = label_at(screens[NOTE_EDIT], "En fazla 191 karakter", 77, &lv_font_montserrat_14);
    note_text = lv_textarea_create(screens[NOTE_EDIT]);
    lv_obj_set_size(note_text,282,52); lv_obj_align(note_text,LV_ALIGN_TOP_MID,-16,98);
    lv_obj_set_style_radius(note_text,26,0);
    lv_obj_set_style_bg_color(note_text,lv_color_hex(0x152033),0);
    lv_obj_set_style_text_color(note_text,lv_color_hex(0xF0F4FC),0);
    lv_obj_set_style_border_color(note_text,lv_color_hex(0x34465F),0);
    lv_obj_set_style_border_width(note_text,1,0);
    lv_obj_set_style_text_font(note_text,&lv_font_montserrat_20,0);
    lv_textarea_set_max_length(note_text,WATCH_NOTE_SIZE-1);
    lv_textarea_set_accepted_chars(note_text,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?;:-_+*/=()@#\n'");
    lv_obj_add_event_cb(note_text,note_changed,LV_EVENT_VALUE_CHANGED,NULL);
    note_delete_label=round_action(screens[NOTE_EDIT],LV_SYMBOL_TRASH,158,100,44,0x885366,note_delete,NULL);
    lv_obj_set_style_text_font(note_delete_label,&lv_font_montserrat_20,0);
    lv_obj_t *save=button_at(screens[NOTE_EDIT],"Kaydet",-126,160,96,note_save,NULL);
    lv_obj_set_height(lv_obj_get_parent(save),52);
    note_case_label=round_action(screens[NOTE_EDIT],"A",-47,160,50,0x37465F,note_case,NULL);
    note_mode_label=button_at(screens[NOTE_EDIT],"123",13,160,58,note_next_mode,NULL);
    lv_obj_set_height(lv_obj_get_parent(note_mode_label),52);
    lv_obj_t *space=button_at(screens[NOTE_EDIT],"Bosluk",83,160,70,note_key,(void *)(uintptr_t)26);
    lv_obj_set_height(lv_obj_get_parent(space),52); lv_obj_set_style_text_font(space,&lv_font_montserrat_14,0);
    round_action(screens[NOTE_EDIT],LV_SYMBOL_BACKSPACE,149,160,50,0x37465F,note_key,(void *)(uintptr_t)27);
    for(unsigned i=0; i<26; ++i) {
        unsigned row = i<6 ? 0 : i<13 ? 1 : i<20 ? 2 : 3;
        unsigned first[] = {0,6,13,20}, counts[] = {6,7,7,6};
        int col=i-first[row], x=col*52-((int)counts[row]-1)*26;
        int curve=(row==0 ? 1 : row==3 ? -1 : 0) * (x*x/2200);
        note_keys[i]=round_action(screens[NOTE_EDIT],"",x,218+row*54+curve,46,0x26344C,note_key,(void *)(uintptr_t)i);
        lv_obj_set_style_text_font(note_keys[i],&lv_font_montserrat_20,0);
    }
    note_keyboard_refresh();

    page_header(WATER, "Su takibi", MENU);
    water_ring = lv_arc_create(screens[WATER]); lv_obj_set_size(water_ring, 206, 206);
    lv_obj_align(water_ring, LV_ALIGN_TOP_MID, 0, 103);
    lv_arc_set_rotation(water_ring, 270); lv_arc_set_bg_angles(water_ring, 0, 360);
    lv_arc_set_range(water_ring, 0, 100);
    lv_obj_set_style_arc_width(water_ring, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(water_ring, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(water_ring, lv_color_hex(0x253147), LV_PART_MAIN);
    lv_obj_set_style_arc_color(water_ring, lv_color_hex(0x82D9F4), LV_PART_INDICATOR);
    lv_obj_remove_style(water_ring, NULL, LV_PART_KNOB); lv_obj_remove_flag(water_ring, LV_OBJ_FLAG_CLICKABLE);
    water_total = label_at(screens[WATER], "0 ml", 159, &lv_font_montserrat_32);
    label_at(screens[WATER], "BUGUN", 201, &lv_font_montserrat_16);
    water_remaining = label_at(screens[WATER], "", 235, &lv_font_montserrat_16);
    lv_obj_set_width(water_remaining, 174); lv_obj_set_style_text_align(water_remaining, LV_TEXT_ALIGN_CENTER, 0);
    round_action(screens[WATER], LV_SYMBOL_MINUS, -148, 175, 72, 0x37465F, water_add, (void *)(intptr_t)-1);
    round_action(screens[WATER], LV_SYMBOL_PLUS, 148, 167, 88, 0x288BAD, water_add, (void *)(intptr_t)1);
    label_at(screens[WATER], "1 bardak / 200 ml", 319, &lv_font_montserrat_20);
    button_at(screens[WATER], "Hedef", -76, 357, 140, navigate, (void *)(uintptr_t)WATER_SETTINGS);
    button_at(screens[WATER], "Gecmis", 76, 357, 140, navigate, (void *)(uintptr_t)HISTORY);
    page_header(WATER_SETTINGS, "Su ayarlari", WATER);
    water_goal_label = label_at(screens[WATER_SETTINGS], "", 119, &lv_font_montserrat_24);
    round_action(screens[WATER_SETTINGS], "-200", -78, 167, 96, 0x37465F, water_goal, (void *)(intptr_t)-200);
    round_action(screens[WATER_SETTINGS], "+200", 78, 167, 96, 0x288BAD, water_goal, (void *)(intptr_t)200);
    label_at(screens[WATER_SETTINGS], "1 bardak = 200 ml", 266, &lv_font_montserrat_16);
    water_reminder_label = button_at(screens[WATER_SETTINGS], "", 0, 302, 294, water_reminder, NULL);
    label_at(screens[WATER_SETTINGS], "Kapali / 30 / 60 / 120 dk", 378, &lv_font_montserrat_16);
    label_at(screens[WATER_SETTINGS], "08:00 - 22:00", 410, &lv_font_montserrat_16);
    page_header(HISTORY, "Su gecmisi", WATER);
    lv_obj_t *list = lv_obj_create(screens[HISTORY]);
    lv_obj_set_size(list, 320, 290); lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 106);
    lv_obj_set_style_radius(list,100,0);
    lv_obj_set_style_border_width(list,0,0);
    lv_obj_set_style_bg_opa(list,LV_OPA_TRANSP,0);
    lv_obj_set_style_text_color(list, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_style_pad_top(list,40,0);
    lv_obj_set_style_pad_bottom(list,40,0);
    for(unsigned i = 0; i < WATCH_HISTORY; ++i) {
        lv_obj_t *row=lv_obj_create(list);
        lv_obj_set_size(row,278,46); lv_obj_set_pos(row,0,i*54);
        lv_obj_set_style_radius(row,23,0); lv_obj_set_style_pad_all(row,0,0);
        lv_obj_set_style_border_width(row,0,0);
        lv_obj_set_style_bg_color(row,lv_color_hex(0x152033),0);
        lv_obj_set_style_text_color(row,lv_color_hex(0xF0F4FC),0);
        lv_obj_remove_flag(row,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
        history_labels[i] = lv_label_create(row);
        lv_obj_set_style_text_font(history_labels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_width(history_labels[i],256);
        lv_obj_set_style_text_align(history_labels[i],LV_TEXT_ALIGN_CENTER,0);
        lv_obj_center(history_labels[i]); set_label(history_labels[i], "--");
    }
    notification = lv_obj_create(lv_layer_top());
    lv_obj_set_size(notification, 480, 480); lv_obj_set_pos(notification, 0, 0);
    lv_obj_set_style_bg_color(notification, lv_color_hex(0x101B2D), 0);
    lv_obj_set_style_text_color(notification, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_style_border_width(notification, 0, 0);
    lv_obj_set_style_pad_all(notification, 0, 0);
    lv_obj_remove_flag(notification, LV_OBJ_FLAG_SCROLLABLE);
    notification_title = label_at(notification, "", 102, &lv_font_montserrat_24);
    notification_text = label_at(notification, "", 167, &lv_font_montserrat_16);
    lv_obj_set_style_text_align(notification_text, LV_TEXT_ALIGN_CENTER, 0);
    notification_first = button_at(notification, "", 0, 250, 264, notification_action, (void *)(uintptr_t)0);
    notification_second = button_at(notification, "", 0, 322, 264, notification_action, (void *)(uintptr_t)1);
    lv_obj_add_flag(notification, LV_OBJ_FLAG_HIDDEN);
    apps_refresh();
}
