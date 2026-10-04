#include "watch_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "watch_model.h"
#include "watch_faces.h"
#include "watch_clock.h"
#include "build_time.h"

enum { HOME, MENU, CALENDAR, SETTINGS, ALARMS, ALARM_EDIT, CALCULATOR,
       NOTES, NOTE_EDIT, FACES, WATER, WATER_SETTINGS, HISTORY, CLOCK_EDIT, SCREEN_COUNT };
static lv_obj_t *screens[SCREEN_COUNT];
static lv_obj_t *calendar, *format_button_label, *theme_button_label;
static lv_obj_t *day_label, *rtc_source_label;
static lv_obj_t *clock_day, *clock_month, *clock_year, *clock_hour, *clock_minute;
static lv_obj_t *clock_hint, *clock_result, *clock_status_label;
static bool ui_clock_valid;
static void clock_editor_init(void);
static void clock_editor_open(void);
static bool twelve_hour, light_theme;
static void apps_init(void);
static void apps_refresh(void);
static void notifications_refresh(void);
static void save_data(void);
static uint16_t current_year = 2026;
static uint8_t current_month = 10, current_day = 4, current_hour, current_minute, current_second;
static lv_obj_t *label_at(lv_obj_t *parent, const char *text, int y, const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
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
    lv_obj_set_size(b, width, 48);
    lv_obj_align(b, LV_ALIGN_TOP_MID, x, y);
    lv_obj_set_style_radius(b, 24, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x253147), 0);
    lv_obj_set_style_text_color(b, lv_color_hex(0xF0F4FC), 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, callback, LV_EVENT_CLICKED, data);
    return l;
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
    lv_label_set_text(format_button_label, twelve_hour ? "Saat bicimi: 12 saat" : "Saat bicimi: 24 saat");
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
    }
    lv_label_set_text(theme_button_label, light_theme ? "Tema: Acik" : "Tema: Koyu");
    watch_data.light_theme = light_theme;
    save_data();
}
static void selected_date(lv_event_t *e)
{
    lv_calendar_date_t d;
    if(lv_calendar_get_pressed_date(lv_event_get_target_obj(e), &d) == LV_RESULT_OK) {
        char text[32];
        snprintf(text, sizeof text, "%02u.%02u.%u", d.day, d.month, d.year);
        lv_label_set_text(day_label, text);
    }
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
    }
    watch_faces_init(screens[HOME], screens[FACES], navigate, (void *)(uintptr_t)MENU);
    label_at(screens[MENU], "UYGULAMALAR", 63, &lv_font_montserrat_24);
    const char *names[] = {"Saat", "Alarm", "Hesap", "Notlar", "Arayuzler", "Su takibi", "Takvim", "Ayarlar"};
    const unsigned targets[] = {HOME, ALARMS, CALCULATOR, NOTES, FACES, WATER, CALENDAR, SETTINGS};
    for(unsigned i = 0; i < 8; ++i)
        button_at(screens[MENU], names[i], i % 2 ? 80 : -80, 126 + (i / 2) * 66,
                  148, navigate, (void *)(uintptr_t)targets[i]);
    button_at(screens[MENU], LV_SYMBOL_LEFT "  Geri", 0, 410, 110, navigate, (void *)(uintptr_t)HOME);
    label_at(screens[2], "TAKVIM", 45, &lv_font_montserrat_24);
    calendar = lv_calendar_create(screens[2]);
    lv_obj_set_size(calendar, 306, 280);
    lv_obj_align(calendar, LV_ALIGN_TOP_MID, 0, 90);
    static const char *days[] = {"Pt", "Sa", "Ca", "Pe", "Cu", "Ct", "Pa"};
    lv_calendar_set_day_names(calendar, days);
    lv_calendar_set_today_date(calendar, current_year, current_month, current_day);
    lv_calendar_set_showed_date(calendar, current_year, current_month);
    lv_calendar_header_arrow_create(calendar);
    lv_obj_add_event_cb(calendar, selected_date, LV_EVENT_VALUE_CHANGED, NULL);
    day_label = label_at(screens[2], "Bir gun secin", 380, &lv_font_montserrat_16);
    button_at(screens[2], LV_SYMBOL_LEFT "  Geri", 0, 413, 110, navigate, (void *)(uintptr_t)1);
    label_at(screens[SETTINGS], "AYARLAR", 61, &lv_font_montserrat_24);
    button_at(screens[SETTINGS], "Saat ve tarih ayarla", 0, 108, 278, navigate, (void *)(uintptr_t)CLOCK_EDIT);
    format_button_label = button_at(screens[SETTINGS], "Saat bicimi: 24 saat", 0, 170, 278, format_clicked, NULL);
    theme_button_label = button_at(screens[SETTINGS], "Tema: Koyu", 0, 232, 278, theme_clicked, NULL);
    button_at(screens[SETTINGS], "Saat arayuzleri", 0, 294, 278, navigate, (void *)(uintptr_t)FACES);
    rtc_source_label = label_at(screens[SETTINGS], "RTC", 352, &lv_font_montserrat_14);
    clock_status_label = label_at(screens[SETTINGS], "Saat ayari gerekli", 377, &lv_font_montserrat_14);
    button_at(screens[SETTINGS], LV_SYMBOL_LEFT "  Geri", 0, 411, 110, navigate, (void *)(uintptr_t)MENU);
    lv_label_set_text(format_button_label, twelve_hour ? "Saat bicimi: 12 saat" : "Saat bicimi: 24 saat");
    lv_label_set_text(theme_button_label, light_theme ? "Tema: Acik" : "Tema: Koyu");
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
    lv_calendar_set_today_date(calendar, year, month, day);
    if(watch_model_tick(watch_day_key(year, month, day), watch_epoch(year, month, day, hour, minute, second), hour, minute)) save_data();
    apps_refresh();
    notifications_refresh();
}
void watch_ui_set_battery(uint8_t percent, bool valid)
{
    watch_faces_set_battery(percent, valid);
}
void watch_ui_set_rtc_source(bool crystal)
{
    lv_label_set_text(rtc_source_label, crystal ? "Saat: LSE kristal" : "Saat: LSI / zaman kayabilir");
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
    lv_label_set_text(clock_result, "Kaydet: saniye 00 olur.");
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
    lv_label_set_text(clock_hint, ui_clock_valid ? "Telefonunuzdaki saat ve tarihi girin.\nSaat bicimi: 24 saat" : "Saat bilinmiyor. Telefonunuza bakin.\nPrizden saat bilgisi alinmaz.");
    lv_label_set_text(clock_result, "Kaydet: saniye 00 olur.");
    lv_screen_load(screens[CLOCK_EDIT]);
}
static void clock_save(lv_event_t *e)
{
    (void)e;
    if(!watch_clock_set_datetime(2000+lv_roller_get_selected(clock_year),1+lv_roller_get_selected(clock_month),
        1+lv_roller_get_selected(clock_day),lv_roller_get_selected(clock_hour),lv_roller_get_selected(clock_minute),0)) {
        lv_label_set_text(clock_result,"Saat kaydedilemedi. Tekrar deneyin.");
        return;
    }
    lv_screen_load(screens[HOME]);
}
static lv_obj_t *clock_roller(int x,int y,int width,unsigned first,unsigned count,bool year)
{
    lv_obj_t *roller=lv_roller_create(screens[CLOCK_EDIT]);
    lv_obj_set_style_text_font(roller,&lv_font_montserrat_16,LV_PART_MAIN);
    lv_obj_set_style_text_font(roller,&lv_font_montserrat_16,LV_PART_SELECTED);
    lv_obj_set_style_text_line_space(roller,6,LV_PART_MAIN);
    clock_options(roller,first,count,year);
    lv_roller_set_visible_row_count(roller,3);
    lv_obj_set_width(roller,width);
    lv_obj_align(roller,LV_ALIGN_TOP_MID,x,y);
    return roller;
}
static void clock_editor_init(void)
{
    label_at(screens[CLOCK_EDIT],"SAAT VE TARIH",47,&lv_font_montserrat_24);
    clock_hint=label_at(screens[CLOCK_EDIT],"",83,&lv_font_montserrat_14);
    lv_obj_set_width(clock_hint,330); lv_obj_set_style_text_align(clock_hint,LV_TEXT_ALIGN_CENTER,0);
    label_at(screens[CLOCK_EDIT],"GUN       AY           YIL",120,&lv_font_montserrat_14);
    clock_day=clock_roller(-96,144,72,1,31,false);
    clock_month=clock_roller(-16,144,72,1,12,false);
    clock_year=clock_roller(88,144,98,2000,100,true);
    lv_obj_add_event_cb(clock_month,clock_date_changed,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(clock_year,clock_date_changed,LV_EVENT_VALUE_CHANGED,NULL);
    label_at(screens[CLOCK_EDIT],"SAAT          DAKIKA",237,&lv_font_montserrat_14);
    clock_hour=clock_roller(-56,259,82,0,24,false);
    clock_minute=clock_roller(56,259,82,0,60,false);
    label_at(screens[CLOCK_EDIT],":",282,&lv_font_montserrat_24);
    clock_result=label_at(screens[CLOCK_EDIT],"",349,&lv_font_montserrat_14);
    lv_obj_set_width(clock_result,330); lv_obj_set_style_text_align(clock_result,LV_TEXT_ALIGN_CENTER,0);
    button_at(screens[CLOCK_EDIT],"Kaydet",-67,390,122,clock_save,NULL);
    button_at(screens[CLOCK_EDIT],"Geri",67,390,122,navigate,(void *)(uintptr_t)MENU);
}

static lv_obj_t *storage_notice, *alarm_labels[WATCH_ALARMS], *alarm_toggle_labels[WATCH_ALARMS];
static lv_obj_t *alarm_hour, *alarm_minute, *alarm_repeat_label;
static lv_obj_t *calc_label, *note_labels[WATCH_NOTES], *note_text, *note_hint, *note_delete_label;
static lv_obj_t *water_ring, *water_total, *water_remaining, *water_goal_label, *water_reminder_label;
static lv_obj_t *history_labels[WATCH_HISTORY];
static lv_obj_t *notification, *notification_title, *notification_text, *notification_first, *notification_second;
static unsigned editing_alarm, editing_note;
static bool edit_daily, confirm_delete, calc_result;
static char calc_expression[48];
static int displayed_notification = -2; /* -2 none, -1 water, >=0 alarm */

void watch_ui_set_clock_valid(bool valid)
{
    bool lost=ui_clock_valid && !valid;
    ui_clock_valid=valid;
    watch_faces_set_clock_valid(valid);
    lv_label_set_text(clock_status_label, valid ? "Saat ayarlandi" : "Saat ayari gerekli");
    if(!valid) {
        lv_calendar_set_today_date(calendar,0,0,0);
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
    lv_calendar_set_showed_date(calendar,year,month);
    save_data();
}
static void page_header(unsigned screen, const char *title, unsigned back)
{
    label_at(screens[screen], title, 47, &lv_font_montserrat_24);
    button_at(screens[screen], LV_SYMBOL_LEFT "  Geri", 0, 413, 110, navigate, (void *)(uintptr_t)back);
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
    lv_label_set_text(alarm_repeat_label, edit_daily ? "Tekrar: Her gun" : "Tekrar: Bir kez");
    lv_screen_load(screens[ALARM_EDIT]);
}
static void alarm_repeat(lv_event_t *e)
{
    (void)e; edit_daily = !edit_daily;
    lv_label_set_text(alarm_repeat_label, edit_daily ? "Tekrar: Her gun" : "Tekrar: Bir kez");
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
    lv_obj_t *matrix = lv_event_get_target_obj(e);
    const char *key = lv_buttonmatrix_get_button_text(matrix, lv_buttonmatrix_get_selected_button(matrix));
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
            lv_label_set_text(calc_label, "Gecersiz islem");
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
    lv_label_set_text(calc_label, calc_expression[0] ? calc_expression : "0");
}
static void note_edit(lv_event_t *e)
{
    editing_note = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    lv_textarea_set_text(note_text, watch_data.notes[editing_note]);
    confirm_delete = false;
    lv_label_set_text(note_delete_label, "Sil");
    lv_label_set_text(note_hint, "En fazla 191 karakter");
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
        lv_label_set_text(note_delete_label, "Onayla");
        lv_label_set_text(note_hint, "Silmek icin tekrar basin");
        return;
    }
    watch_data.notes[editing_note][0] = '\0';
    save_data(); apps_refresh(); lv_screen_load(screens[NOTES]);
}
static void note_changed(lv_event_t *e)
{
    (void)e; confirm_delete = false;
    lv_label_set_text(note_delete_label, "Sil");
    lv_label_set_text(note_hint, "En fazla 191 karakter");
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
        if(selected >= 0) {
            char text[80];
            snprintf(text, sizeof text, "Alarm %u - %02u:%02u\nEkran ve LED uyarisi", selected + 1,
                     watch_data.alarms[selected].hour, watch_data.alarms[selected].minute);
            lv_label_set_text(notification_title, "ALARM");
            lv_label_set_text(notification_text, text);
            lv_label_set_text(notification_first, "5 dk ertele");
            lv_label_set_text(notification_second, "Durdur");
        } else {
            lv_label_set_text(notification_title, "SU ZAMANI");
            lv_label_set_text(notification_text, "Bir bardak su ictiniz mi?\n1 bardak = 200 ml");
            lv_label_set_text(notification_first, "+200 ml");
            lv_label_set_text(notification_second, "Daha sonra");
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
        snprintf(text, sizeof text, "%02u:%02u  %s", a->hour, a->minute, a->daily ? "Her gun" : "Bir kez");
        lv_label_set_text(alarm_labels[i], text);
        lv_label_set_text(alarm_toggle_labels[i], a->enabled ? "Acik" : "Kapali");
        lv_obj_set_style_bg_color(lv_obj_get_parent(alarm_toggle_labels[i]), lv_color_hex(a->enabled ? 0x167D73 : 0x253147), 0);
    }
    for(unsigned i = 0; i < WATCH_NOTES; ++i) {
        char preview[23];
        snprintf(preview, sizeof preview, "%.22s", watch_data.notes[i]);
        for(char *p = preview; *p; ++p) if(*p == '\n') *p = ' ';
        snprintf(text, sizeof text, "%u. %s", i + 1, preview[0] ? preview : "Yeni not");
        lv_label_set_text(note_labels[i], text);
    }
    unsigned ml = watch_data.water[0].ml, goal = watch_data.water_goal;
    lv_arc_set_value(water_ring, !ui_clock_valid ? 0 : ml >= goal ? 100 : ml * 100 / goal);
    if(ui_clock_valid) snprintf(text, sizeof text, "%u ml", ml);
    else snprintf(text, sizeof text, "-- ml");
    lv_label_set_text(water_total, text);
    if(!ui_clock_valid) snprintf(text, sizeof text, "Once saat ve tarihi ayarlayin");
    else snprintf(text, sizeof text, ml >= goal ? "Hedef tamamlandi" : "Kalan: %u ml", ml >= goal ? 0 : goal - ml);
    lv_label_set_text(water_remaining, text);
    snprintf(text, sizeof text, "Gunluk hedef: %u ml", goal); lv_label_set_text(water_goal_label, text);
    if(watch_data.reminder_minutes) snprintf(text, sizeof text, "Hatirlatma: %u dk", watch_data.reminder_minutes);
    else snprintf(text, sizeof text, "Hatirlatma: Kapali");
    lv_label_set_text(water_reminder_label, text);
    for(unsigned i = 0; i < WATCH_HISTORY; ++i) {
        watch_water_day_t *d = &watch_data.water[i];
        if(d->day) snprintf(text, sizeof text, "%02u.%02u.%04u  %u / %u ml", d->day % 100, d->day / 100 % 100, d->day / 10000, d->ml, d->goal);
        else snprintf(text, sizeof text, "--");
        lv_label_set_text(history_labels[i], text);
    }
}
static void apps_init(void)
{
    storage_notice = label_at(lv_layer_top(), "Kayit basarisiz: yeniden deneyin", 25, &lv_font_montserrat_16);
    lv_obj_set_style_text_color(storage_notice, lv_color_hex(0xFF596F), 0);
    lv_obj_add_flag(storage_notice, LV_OBJ_FLAG_HIDDEN);
    page_header(ALARMS, "ALARMLAR", MENU);
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) {
        alarm_labels[i] = button_at(screens[ALARMS], "", -48, 123 + i * 80, 204, alarm_edit, (void *)(uintptr_t)i);
        alarm_toggle_labels[i] = button_at(screens[ALARMS], "", 109, 123 + i * 80, 96, alarm_toggle, (void *)(uintptr_t)i);
    }
    label_at(screens[ALARMS], "Saate dokunarak duzenleyin", 370, &lv_font_montserrat_16);
    page_header(ALARM_EDIT, "ALARM AYARI", ALARMS);
    char options[180] = {0};
    for(unsigned i = 0; i < 24; ++i) { char n[5]; snprintf(n, sizeof n, i == 23 ? "%02u" : "%02u\n", i); strcat(options, n); }
    alarm_hour = lv_roller_create(screens[ALARM_EDIT]);
    lv_roller_set_options(alarm_hour, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_hour, 3); lv_obj_set_width(alarm_hour, 90);
    lv_obj_align(alarm_hour, LV_ALIGN_TOP_MID, -62, 111);
    options[0] = '\0';
    for(unsigned i = 0; i < 60; ++i) { char n[5]; snprintf(n, sizeof n, i == 59 ? "%02u" : "%02u\n", i); strcat(options, n); }
    alarm_minute = lv_roller_create(screens[ALARM_EDIT]);
    lv_roller_set_options(alarm_minute, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_minute, 3); lv_obj_set_width(alarm_minute, 90);
    lv_obj_align(alarm_minute, LV_ALIGN_TOP_MID, 62, 111);
    label_at(screens[ALARM_EDIT], ":", 140, &lv_font_montserrat_24);
    label_at(screens[ALARM_EDIT], "SAAT            DAKIKA", 225, &lv_font_montserrat_16);
    alarm_repeat_label = button_at(screens[ALARM_EDIT], "", 0, 264, 272, alarm_repeat, NULL);
    button_at(screens[ALARM_EDIT], "Kaydet ve etkinlestir", 0, 331, 272, alarm_save, NULL);

    page_header(CALCULATOR, "HESAP MAKINESI", MENU);
    calc_label = label_at(screens[CALCULATOR], "0", 91, &lv_font_montserrat_24);
    lv_obj_set_width(calc_label, 310); lv_label_set_long_mode(calc_label, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(calc_label, LV_TEXT_ALIGN_RIGHT, 0);
    static const char *keys[] = {"C", "DEL", "/", "*", "\n", "7", "8", "9", "-", "\n", "4", "5", "6", "+", "\n", "1", "2", "3", "=", "\n", "0", ".", ""};
    lv_obj_t *keypad = lv_buttonmatrix_create(screens[CALCULATOR]);
    lv_buttonmatrix_set_map(keypad, keys);
    lv_obj_set_size(keypad, 306, 262); lv_obj_align(keypad, LV_ALIGN_TOP_MID, 0, 132);
    lv_obj_set_style_bg_color(keypad, lv_color_hex(0x101B2D), 0);
    lv_obj_set_style_bg_color(keypad, lv_color_hex(0x253147), LV_PART_ITEMS);
    lv_obj_set_style_text_color(keypad, lv_color_hex(0xF0F4FC), LV_PART_ITEMS);
    lv_obj_add_event_cb(keypad, calculator_key, LV_EVENT_VALUE_CHANGED, NULL);

    page_header(NOTES, "NOT DEFTERI", MENU);
    for(unsigned i = 0; i < WATCH_NOTES; ++i)
        note_labels[i] = button_at(screens[NOTES], "", 0, 111 + i * 70, 300, note_edit, (void *)(uintptr_t)i);
    page_header(NOTE_EDIT, "NOT DUZENLE", NOTES);
    note_hint = label_at(screens[NOTE_EDIT], "En fazla 191 karakter", 83, &lv_font_montserrat_16);
    note_text = lv_textarea_create(screens[NOTE_EDIT]);
    lv_obj_set_size(note_text, 310, 105); lv_obj_align(note_text, LV_ALIGN_TOP_MID, 0, 110);
    lv_textarea_set_max_length(note_text, WATCH_NOTE_SIZE - 1);
    lv_textarea_set_accepted_chars(note_text, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?;:-_+*/=()@#\n'");
    lv_obj_add_event_cb(note_text, note_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *keyboard = lv_keyboard_create(screens[NOTE_EDIT]);
    lv_obj_set_size(keyboard, 360, 158); lv_obj_align(keyboard, LV_ALIGN_TOP_MID, 0, 232);
    lv_keyboard_set_textarea(keyboard, note_text);
    /* Compact action row remains inside the round panel. Back cancels edits. */
    lv_obj_t *back_button = lv_obj_get_child(screens[NOTE_EDIT], 1);
    lv_obj_delete(back_button);
    button_at(screens[NOTE_EDIT], "Kaydet", -84, 395, 78, note_save, NULL);
    note_delete_label = button_at(screens[NOTE_EDIT], "Sil", 0, 395, 78, note_delete, NULL);
    button_at(screens[NOTE_EDIT], "Iptal", 84, 395, 78, navigate, (void *)(uintptr_t)NOTES);

    page_header(WATER, "SU TAKIBI", MENU);
    water_ring = lv_arc_create(screens[WATER]); lv_obj_set_size(water_ring, 166, 166);
    lv_obj_align(water_ring, LV_ALIGN_TOP_MID, 0, 89);
    lv_arc_set_rotation(water_ring, 270); lv_arc_set_bg_angles(water_ring, 0, 360);
    lv_arc_set_range(water_ring, 0, 100);
    lv_obj_set_style_arc_width(water_ring, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(water_ring, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(water_ring, lv_color_hex(0x253147), LV_PART_MAIN);
    lv_obj_set_style_arc_color(water_ring, lv_color_hex(0x51C8EF), LV_PART_INDICATOR);
    lv_obj_remove_style(water_ring, NULL, LV_PART_KNOB); lv_obj_remove_flag(water_ring, LV_OBJ_FLAG_CLICKABLE);
    water_total = label_at(screens[WATER], "0 ml", 142, &lv_font_montserrat_24);
    label_at(screens[WATER], "BUGUN", 177, &lv_font_montserrat_16);
    water_remaining = label_at(screens[WATER], "", 267, &lv_font_montserrat_16);
    button_at(screens[WATER], "+1 bardak (200 ml)", -49, 297, 208, water_add, (void *)(intptr_t)1);
    button_at(screens[WATER], "Geri al", 111, 297, 96, water_add, (void *)(intptr_t)-1);
    button_at(screens[WATER], "Hedef / Uyari", -76, 358, 156, navigate, (void *)(uintptr_t)WATER_SETTINGS);
    button_at(screens[WATER], "Gecmis", 88, 358, 140, navigate, (void *)(uintptr_t)HISTORY);
    page_header(WATER_SETTINGS, "SU AYARLARI", WATER);
    water_goal_label = label_at(screens[WATER_SETTINGS], "", 119, &lv_font_montserrat_24);
    button_at(screens[WATER_SETTINGS], "-200", -70, 168, 120, water_goal, (void *)(intptr_t)-200);
    button_at(screens[WATER_SETTINGS], "+200", 70, 168, 120, water_goal, (void *)(intptr_t)200);
    label_at(screens[WATER_SETTINGS], "1 bardak = 200 ml", 235, &lv_font_montserrat_16);
    water_reminder_label = button_at(screens[WATER_SETTINGS], "", 0, 278, 288, water_reminder, NULL);
    label_at(screens[WATER_SETTINGS], "Kapali / 30 / 60 / 120 dk", 341, &lv_font_montserrat_16);
    label_at(screens[WATER_SETTINGS], "Hatirlatma saatleri: 08:00-22:00", 370, &lv_font_montserrat_16);
    page_header(HISTORY, "SU GECMISI", WATER);
    lv_obj_t *list = lv_obj_create(screens[HISTORY]);
    lv_obj_set_size(list, 326, 300); lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 95);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x101B2D), 0);
    lv_obj_set_style_text_color(list, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    for(unsigned i = 0; i < WATCH_HISTORY; ++i) {
        history_labels[i] = lv_label_create(list);
        lv_obj_set_pos(history_labels[i], 0, i * 32); lv_label_set_text(history_labels[i], "--");
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
