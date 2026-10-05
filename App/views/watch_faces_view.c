#include "watch_faces.h"
#include "viewmodels/watch_viewmodels.h"
#include "watch_ui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

LV_FONT_DECLARE(watch_font_clock_72);
static void set_label(lv_obj_t *label, const char *value);
static const lv_style_prop_t no_transition_properties[] = {0};
static const lv_style_transition_dsc_t no_transition = {.props = no_transition_properties};
enum { PASTEL, NEON, CLASSIC, ORBIT, FACE_COUNT };
enum { DIAL_CENTER = 240, DIAL_RADIUS = 166, NUMERAL_RADIUS = 136, INFO_OFFSET = 190 };
_Static_assert(FACE_COUNT==WATCH_FACE_COUNT,"Face count must match persistent model validation");
static lv_obj_t *face_roots[FACE_COUNT], *face_clock[FACE_COUNT], *face_date[FACE_COUNT];
static lv_obj_t *face_water[FACE_COUNT], *face_battery[FACE_COUNT], *face_alarm[FACE_COUNT];
static lv_obj_t *pastel_card, *pastel_progress, *neon_ring, *orbit_ring, *seconds_label, *orbit_seconds, *period_labels[FACE_COUNT];
static lv_obj_t *hour_hand, *minute_hand, *second_hand, *center_pin;
static lv_point_precise_t hand_points[3][2], tick_points[60][2], mini_hand_points[3][2];
static lv_obj_t *picker_screen, *style_buttons[FACE_COUNT], *apply_button;
static lv_obj_t *mini_clocks[FACE_COUNT], *mini_water, *mini_ring, *mini_hands[3], *mini_pin, *picker_hint;
static lv_obj_t *classic_ticks[60], *classic_numbers[4], *mini_numbers[2], *mini_dial;

static const uint32_t accents[] = {0xFFA9C6, 0x94E8C5, 0x92CFFF, 0xFFD18B};
static const char *style_names[] = {"Pastel", "Neon", "Klasik", "Orbit"};

