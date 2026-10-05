#include "views/watch_view_internal.h"
static lv_obj_t *note_labels[WATCH_NOTES],*note_text,*note_hint,*note_delete_label,*note_keys[26],*note_mode_label,*note_case_label;

static void note_keyboard_refresh(void)
{
    const watch_notes_state_t *state=watch_notes_vm_state();
    for(unsigned i=0; i<26; ++i) {
        char c = watch_notes_vm_character(i);
        char value[2] = {c,0};
        set_label(note_keys[i],c == '\n' ? LV_SYMBOL_NEW_LINE : value);
        lv_obj_t *button=lv_obj_get_parent(note_keys[i]);
        if(c) lv_obj_remove_state(button,LV_STATE_DISABLED);
        else lv_obj_add_state(button,LV_STATE_DISABLED);
    }
    set_label(note_mode_label,state->mode == 0 ? "123" : state->mode == 1 ? "#+=" : "ABC");
    set_label(note_case_label,state->uppercase ? "a" : "A");
}

static void note_key(lv_event_t *e)
{
    unsigned i = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(i == 27) lv_textarea_delete_char(note_text);
    else if(i == 26) lv_textarea_add_char(note_text,' ');
    else {
        char c=watch_notes_vm_character(i);
        if(c) lv_textarea_add_char(note_text,c);
    }
}

static void note_next_mode(lv_event_t *e) { (void)e; watch_notes_vm_next_mode(); note_keyboard_refresh(); }
static void note_case(lv_event_t *e) { (void)e; watch_notes_vm_case(); note_keyboard_refresh(); }
static void note_changed(lv_event_t *e)
{
    (void)e; watch_notes_vm_change(lv_textarea_get_text(note_text));
    set_label(note_delete_label,LV_SYMBOL_TRASH); set_label(note_hint,watch_notes_vm_state()->hint);
}
static void note_edit(lv_event_t *e)
{
    watch_notes_vm_open((unsigned)(uintptr_t)lv_event_get_user_data(e));
    /* LVGL copies text and emits VALUE_CHANGED synchronously. Avoid overlapping draft copy. */
    char draft[WATCH_NOTE_SIZE]; snprintf(draft,sizeof draft,"%s",watch_notes_vm_state()->draft);
    lv_textarea_set_text(note_text,draft); note_changed(NULL); note_keyboard_refresh(); lv_screen_load(screens[NOTE_EDIT]);
}
static void note_save(lv_event_t *e)
{
    (void)e; watch_notes_vm_change(lv_textarea_get_text(note_text));
    if(watch_notes_vm_save()) { apps_refresh(); lv_screen_load(screens[NOTES]); }
    storage_refresh();
}
static void note_delete(lv_event_t *e)
{
    (void)e; bool deleted=watch_notes_vm_delete();
    set_label(note_delete_label,watch_notes_vm_state()->confirm_delete ? "?" : LV_SYMBOL_TRASH);
    set_label(note_hint,watch_notes_vm_state()->hint); storage_refresh();
    if(deleted) { apps_refresh(); lv_screen_load(screens[NOTES]); }
}
void notes_refresh(void)
{
    char text[96]; for(unsigned i=0;i<WATCH_NOTES;++i) { watch_notes_vm_preview(i,text,sizeof text); set_label(note_labels[i],text); }
}

void notes_init(void)
{    page_header(NOTES, "Notlar", MENU);
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

}
