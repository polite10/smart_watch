#include "views/watch_view_internal.h"
static lv_obj_t *calc_label;

static void calculator_key(lv_event_t *e)
{
    watch_calculator_vm_key(lv_event_get_user_data(e));
    const watch_calculator_state_t *state=watch_calculator_vm_state();
    set_label(calc_label,state->text); lv_obj_set_style_text_font(calc_label,state->small ? &lv_font_montserrat_20 : &lv_font_montserrat_32,0);
}

void calculator_init(void)
{    page_header(CALCULATOR, "Hesap", MENU);
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

}
