#include "watch_faces.h"
#include "watch_model.h"
#include <stdio.h>
#include <math.h>

LV_FONT_DECLARE(watch_font_clock_72);
enum { PASTEL, NEON, CLASSIC, FACE_COUNT };
static lv_obj_t *face_roots[FACE_COUNT], *face_clock[2], *face_date[FACE_COUNT];
static lv_obj_t *face_water[FACE_COUNT], *face_battery[FACE_COUNT], *face_alarm[2];
static lv_obj_t *pastel_card, *pastel_progress, *neon_ring, *seconds_label, *period_labels[2];
static lv_obj_t *hour_hand, *minute_hand, *second_hand, *center_pin;
static lv_point_precise_t hand_points[3][2], tick_points[60][2], mini_hand_points[2][2];
static lv_obj_t *picker_screen, *style_buttons[FACE_COUNT], *color_buttons[FACE_COUNT], *apply_button;
static lv_obj_t *mini_clocks[2], *mini_water, *mini_ring, *mini_hands[2], *mini_pin, *color_hint;
static unsigned draft_style, draft_color;
static uint16_t face_year = 2026;
static uint8_t face_month = 10, face_day = 4, face_hour, face_minute, face_second;
static bool face_twelve;
static const uint32_t accents[] = {0xFFA9C6, 0x94E8C5, 0x92CFFF};
static const uint32_t ink_accents[] = {0xA73C67, 0x20725F, 0x315E9D};
static const char *style_names[] = {"Pastel", "Neon", "Klasik"};
static const char *color_names[] = {"Pembe", "Mint", "Mavi"};
static const char *months[] = {"OCAK", "SUBAT", "MART", "NISAN", "MAYIS", "HAZIRAN",
                              "TEMMUZ", "AGUSTOS", "EYLUL", "EKIM", "KASIM", "ARALIK"};

static lv_obj_t *text(lv_obj_t *parent, const char *value, int y, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, value);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
    return label;
}
static lv_obj_t *shape(lv_obj_t *parent, int x, int y, int width, int height, int radius, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, width, height);
    lv_obj_set_style_pad_all(obj, 0, 0); lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, radius, 0); lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
