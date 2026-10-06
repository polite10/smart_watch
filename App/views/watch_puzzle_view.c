#include "views/watch_view_internal.h"

enum { TILE_SIZE=62, TILE_PITCH=70, BOARD_X=104, BOARD_Y=118 };
static lv_obj_t *puzzle_tiles[16], *puzzle_blank, *puzzle_status;
static bool puzzle_moving;

static int slot_x(unsigned slot) { return BOARD_X+(slot%4)*TILE_PITCH; }
static int slot_y(unsigned slot) { return BOARD_Y+(slot/4)*TILE_PITCH; }
static void tile_x(void *obj, int32_t value) { lv_obj_set_x(obj,value); }
static void tile_y(void *obj, int32_t value) { lv_obj_set_y(obj,value); }

void puzzle_refresh(void)
{
    if(!puzzle_status) return;
    const watch_puzzle_state_t *state=watch_puzzle_vm_state();
    char text[48];
    snprintf(text,sizeof text,state->won ? "Tebrikler! / %lu hamle" : "Hamle: %lu / 1 - 15",(unsigned long)state->moves);
    set_label(puzzle_status,text);
    lv_obj_set_style_text_color(puzzle_status,lv_color_hex(state->won ? 0x68CCA9 : (light_theme ? 0x435675 : 0xA8B9D2)),0);
    lv_obj_set_pos(puzzle_blank,slot_x(state->blank),slot_y(state->blank));
    for(unsigned slot=0; slot<16; ++slot) {
        unsigned value=state->tiles[slot];
        if(!value) continue;
        lv_obj_t *tile=puzzle_tiles[value];
        bool correct=value==slot+1;
        lv_obj_set_style_bg_color(tile,lv_color_hex(correct ? 0x96E1C2 : (light_theme ? 0xD4E3F0 : 0x26344C)),0);
        lv_obj_set_style_text_color(tile,lv_color_hex(correct || light_theme ? 0x182C2D : 0xF0F4FC),0);
    }
}

static void move_completed(lv_anim_t *animation)
{
    (void)animation; puzzle_moving=false; puzzle_refresh();
}

static void tile_clicked(lv_event_t *e)
{
    if(puzzle_moving) return;
    unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    const watch_puzzle_state_t *state=watch_puzzle_vm_state();
    unsigned slot=0, destination=state->blank;
    while(slot<16 && state->tiles[slot]!=value) ++slot;
    if(!watch_puzzle_vm_move(slot)) return;
    puzzle_moving=true;
    lv_obj_t *tile=puzzle_tiles[value]; lv_obj_move_foreground(tile);
    lv_obj_set_pos(puzzle_blank,slot_x(slot),slot_y(slot));
    lv_anim_t animation; lv_anim_init(&animation);
    lv_anim_set_var(&animation,tile);
    bool horizontal=slot/4==destination/4;
    lv_anim_set_exec_cb(&animation,horizontal ? tile_x : tile_y);
    lv_anim_set_values(&animation,horizontal ? slot_x(slot) : slot_y(slot),horizontal ? slot_x(destination) : slot_y(destination));
    lv_anim_set_duration(&animation,140);
    lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&animation,move_completed);
    lv_anim_start(&animation);
}

static void new_game(void)
{
    for(unsigned value=1; value<16; ++value) {
        lv_anim_delete(puzzle_tiles[value],tile_x); lv_anim_delete(puzzle_tiles[value],tile_y);
    }
    puzzle_moving=false; watch_puzzle_vm_new(lv_tick_get());
    const watch_puzzle_state_t *state=watch_puzzle_vm_state();
    for(unsigned slot=0; slot<16; ++slot) if(state->tiles[slot])
        lv_obj_set_pos(puzzle_tiles[state->tiles[slot]],slot_x(slot),slot_y(slot));
    puzzle_refresh();
}
static void restart_clicked(lv_event_t *e) { (void)e; new_game(); }
void puzzle_open(void) { new_game(); watch_navigation_show(PUZZLE); }

void puzzle_init(void)
{
    page_header(PUZZLE,"15 Bulmaca",MENU);
    puzzle_status=label_at(screens[PUZZLE],"Hamle: 0 / 1 - 15",84,&lv_font_montserrat_16);
    puzzle_blank=lv_obj_create(screens[PUZZLE]);
    lv_obj_set_size(puzzle_blank,TILE_SIZE,TILE_SIZE);
    lv_obj_set_style_radius(puzzle_blank,14,0);
    lv_obj_set_style_bg_opa(puzzle_blank,LV_OPA_TRANSP,0);
    lv_obj_set_style_border_width(puzzle_blank,2,0);
    lv_obj_set_style_border_color(puzzle_blank,lv_color_hex(0x56708A),0);
    lv_obj_remove_flag(puzzle_blank,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    for(unsigned value=1; value<16; ++value) {
        char text[4]; snprintf(text,sizeof text,"%u",value);
        lv_obj_t *label=button_at(screens[PUZZLE],text,0,0,TILE_SIZE,tile_clicked,(void *)(uintptr_t)value);
        lv_obj_t *tile=lv_obj_get_parent(label); puzzle_tiles[value]=tile;
        lv_obj_set_align(tile,LV_ALIGN_TOP_LEFT);
        lv_obj_set_height(tile,TILE_SIZE); lv_obj_set_style_radius(tile,14,0);
        lv_obj_set_style_text_font(label,&lv_font_montserrat_24,0);
    }
    lv_obj_t *restart=lv_obj_get_parent(button_at(screens[PUZZLE],"Yeni oyun",0,404,176,restart_clicked,NULL));
    lv_obj_set_height(restart,48); lv_obj_set_style_radius(restart,24,0);
    new_game();
}