static lv_obj_t *text(lv_obj_t *parent, const char *value, int y, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    set_label(label, value);
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
    lv_obj_set_size(button, width, 60); lv_obj_align(button, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_radius(button, 30, 0); lv_obj_set_style_bg_color(button, lv_color_hex(0x253147), 0);
    lv_obj_set_style_text_color(button, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_style_shadow_width(button,0,0);
    lv_obj_set_style_transition(button,&no_transition,LV_STATE_DEFAULT);
    lv_obj_set_style_transition(button,&no_transition,LV_STATE_PRESSED);
    lv_obj_t *label = lv_label_create(button); set_label(label, value); lv_obj_center(label);
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
static void colors_apply(void)
{
    bool light=watch_settings_vm_light();
    uint32_t background=light ? 0xF6EFE4 : 0x0C1422;
    uint32_t ink=light ? 0x493B35 : 0xF0F4FC;
    uint32_t muted=light ? 0x7B6C62 : 0xA8B9D2;
    lv_obj_set_style_bg_color(face_roots[CLASSIC],lv_color_hex(background),0);
    lv_obj_set_style_bg_color(style_buttons[CLASSIC],lv_color_hex(background),0);
    lv_obj_set_style_bg_color(mini_dial,lv_color_hex(background),0);
    for(unsigned i=0;i<60;++i) lv_obj_set_style_line_color(classic_ticks[i],lv_color_hex(i%5 ? muted : ink),0);
    for(unsigned i=0;i<4;++i) lv_obj_set_style_text_color(classic_numbers[i],lv_color_hex(ink),0);
    for(unsigned i=0;i<2;++i) {
        lv_obj_set_style_text_color(mini_numbers[i],lv_color_hex(ink),0);
        lv_obj_set_style_line_color(mini_hands[i],lv_color_hex(ink),0);
    }
    lv_obj_set_style_text_color(lv_obj_get_child(style_buttons[CLASSIC],1),lv_color_hex(ink),0);
    lv_obj_set_style_line_color(hour_hand,lv_color_hex(ink),0);
    lv_obj_set_style_line_color(minute_hand,lv_color_hex(ink),0);
    lv_obj_set_style_line_color(second_hand,lv_color_hex(0x73BEFF),0);
    lv_obj_set_style_bg_color(center_pin,lv_color_hex(0x73BEFF),0);
    lv_obj_set_style_text_color(face_date[CLASSIC],lv_color_hex(ink),0);
    lv_obj_set_style_text_color(face_water[CLASSIC],lv_color_hex(muted),0);
    lv_obj_set_style_text_color(face_battery[CLASSIC],lv_color_hex(muted),0);
}
void watch_faces_apply_saved(void)
{
    unsigned style = watch_faces_vm_style();
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        if(i == style) lv_obj_remove_flag(face_roots[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(face_roots[i], LV_OBJ_FLAG_HIDDEN);
    }
    colors_apply();
    watch_datetime_t t=watch_faces_vm_state()->datetime;
    watch_faces_set_datetime(t.year,t.month,t.day,t.hour,t.minute,t.second,watch_faces_vm_state()->twelve);
    watch_faces_refresh();
}
static void picker_refresh(void)
{
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        lv_obj_set_style_border_width(style_buttons[i], i == watch_faces_vm_state()->draft_style ? 3 : 1, 0);
        lv_obj_set_style_border_color(style_buttons[i], lv_color_hex(i == watch_faces_vm_state()->draft_style ? accents[i] : 0x33435C), 0);
    }
    colors_apply();
    set_label(picker_hint,"Bir gorunum secin");
    set_label(lv_obj_get_child(apply_button, 0), LV_SYMBOL_OK);
}
void watch_faces_open(void)
{
    watch_faces_vm_open();
    picker_refresh(); lv_screen_load(picker_screen);
}
static void choose_style(lv_event_t *e) { watch_faces_vm_choose((unsigned)(uintptr_t)lv_event_get_user_data(e)); picker_refresh(); }
static void apply(lv_event_t *e)
{
    (void)e;
    if(!watch_faces_vm_apply()) {
        set_label(lv_obj_get_child(apply_button,0),LV_SYMBOL_REFRESH); set_label(picker_hint,"Kayit basarisiz"); return;
    }
    watch_faces_apply_saved(); lv_screen_load(lv_obj_get_parent(face_roots[0]));
}
static void set_label(lv_obj_t *label, const char *value)
{
    if(strcmp(lv_label_get_text(label), value)) lv_label_set_text(label,value);
}

void watch_faces_set_datetime(uint16_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second,bool twelve_hour)
{
    watch_faces_vm_refresh((watch_datetime_t){year,month,day,hour,minute,second},twelve_hour,watch_faces_vm_state()->clock_valid);
    if(lv_screen_active() != picker_screen && lv_screen_active() != lv_obj_get_parent(face_roots[0])) return;
    const watch_faces_state_t *state=watch_faces_vm_state();
    for(unsigned i=0;i<FACE_COUNT;++i) {
        if(face_clock[i]) { set_label(face_clock[i],state->time); set_label(mini_clocks[i],state->time); set_label(period_labels[i],state->period); }
        set_label(face_date[i],state->date);
    }
    set_label(seconds_label,state->seconds); set_label(orbit_seconds,state->orbit_seconds);
    if(!state->clock_valid) return;
    lv_arc_set_value(orbit_ring,second);
    if(watch_faces_vm_style() == CLASSIC && lv_screen_active() != picker_screen) {
        update_hand(hour_hand, hand_points[0], state->hour_angle, DIAL_CENTER, DIAL_CENTER, 85, 12);
        update_hand(minute_hand, hand_points[1], state->minute_angle, DIAL_CENTER, DIAL_CENTER, 120, 16);
        update_hand(second_hand, hand_points[2], state->second_angle, DIAL_CENTER, DIAL_CENTER, 134, 24);
    }
    if(lv_screen_active() == picker_screen) {
        update_hand(mini_hands[0], mini_hand_points[0], state->hour_angle, 50, 50, 21, 0);
        update_hand(mini_hands[1], mini_hand_points[1], minute * 6, 50, 50, 34, 0);
        update_hand(mini_hands[2], mini_hand_points[2], state->second_angle, 50, 50, 40, 8);
    }
}
void watch_faces_set_clock_valid(bool valid)
{
    watch_datetime_t t=watch_faces_vm_state()->datetime;
    watch_faces_vm_refresh(t,watch_faces_vm_state()->twelve,valid);
    lv_obj_t *hands[] = {hour_hand, minute_hand, second_hand, mini_hands[0], mini_hands[1],mini_hands[2]};
    for(unsigned i = 0; i < sizeof hands / sizeof hands[0]; ++i) {
        if(valid) lv_obj_remove_flag(hands[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(hands[i], LV_OBJ_FLAG_HIDDEN);
    }
    watch_faces_set_datetime(t.year,t.month,t.day,t.hour,t.minute,t.second,watch_faces_vm_state()->twelve);
    watch_faces_refresh();
}
void watch_faces_set_battery(uint8_t percent, bool valid)
{
    watch_faces_vm_set_battery(percent,valid);
    for(unsigned i=0;i<FACE_COUNT;++i) {
        set_label(face_battery[i],watch_faces_vm_state()->battery);
        if(valid) lv_obj_remove_flag(face_battery[i],LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(face_battery[i],LV_OBJ_FLAG_HIDDEN);
    }
}
void watch_faces_refresh(void)
{
    if(!neon_ring) return;
    watch_water_state_t water=watch_water_vm_state();
    watch_faces_vm_refresh(watch_faces_vm_state()->datetime,watch_faces_vm_state()->twelve,watch_faces_vm_state()->clock_valid);
    for(unsigned i=0;i<FACE_COUNT;++i) {
        set_label(face_water[i],water.summary);
        if(face_alarm[i]) set_label(face_alarm[i],watch_faces_vm_state()->alarm);
    }
    lv_obj_set_width(pastel_progress,water.progress ? water.progress*226/100 : 1);
    lv_arc_set_value(neon_ring,water.progress); lv_arc_set_value(mini_ring,water.progress);
}
void watch_faces_init(lv_obj_t *home, lv_obj_t *picker, lv_event_cb_t navigate, void *menu)
{
    picker_screen = picker;
    lv_obj_set_style_bg_color(picker, lv_color_hex(0x070D18), 0);
    const uint32_t backgrounds[] = {0x19172B, 0x060C14, 0x0C1422, 0x101B2C};
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        face_roots[i] = shape(home, 0, 0, 480, 480, 0, backgrounds[i]);
        lv_obj_add_flag(face_roots[i], LV_OBJ_FLAG_CLICKABLE);
        /* All home interactions are arbitrated by the raw gesture filter. */
    }
    lv_obj_t *root = face_roots[PASTEL];
    shape(root, 72, 97, 74, 74, LV_RADIUS_CIRCLE, 0x322743);
    shape(root, 338, 216, 63, 63, LV_RADIUS_CIRCLE, 0x23383B);
    shape(root, 125, 245, 22, 22, LV_RADIUS_CIRCLE, 0x50415F);
    text(root, "GUNUN RITMI", 69, &lv_font_montserrat_16, 0xD9CEE6);
    face_date[PASTEL] = text(root, "", 153, &lv_font_montserrat_16, 0xDDD4E8);
    face_clock[PASTEL] = text(root, "00:00", 202, &watch_font_clock_72, accents[0]);
    lv_obj_set_style_text_letter_space(face_clock[PASTEL], -2, 0);
    lv_obj_align(face_clock[PASTEL], LV_ALIGN_CENTER, 0, 0);
    period_labels[PASTEL] = text(root, "", 283, &lv_font_montserrat_16, 0xDDD4E8);
    pastel_card = shape(root, 107, 317, 266, 76, 25, 0x38283F);
    text(pastel_card, "SU HEDEFIN", 10, &lv_font_montserrat_14, 0xEAD8E7);
    face_water[PASTEL] = text(pastel_card, "", 31, &lv_font_montserrat_16, 0xFFF4FA);
    shape(pastel_card, 20, 59, 226, 4, 2, 0x625165);
    pastel_progress = shape(pastel_card, 20, 59, 1, 4, 2, accents[0]);
    face_alarm[PASTEL] = text(root, "", 410, &lv_font_montserrat_14, 0xD9CEE6);
    face_battery[PASTEL] = text(root, "Pil --", 94, &lv_font_montserrat_14, 0xAEA4BF);

    root = face_roots[NEON];
    neon_ring = arc(root, 384, 48, 11, 0x1C2934);
    face_date[NEON] = text(root, "", 145, &lv_font_montserrat_16, 0xC3D5E2);
    face_clock[NEON] = text(root, "00:00", 202, &watch_font_clock_72, accents[NEON]);
    lv_obj_set_style_arc_color(neon_ring,lv_color_hex(accents[NEON]),LV_PART_INDICATOR);
    lv_obj_set_style_text_letter_space(face_clock[NEON], -2, 0);
    lv_obj_align(face_clock[NEON], LV_ALIGN_CENTER, 0, 0);
    seconds_label = text(root, "00", 284, &lv_font_montserrat_24, 0xE5F3FA);
    period_labels[NEON] = text(root, "", 112, &lv_font_montserrat_14, 0x9AB6C8);
    text(root, "SU HEDEFI", 327, &lv_font_montserrat_14, 0x8AABB9);
    face_water[NEON] = text(root, "", 348, &lv_font_montserrat_20, 0xD3EFF1);
    face_alarm[NEON] = text(root, "", 376, &lv_font_montserrat_14, 0xAFCBD5);
    lv_obj_set_width(face_alarm[NEON],190); lv_label_set_long_mode(face_alarm[NEON],LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(face_alarm[NEON],LV_TEXT_ALIGN_CENTER,0);
    face_battery[NEON] = text(root, "Pil --", 75, &lv_font_montserrat_14, 0x8AABB9);

    root = face_roots[CLASSIC];
    for(unsigned i = 0; i < 60; ++i) {
        float angle = i * .10471975512f;
        float inner = i % 5 ? 158 : 149;
        tick_points[i][0] = (lv_point_precise_t){DIAL_CENTER + inner * sinf(angle), DIAL_CENTER - inner * cosf(angle)};
        tick_points[i][1] = (lv_point_precise_t){DIAL_CENTER + DIAL_RADIUS * sinf(angle), DIAL_CENTER - DIAL_RADIUS * cosf(angle)};
        classic_ticks[i]=line(root, tick_points[i], i % 5 ? 2 : 4, i % 5 ? 0xA8B9D2 : 0xF0F4FC);
    }
    /* Center the actual label boxes so font metrics cannot shift the dial. */
    const char *numerals[] = {"12", "3", "9", "6"};
    const int numeral_x[] = {0, NUMERAL_RADIUS, -NUMERAL_RADIUS, 0};
    const int numeral_y[] = {-NUMERAL_RADIUS, 0, 0, NUMERAL_RADIUS};
    for(unsigned i=0;i<4;++i) {
        classic_numbers[i]=text(root,numerals[i],0,&lv_font_montserrat_24,0xF0F4FC);
        lv_obj_align(classic_numbers[i],LV_ALIGN_CENTER,numeral_x[i],numeral_y[i]);
    }
    face_date[CLASSIC] = text(root, "", 0, &lv_font_montserrat_14, 0xF0F4FC);
    lv_obj_align(face_date[CLASSIC],LV_ALIGN_CENTER,0,-INFO_OFFSET);
    face_water[CLASSIC] = text(root, "", 0, &lv_font_montserrat_14, 0xA8B9D2);
    lv_obj_align(face_water[CLASSIC],LV_ALIGN_CENTER,0,INFO_OFFSET);
    face_battery[CLASSIC] = text(root, "Pil --", 22, &lv_font_montserrat_14, 0xA8B9D2);
    hour_hand = line(root, hand_points[0], 10, 0x493B35);
    minute_hand = line(root, hand_points[1], 6, 0x493B35);
    second_hand = line(root, hand_points[2], 3, 0x73BEFF);
    center_pin = shape(root, 0, 0, 14, 14, LV_RADIUS_CIRCLE, 0x73BEFF);
    lv_obj_center(center_pin);

    root=face_roots[ORBIT];
    orbit_ring=arc(root,408,36,6,0x293A51); lv_arc_set_range(orbit_ring,0,59);
    lv_obj_set_style_arc_color(orbit_ring,lv_color_hex(accents[ORBIT]),LV_PART_INDICATOR);
    text(root,"ORBIT",90,&lv_font_montserrat_16,accents[ORBIT]);
    face_date[ORBIT]=text(root,"",150,&lv_font_montserrat_16,0xBDD0E4);
    face_clock[ORBIT]=text(root,"00:00",202,&watch_font_clock_72,0xF5F8FF);
    lv_obj_set_style_text_letter_space(face_clock[ORBIT],-2,0); lv_obj_align(face_clock[ORBIT],LV_ALIGN_CENTER,0,0);
    period_labels[ORBIT]=text(root,"",121,&lv_font_montserrat_14,0xBDD0E4);
    orbit_seconds=text(root,"00 sn",286,&lv_font_montserrat_20,accents[ORBIT]);
    lv_obj_t *card=shape(root,120,327,240,46,23,0x23364C);
    face_water[ORBIT]=text(card,"",12,&lv_font_montserrat_20,0xB7E6F6);
    face_alarm[ORBIT]=text(root,"",384,&lv_font_montserrat_14,0xBDD0E4);
    lv_obj_set_width(face_alarm[ORBIT],190); lv_label_set_long_mode(face_alarm[ORBIT],LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(face_alarm[ORBIT],LV_TEXT_ALIGN_CENTER,0);
    face_battery[ORBIT]=text(root,"Pil --",66,&lv_font_montserrat_14,0xBDD0E4);

    watch_ui_header(picker, "Arayuzler", navigate, menu);
    lv_obj_set_style_text_color(lv_obj_get_child(picker,0),lv_color_hex(0xEDF2FB),0);
    picker_hint=text(picker, "Bir gorunum secin", 82, &lv_font_montserrat_14, 0xAABAD0);
    for(unsigned i = 0; i < FACE_COUNT; ++i) {
        lv_obj_t *button = lv_button_create(picker); style_buttons[i] = button;
        lv_obj_set_size(button,140,140); lv_obj_align(button,LV_ALIGN_TOP_MID,i%2 ? 84 : -84,i<2 ? 106 : 260);
        lv_obj_set_style_pad_all(button, 0, 0); lv_obj_set_style_radius(button,LV_RADIUS_CIRCLE,0);
        lv_obj_set_style_shadow_width(button,0,0);
        lv_obj_set_style_transition(button,&no_transition,LV_STATE_DEFAULT);
        lv_obj_set_style_transition(button,&no_transition,LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(button,lv_color_hex(backgrounds[i]),0);
        lv_obj_add_event_cb(button, choose_style, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *dial = shape(button, 20, 8, 100, 100, LV_RADIUS_CIRCLE, backgrounds[i]);
        if(i != CLASSIC) {
            if(i == 0) mini_water = shape(dial, 25, 72, 50, 7, 4, accents[0]);
            else {
                lv_obj_t *ring=arc(dial,90,5,3,0x273C4A);
                lv_obj_set_style_arc_color(ring,lv_color_hex(accents[i]),LV_PART_INDICATOR);
                if(i==NEON) mini_ring=ring; else lv_arc_set_value(ring,65);
            }
            mini_clocks[i] = text(dial, "00:00", 36, &lv_font_montserrat_24, accents[i]);
        } else {
            mini_dial=dial;
            mini_numbers[0]=text(dial, "12", 4, &lv_font_montserrat_14, 0xF0F4FC);
            mini_numbers[1]=text(dial, "6", 80, &lv_font_montserrat_14, 0xF0F4FC);
            mini_hands[0] = line(dial, mini_hand_points[0], 3, 0x493B35);
            mini_hands[1] = line(dial, mini_hand_points[1], 2, 0x493B35);
            mini_hands[2] = line(dial, mini_hand_points[2], 1, 0x73BEFF);
            mini_pin = shape(dial, 47, 47, 6, 6, LV_RADIUS_CIRCLE, 0x73BEFF);
        }
        text(button,style_names[i],115,&lv_font_montserrat_14,0xEDF2FB);
    }
    apply_button = action(picker, LV_SYMBOL_OK, 410, 64, apply, NULL);
    lv_obj_set_height(apply_button,64); lv_obj_set_style_radius(apply_button,LV_RADIUS_CIRCLE,0);
    lv_obj_set_style_pad_all(apply_button,0,0); lv_obj_set_style_border_width(apply_button,0,0);
    lv_obj_set_style_bg_color(apply_button,lv_color_hex(0x96E1C2),0);
    lv_obj_set_style_bg_color(apply_button,lv_color_hex(0x6CB99B),LV_STATE_PRESSED);
    lv_obj_set_style_text_color(apply_button,lv_color_hex(0x182C2D),0);
    lv_obj_set_style_text_font(lv_obj_get_child(apply_button,0),&lv_font_montserrat_28,0);

    watch_datetime_t t=watch_faces_vm_state()->datetime;
    watch_faces_set_datetime(t.year,t.month,t.day,t.hour,t.minute,t.second,watch_faces_vm_state()->twelve);
    watch_faces_set_battery(0, false);
    watch_faces_apply_saved();
}