static lv_obj_t *action(lv_obj_t *parent, const char *value, int y, int width, lv_event_cb_t callback, void *data)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, width, 46); lv_obj_align(button, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_radius(button, 23, 0); lv_obj_set_style_bg_color(button, lv_color_hex(0x253147), 0);
    lv_obj_set_style_text_color(button, lv_color_hex(0xF0F4FC), 0);
    lv_obj_t *label = lv_label_create(button); lv_label_set_text(label, value); lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, data);
    return button;
}
static lv_obj_t *arc(lv_obj_t *parent, int size, int y, int width, uint32_t track)
{
    lv_obj_t *obj = lv_arc_create(parent);
    lv_obj_set_size(obj, size, size); lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y);
    lv_arc_set_rotation(obj, 270); lv_arc_set_bg_angles(obj, 0, 360); lv_arc_set_range(obj, 0, 100);
    lv_obj_set_style_arc_width(obj, width, LV_PART_MAIN); lv_obj_set_style_arc_width(obj, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(obj, lv_color_hex(track), LV_PART_MAIN);
    lv_obj_remove_style(obj, NULL, LV_PART_KNOB); lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
static lv_obj_t *line(lv_obj_t *parent, lv_point_precise_t *points, int width, uint32_t color)
{
    lv_obj_t *obj = lv_line_create(parent);
    lv_line_set_points(obj, points, 2);
    lv_obj_set_style_line_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_line_width(obj, width, 0); lv_obj_set_style_line_rounded(obj, true, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
static void update_hand(lv_obj_t *obj, lv_point_precise_t *points, float angle, float cx, float cy, float length, float tail)
{
    float a = angle * 0.01745329252f;
    points[0] = (lv_point_precise_t){cx - tail * sinf(a), cy + tail * cosf(a)};
    points[1] = (lv_point_precise_t){cx + length * sinf(a), cy - length * cosf(a)};
    lv_line_set_points(obj, points, 2); lv_obj_invalidate(obj);
}
static void colors_apply(unsigned color)
{
    lv_color_t accent = lv_color_hex(accents[color]);
    lv_obj_set_style_text_color(face_clock[PASTEL], accent, 0);
    lv_obj_set_style_text_color(face_clock[NEON], accent, 0);
    lv_obj_set_style_bg_color(pastel_progress, accent, 0);
    lv_obj_set_style_arc_color(neon_ring, accent, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(second_hand, lv_color_hex(ink_accents[color]), 0);
    lv_obj_set_style_bg_color(center_pin, lv_color_hex(ink_accents[color]), 0);
    lv_obj_set_style_bg_color(pastel_card, lv_color_hex(color == 0 ? 0x38283F : color == 1 ? 0x233D3E : 0x24364C), 0);
    lv_obj_set_style_text_color(face_date[CLASSIC], lv_color_hex(ink_accents[color]), 0);
}
void watch_faces_apply_saved(void)
{
    unsigned style = watch_data.face_style < FACE_COUNT ? watch_data.face_style : 0;
    unsigned color = watch_data.face_color < FACE_COUNT ? watch_data.face_color : 0;
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        if(i == style) lv_obj_remove_flag(face_roots[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(face_roots[i], LV_OBJ_FLAG_HIDDEN);
    }
    colors_apply(color);
    watch_faces_refresh();
}
static void picker_refresh(void)
{
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        lv_obj_set_style_border_width(style_buttons[i], i == draft_style ? 3 : 1, 0);
        lv_obj_set_style_border_color(style_buttons[i], lv_color_hex(i == draft_style ? accents[draft_color] : 0x33435C), 0);
        lv_obj_set_style_border_width(color_buttons[i], i == draft_color ? 3 : 0, 0);
        lv_obj_set_style_border_color(color_buttons[i], lv_color_hex(0xFFFFFF), 0);
    }
    lv_obj_set_style_text_color(mini_clocks[0], lv_color_hex(accents[draft_color]), 0);
    lv_obj_set_style_text_color(mini_clocks[1], lv_color_hex(accents[draft_color]), 0);
    lv_obj_set_style_bg_color(mini_water, lv_color_hex(accents[draft_color]), 0);
    lv_obj_set_style_arc_color(mini_ring, lv_color_hex(accents[draft_color]), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(mini_pin, lv_color_hex(ink_accents[draft_color]), 0);
    lv_label_set_text(color_hint, color_names[draft_color]);
    lv_label_set_text(lv_obj_get_child(apply_button, 0), "Uygula");
}
void watch_faces_open(void)
{
    draft_style = watch_data.face_style; draft_color = watch_data.face_color;
    picker_refresh(); lv_screen_load(picker_screen);
}
static void choose_style(lv_event_t *e)
{
    draft_style = (unsigned)(uintptr_t)lv_event_get_user_data(e); picker_refresh();
}
static void choose_color(lv_event_t *e)
{
    draft_color = (unsigned)(uintptr_t)lv_event_get_user_data(e); picker_refresh();
}
static void apply(lv_event_t *e)
{
    (void)e;
    uint8_t old_style = watch_data.face_style, old_color = watch_data.face_color;
    watch_data.face_style = draft_style; watch_data.face_color = draft_color;
    if(!watch_model_save()) {
        watch_data.face_style = old_style; watch_data.face_color = old_color;
        lv_label_set_text(lv_obj_get_child(apply_button, 0), "Tekrar dene");
        lv_label_set_text(color_hint, "Kayit basarisiz");
        return;
    }
    watch_faces_apply_saved(); lv_screen_load(lv_obj_get_parent(face_roots[0]));
}
static void long_press(lv_event_t *e) { (void)e; watch_faces_open(); }

void watch_faces_set_datetime(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second, bool twelve_hour)
{
    face_year = year; face_month = month; face_day = day;
    face_hour = hour; face_minute = minute; face_second = second; face_twelve = twelve_hour;
    unsigned h = twelve_hour ? (hour % 12 ? hour % 12 : 12) : hour;
    char value[40]; snprintf(value, sizeof value, "%02u:%02u", h, minute);
    for(unsigned i = 0; i < 2; ++i) {
        lv_label_set_text(face_clock[i], value); lv_label_set_text(mini_clocks[i], value);
        lv_label_set_text(period_labels[i], twelve_hour ? (hour < 12 ? "AM" : "PM") : "");
    }
    snprintf(value, sizeof value, "%02u", second); lv_label_set_text(seconds_label, value);
    snprintf(value, sizeof value, "%u %s %u", day, months[month - 1], year);
    for(unsigned i = 0; i < FACE_COUNT; ++i) lv_label_set_text(face_date[i], value);
    update_hand(hour_hand, hand_points[0], (hour % 12) * 30 + minute * .5f, 240, 225, 85, 12);
    update_hand(minute_hand, hand_points[1], minute * 6 + second * .1f, 240, 225, 125, 16);
    update_hand(second_hand, hand_points[2], second * 6, 240, 225, 134, 24);
    update_hand(mini_hands[0], mini_hand_points[0], (hour % 12) * 30 + minute * .5f, 50, 50, 21, 0);
    update_hand(mini_hands[1], mini_hand_points[1], minute * 6, 50, 50, 34, 0);
}
void watch_faces_set_battery(uint8_t percent, bool valid)
{
    char value[24];
    if(valid) snprintf(value, sizeof value, "Pil %u%%", percent > 100 ? 100 : percent);
    else snprintf(value, sizeof value, "Pil --");
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        lv_label_set_text(face_battery[i], value);
        if(valid) lv_obj_remove_flag(face_battery[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(face_battery[i], LV_OBJ_FLAG_HIDDEN);
    }
}
void watch_faces_refresh(void)
{
    if(!neon_ring) return;
    unsigned ml = watch_data.water[0].ml, goal = watch_data.water_goal;
    unsigned progress = ml >= goal ? 100 : ml * 100 / goal;
    char value[48]; snprintf(value, sizeof value, "%u / %u ml", ml, goal);
    for(unsigned i = 0; i < FACE_COUNT; ++i) lv_label_set_text(face_water[i], value);
    lv_obj_set_width(pastel_progress, progress ? progress * 226 / 100 : 1);
    lv_arc_set_value(neon_ring, progress); lv_arc_set_value(mini_ring, progress);
    int next_alarm = -1; unsigned closest = 1441;
    for(unsigned i = 0; i < WATCH_ALARMS; ++i) if(watch_data.alarms[i].enabled) {
        unsigned target = watch_data.alarms[i].hour * 60 + watch_data.alarms[i].minute;
        unsigned distance = (target + 1440 - face_hour * 60 - face_minute) % 1440;
        if(distance < closest) { closest = distance; next_alarm = i; }
    }
    if(next_alarm >= 0) snprintf(value, sizeof value, "Alarm  %02u:%02u", watch_data.alarms[next_alarm].hour, watch_data.alarms[next_alarm].minute);
    else snprintf(value, sizeof value, "Alarm kapali");
    for(unsigned i = 0; i < 2; ++i) lv_label_set_text(face_alarm[i], value);
}
void watch_faces_init(lv_obj_t *home, lv_obj_t *picker, lv_event_cb_t navigate, void *menu)
{
    picker_screen = picker;
    lv_obj_set_style_bg_color(picker, lv_color_hex(0x070D18), 0);
    const uint32_t backgrounds[] = {0x19172B, 0x060C14, 0xF6EFE4};
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        face_roots[i] = shape(home, 0, 0, 480, 480, 0, backgrounds[i]);
        lv_obj_add_flag(face_roots[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(face_roots[i], long_press, LV_EVENT_LONG_PRESSED, NULL);
        action(face_roots[i], LV_SYMBOL_LIST "  Menu", 414, 120, navigate, menu);
    }
    lv_obj_t *root = face_roots[PASTEL];
    shape(root, 72, 97, 74, 74, LV_RADIUS_CIRCLE, 0x322743);
    shape(root, 338, 216, 63, 63, LV_RADIUS_CIRCLE, 0x23383B);
    shape(root, 125, 245, 22, 22, LV_RADIUS_CIRCLE, 0x50415F);
    text(root, "GUNUN RITMI", 69, &lv_font_montserrat_16, 0xD9CEE6);
    face_date[PASTEL] = text(root, "", 120, &lv_font_montserrat_16, 0xDDD4E8);
    face_clock[PASTEL] = text(root, "00:00", 171, &watch_font_clock_72, accents[0]);
    lv_obj_set_style_text_letter_space(face_clock[PASTEL], -2, 0);
    period_labels[PASTEL] = text(root, "", 250, &lv_font_montserrat_16, 0xDDD4E8);
    pastel_card = shape(root, 107, 282, 266, 76, 25, 0x38283F);
    text(pastel_card, "SU HEDEFIN", 10, &lv_font_montserrat_14, 0xEAD8E7);
    face_water[PASTEL] = text(pastel_card, "", 31, &lv_font_montserrat_16, 0xFFF4FA);
    shape(pastel_card, 20, 59, 226, 4, 2, 0x625165);
    pastel_progress = shape(pastel_card, 20, 59, 1, 4, 2, accents[0]);
    face_alarm[PASTEL] = text(root, "", 372, &lv_font_montserrat_14, 0xD9CEE6);
    face_battery[PASTEL] = text(root, "Pil --", 94, &lv_font_montserrat_14, 0xAEA4BF);

    root = face_roots[NEON];
    neon_ring = arc(root, 368, 41, 11, 0x1C2934);
    face_date[NEON] = text(root, "", 106, &lv_font_montserrat_16, 0xC3D5E2);
    face_clock[NEON] = text(root, "00:00", 166, &watch_font_clock_72, accents[0]);
    lv_obj_set_style_text_letter_space(face_clock[NEON], -2, 0);
    seconds_label = text(root, "00", 245, &lv_font_montserrat_24, 0xE5F3FA);
    period_labels[NEON] = text(root, "", 139, &lv_font_montserrat_14, 0x9AB6C8);
    text(root, "SU HEDEFI", 289, &lv_font_montserrat_14, 0x8AABB9);
    face_water[NEON] = text(root, "", 312, &lv_font_montserrat_24, 0xD3EFF1);
    face_alarm[NEON] = text(root, "", 357, &lv_font_montserrat_14, 0xAFCBD5);
    face_battery[NEON] = text(root, "Pil --", 75, &lv_font_montserrat_14, 0x8AABB9);

    root = face_roots[CLASSIC];
    for(unsigned i = 0; i < 60; ++i) {
        float angle = i * .10471975512f;
        float inner = i % 5 ? 158 : 149;
        tick_points[i][0] = (lv_point_precise_t){240 + inner * sinf(angle), 225 - inner * cosf(angle)};
        tick_points[i][1] = (lv_point_precise_t){240 + 166 * sinf(angle), 225 - 166 * cosf(angle)};
        line(root, tick_points[i], i % 5 ? 2 : 4, i % 5 ? 0xB6AB9C : 0x493B35);
    }
    text(root, "12", 83, &lv_font_montserrat_24, 0x493B35);
    lv_obj_t *number = text(root, "3", 211, &lv_font_montserrat_24, 0x493B35); lv_obj_align(number, LV_ALIGN_TOP_MID, 136, 211);
    number = text(root, "9", 211, &lv_font_montserrat_24, 0x493B35); lv_obj_align(number, LV_ALIGN_TOP_MID, -136, 211);
    text(root, "6", 344, &lv_font_montserrat_24, 0x493B35);
    text(root, "KLASIK", 143, &lv_font_montserrat_14, 0x9A8576);
    face_date[CLASSIC] = text(root, "", 283, &lv_font_montserrat_14, ink_accents[0]);
    face_water[CLASSIC] = text(root, "", 312, &lv_font_montserrat_14, 0x7B6C62);
    face_battery[CLASSIC] = text(root, "Pil --", 393, &lv_font_montserrat_14, 0x7B6C62);
    hour_hand = line(root, hand_points[0], 10, 0x493B35);
    minute_hand = line(root, hand_points[1], 6, 0x493B35);
    second_hand = line(root, hand_points[2], 2, ink_accents[0]);
    center_pin = shape(root, 233, 218, 14, 14, LV_RADIUS_CIRCLE, ink_accents[0]);

    text(picker, "SAAT ARAYUZLERI", 47, &lv_font_montserrat_24, 0xEDF2FB);
    text(picker, "Bir gorunum secin", 86, &lv_font_montserrat_14, 0xAABAD0);
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        lv_obj_t *button = lv_button_create(picker); style_buttons[i] = button;
        lv_obj_set_size(button, 116, 145); lv_obj_align(button, LV_ALIGN_TOP_MID, ((int)i - 1) * 122, 116);
        lv_obj_set_style_pad_all(button, 0, 0); lv_obj_set_style_radius(button, 24, 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x142033), 0);
        lv_obj_add_event_cb(button, choose_style, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *dial = shape(button, 8, 10, 100, 100, LV_RADIUS_CIRCLE, backgrounds[i]);
        if(i < 2) {
            if(i == 0) mini_water = shape(dial, 25, 72, 50, 7, 4, accents[0]);
            else mini_ring = arc(dial, 90, 5, 3, 0x273C4A);
            mini_clocks[i] = text(dial, "00:00", 36, &lv_font_montserrat_24, accents[0]);
        } else {
            text(dial, "12", 4, &lv_font_montserrat_14, 0x493B35);
            text(dial, "6", 80, &lv_font_montserrat_14, 0x493B35);
            mini_hands[0] = line(dial, mini_hand_points[0], 3, 0x493B35);
            mini_hands[1] = line(dial, mini_hand_points[1], 2, 0x493B35);
            mini_pin = shape(dial, 47, 47, 6, 6, LV_RADIUS_CIRCLE, ink_accents[0]);
        }
        text(button, style_names[i], 119, &lv_font_montserrat_16, 0xEDF2FB);
    }
    color_hint = text(picker, "Pembe", 276, &lv_font_montserrat_16, 0xBDCEE2);
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        lv_obj_t *button = lv_button_create(picker); color_buttons[i] = button;
        lv_obj_set_size(button, 38, 38); lv_obj_align(button, LV_ALIGN_TOP_MID, ((int)i - 1) * 57, 306);
        lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(accents[i]), 0);
        lv_obj_add_event_cb(button, choose_color, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
    apply_button = action(picker, "Uygula", 359, 176, apply, NULL);
    action(picker, LV_SYMBOL_LEFT "  Geri", 414, 110, navigate, menu);
    watch_faces_set_datetime(face_year, face_month, face_day, face_hour, face_minute, face_second, face_twelve);
    watch_faces_set_battery(0, false);
    watch_faces_apply_saved();
}
