#include "views/watch_view_internal.h"
#include "views/watch_fonts.h"

enum { TILE_SIZE=62, TILE_PITCH=70, BOARD_X=104, BOARD_Y=118 };
static lv_obj_t *puzzle_tiles[16], *puzzle_blank, *puzzle_status;
static bool puzzle_moving;
static lv_obj_t *puzzle_best;
static lv_obj_t *tile_top[16],*tile_bottom[16];
static uint32_t tile_colors[16];
static bool tile_theme[16];

static int slot_x(unsigned slot) { return BOARD_X+(slot%4)*TILE_PITCH; }
static int slot_y(unsigned slot) { return BOARD_Y+(slot/4)*TILE_PITCH; }
static void tile_x(void *obj, int32_t value) { lv_obj_set_x(obj,value); }
static void tile_y(void *obj, int32_t value) { lv_obj_set_y(obj,value); }

void puzzle_refresh(void)
{
    if(!puzzle_status) return;
    lv_obj_set_style_bg_color(screens[PUZZLE],lv_color_hex(light_theme ? 0xEAE5DA : 0x2B424D),0);
    const watch_puzzle_state_t *state=watch_puzzle_vm_state();
    char text[48];
    snprintf(text,sizeof text,state->won ? "Tebrikler! / %lu hamle" : "Hamle: %lu / 1 - 15",(unsigned long)state->moves);
    set_label(puzzle_status,text);
    uint32_t status_color=state->won ? 0x96E1C2 : (light_theme ? 0x435675 : 0xDCE8E8);
    if(lv_color_to_u32(lv_obj_get_style_text_color(puzzle_status,0))!=status_color)
        lv_obj_set_style_text_color(puzzle_status,lv_color_hex(status_color),0);
    uint32_t best=watch_games_vm_records()->puzzle_moves;
    if(best) snprintf(text,sizeof text,watch_games_vm_records_saved() ? "En iyi: %lu hamle" : "En iyi: %lu / Kayıt hatası",(unsigned long)best);
    else snprintf(text,sizeof text,"En iyi: --");
    set_label(puzzle_best,text);
    lv_obj_set_pos(puzzle_blank,slot_x(state->blank),slot_y(state->blank));
    for(unsigned slot=0; slot<16; ++slot) {
        unsigned value=state->tiles[slot];
        if(!value) continue;
        lv_obj_t *tile=puzzle_tiles[value];
        bool correct=value==slot+1;
        const uint32_t stone[]={0xE6D7BF,0xC8DDE0,0xE9CDA9,0xD8D0E8};
        uint32_t color=correct ? 0xA6D8B8 : stone[(value-1)/4];
        if(tile_colors[value]!=color || tile_theme[value]!=light_theme) {
            tile_colors[value]=color;tile_theme[value]=light_theme;
            lv_obj_set_style_bg_color(tile,lv_color_hex(color),0);
            lv_obj_set_style_text_color(tile,lv_color_hex(correct ? 0x193B2D : 0x35444B),0);
            lv_obj_set_style_border_color(tile,lv_color_hex(correct ? 0xCFF0D7 : 0xF5EFE1),0);
            lv_obj_set_style_bg_color(tile,lv_color_hex(correct ? 0x89C79D : 0xB7C4C6),LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(tile_top[value],lv_color_hex(correct ? 0xD5F0DB : 0xFFF6E7),0);
            lv_obj_set_style_bg_color(tile_bottom[value],lv_color_hex(correct ? 0x6DA586 : 0x99A7A9),0);
        }
    }
}

static void move_completed(lv_anim_t *animation)
{
    (void)animation; puzzle_moving=false; puzzle_refresh();
    if(watch_puzzle_vm_state()->won && !watch_games_vm_save_records()) set_label(puzzle_best,"Rekor kaydedilemedi");
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
    lv_anim_set_duration(&animation,170);
    lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&animation,move_completed);
    lv_anim_start(&animation);
}

static void new_game(void)
{
    for(unsigned value=1; value<16; ++value) {
        lv_anim_delete(puzzle_tiles[value],tile_x); lv_anim_delete(puzzle_tiles[value],tile_y);
    }
    (void)watch_games_vm_save_records();
    puzzle_moving=false; watch_puzzle_vm_new(lv_tick_get());
    const watch_puzzle_state_t *state=watch_puzzle_vm_state();
    for(unsigned slot=0; slot<16; ++slot) if(state->tiles[slot])
        lv_obj_set_pos(puzzle_tiles[state->tiles[slot]],slot_x(slot),slot_y(slot));
    puzzle_refresh();
}
void puzzle_open(void) { watch_game_background_load(PUZZLE);new_game(); watch_navigation_show(PUZZLE); }

void puzzle_init(void)
{
    watch_game_background_init(PUZZLE);
    page_header(PUZZLE,"15 Bulmaca",MENU);
    puzzle_status=label_at(screens[PUZZLE],"Hamle: 0 / 1 - 15",64,&lv_font_montserrat_16);
    puzzle_best=label_at(screens[PUZZLE],"En iyi: --",88,&watch_font_turkish_16);
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
        button_color_motion(tile);
        lv_obj_set_style_transform_scale_x(tile,256,LV_STATE_PRESSED);
        lv_obj_set_style_transform_scale_y(tile,256,LV_STATE_PRESSED);
        lv_obj_set_align(tile,LV_ALIGN_TOP_LEFT);
        lv_obj_set_height(tile,TILE_SIZE); lv_obj_set_style_radius(tile,11,0);
        lv_obj_set_style_border_width(tile,2,0);
        lv_obj_set_style_text_font(label,&lv_font_montserrat_24,0);
        /* Shallow top and lower stone facets give depth without new draw layers. */
        lv_obj_t **facets[]={&tile_top[value],&tile_bottom[value]};
        for(unsigned j=0;j<2;++j) {
            lv_obj_t *facet=lv_obj_create(tile);*facets[j]=facet;
            lv_obj_set_size(facet,48,3);lv_obj_set_pos(facet,5,j ? 53 : 3);
            lv_obj_set_style_radius(facet,2,0);lv_obj_set_style_border_width(facet,0,0);
            lv_obj_set_style_pad_all(facet,0,0);lv_obj_set_style_bg_opa(facet,180,0);
            lv_obj_remove_flag(facet,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
        }
    }
    new_game();
}
