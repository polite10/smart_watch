#include "watch_ui.h"
#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *screens[4], *clock_label, *date_label, *battery_label;
static lv_obj_t *rings[3], *calendar, *format_button_label, *theme_button_label;
static lv_obj_t *day_label;
static bool twelve_hour, light_theme;
static uint16_t current_year = 2026;
static uint8_t current_month = 10, current_day = 4, current_hour, current_minute;
static const char *months[] = {"OCAK", "SUBAT", "MART", "NISAN", "MAYIS", "HAZIRAN",
                              "TEMMUZ", "AGUSTOS", "EYLUL", "EKIM", "KASIM", "ARALIK"};
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
    lv_screen_load(screens[screen]);
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
    char text[40];
    unsigned h = current_hour;
    if(twelve_hour) h = h % 12 == 0 ? 12 : h % 12;
    snprintf(text, sizeof text, "%02u:%02u", h, current_minute);
    lv_label_set_text(clock_label, text);
    snprintf(text, sizeof text, "%u %s %u%s", current_day, months[current_month - 1],
             current_year, twelve_hour ? (current_hour < 12 ? " AM" : " PM") : "");
    lv_label_set_text(date_label, text);
}
static void format_clicked(lv_event_t *e)
{
    (void)e;
    twelve_hour = !twelve_hour;
    lv_label_set_text(format_button_label, twelve_hour ? "Saat bicimi: 12 saat" : "Saat bicimi: 24 saat");
    refresh_time();
}
static void theme_clicked(lv_event_t *e)
{
    (void)e;
    light_theme = !light_theme;
    for(unsigned i = 0; i < 4; ++i) {
        lv_obj_set_style_bg_color(screens[i], lv_color_hex(light_theme ? 0xEAF0F8 : 0x070D18), 0);
        lv_obj_set_style_text_color(screens[i], lv_color_hex(light_theme ? 0x182238 : 0xF0F4FC), 0);
    }
    lv_label_set_text(theme_button_label, light_theme ? "Tema: Acik" : "Tema: Koyu");
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
    for(unsigned i = 0; i < 4; ++i) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_remove_flag(screens[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(screens[i], lv_color_hex(0x070D18), 0);
        lv_obj_set_style_text_color(screens[i], lv_color_hex(0xF0F4FC), 0);
        lv_obj_set_style_text_font(screens[i], &lv_font_montserrat_16, 0);
    }
    battery_label = label_at(screens[0], "Pil: --", 57, &lv_font_montserrat_16);
    const uint32_t colors[] = {0xFF596F, 0xADDF52, 0x51C8EF};
    const int sizes[] = {326, 300, 274};
    for(unsigned i = 0; i < 3; ++i) {
        rings[i] = lv_arc_create(screens[0]);
        lv_obj_set_size(rings[i], sizes[i], sizes[i]);
        lv_obj_align(rings[i], LV_ALIGN_CENTER, 0, -1);
        lv_arc_set_rotation(rings[i], 270);
        lv_arc_set_bg_angles(rings[i], 0, 360);
        lv_arc_set_range(rings[i], 0, 100);
        lv_arc_set_value(rings[i], 0);
        lv_obj_set_style_arc_width(rings[i], 8, LV_PART_MAIN);
        lv_obj_set_style_arc_width(rings[i], 8, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(rings[i], lv_color_hex(0x253147), LV_PART_MAIN);
        lv_obj_set_style_arc_color(rings[i], lv_color_hex(colors[i]), LV_PART_INDICATOR);
        lv_obj_remove_style(rings[i], NULL, LV_PART_KNOB);
        lv_obj_remove_flag(rings[i], LV_OBJ_FLAG_CLICKABLE);
    }
    clock_label = label_at(screens[0], "00:00", 187, &lv_font_montserrat_48);
    date_label = label_at(screens[0], "4 EKIM 2026", 251, &lv_font_montserrat_16);
    label_at(screens[0], "AKTIVITE / DEMO", 291, &lv_font_montserrat_16);
    button_at(screens[0], LV_SYMBOL_LIST "  Menu", 0, 407, 120, navigate, (void *)(uintptr_t)1);
    label_at(screens[1], "UYGULAMALAR", 80, &lv_font_montserrat_24);
    button_at(screens[1], LV_SYMBOL_HOME "  Saat", 0, 146, 246, navigate, (void *)(uintptr_t)0);
    button_at(screens[1], "Takvim", 0, 212, 246, navigate, (void *)(uintptr_t)2);
    button_at(screens[1], LV_SYMBOL_SETTINGS "  Ayarlar", 0, 278, 246, navigate, (void *)(uintptr_t)3);
    button_at(screens[1], LV_SYMBOL_LEFT "  Geri", 0, 395, 110, navigate, (void *)(uintptr_t)0);
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
    label_at(screens[3], "AYARLAR", 80, &lv_font_montserrat_24);
    format_button_label = button_at(screens[3], "Saat bicimi: 24 saat", 0, 160, 278, format_clicked, NULL);
    theme_button_label = button_at(screens[3], "Tema: Koyu", 0, 226, 278, theme_clicked, NULL);
    label_at(screens[3], "Pil: veri bekleniyor", 300, &lv_font_montserrat_16);
    label_at(screens[3], "Aktivite: demo", 326, &lv_font_montserrat_16);
    button_at(screens[3], LV_SYMBOL_LEFT "  Geri", 0, 395, 110, navigate, (void *)(uintptr_t)1);
    watch_ui_set_activity(72, 45, 83);
    refresh_time();
    lv_screen_load(screens[0]);
}
void watch_ui_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second)
{
    (void)second;
    if(year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59) return;
    current_year = year; current_month = month; current_day = day;
    current_hour = hour; current_minute = minute;
    refresh_time();
    lv_calendar_set_today_date(calendar, year, month, day);
}
void watch_ui_set_battery(uint8_t percent, bool valid)
{
    char text[24];
    if(valid) snprintf(text, sizeof text, "Pil: %u%%", percent > 100 ? 100 : percent);
    else snprintf(text, sizeof text, "Pil: --");
    lv_label_set_text(battery_label, text);
}
void watch_ui_set_activity(uint16_t move, uint16_t exercise, uint16_t stand)
{
    const uint16_t values[] = {move, exercise, stand};
    for(unsigned i = 0; i < 3; ++i) lv_arc_set_value(rings[i], values[i] > 100 ? 100 : values[i]);
}
