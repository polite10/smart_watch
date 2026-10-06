#include "views/watch_view_internal.h"
#include "viewmodels/watch_arcade_vm.h"
#include "views/watch_art.h"

static lv_obj_t *flappy_bird,*flappy_score,*flappy_hint,*flappy_pause;
static lv_obj_t *pipe_top[3],*pipe_bottom[3],*pipe_top_cap[3],*pipe_bottom_cap[3];
static lv_obj_t *snake_canvas,*snake_score,*snake_hint,*snake_pause;
static uint16_t snake_pixels[288*288] __attribute__((aligned(4)));
static uint32_t arcade_last_tick,arcade_accumulator;
static lv_point_t snake_anchor;

static lv_obj_t *plain(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *obj=lv_obj_create(parent);lv_obj_set_pos(obj,x,y);lv_obj_set_size(obj,w,h);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj,lv_color_hex(color),0);lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_radius(obj,radius,0);lv_obj_set_style_pad_all(obj,0,0);
    return obj;
}
static void score_label(lv_obj_t *label,unsigned score)
{
    char text[12];snprintf(text,sizeof text,"%02u",score);set_label(label,text);
}
static void hint(lv_obj_t *label,watch_game_phase_t phase,bool snake)
{
    const char *text=phase==WATCH_GAME_READY ? (snake ? "Surukleyerek basla" : "Dokun ve uc") :
        phase==WATCH_GAME_PAUSED ? "Duraklatildi" : phase==WATCH_GAME_OVER ? "Oyun bitti\nTekrar dokun" :
        phase==WATCH_GAME_WON ? "Tebrikler!\nTekrar dokun" : "";
    set_label(label,text);
    if(*text) lv_obj_remove_flag(label,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(label,LV_OBJ_FLAG_HIDDEN);
}
static void flappy_render(void)
{
    const watch_flappy_state_t *s=watch_flappy_vm_state();score_label(flappy_score,s->score);hint(flappy_hint,s->phase,false);
    set_label(flappy_pause,s->phase==WATCH_GAME_PAUSED ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    lv_obj_set_pos(flappy_bird,142,(int)s->y-28);lv_image_set_rotation(flappy_bird,(int)(s->velocity*60));
    for(unsigned i=0;i<3;++i) {
        int x=(int)s->pipes[i].x,top=(int)s->pipes[i].gap_y-64,bottom=(int)s->pipes[i].gap_y+64;
        lv_obj_t *parts[]={pipe_top[i],pipe_bottom[i],pipe_top_cap[i],pipe_bottom_cap[i]};
        for(unsigned j=0;j<4;++j) {
            if(x>480) lv_obj_add_flag(parts[j],LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(parts[j],LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_pos(pipe_top[i],x,100);lv_obj_set_height(pipe_top[i],top-100);
        lv_obj_set_pos(pipe_bottom[i],x,bottom);lv_obj_set_height(pipe_bottom[i],408-bottom);
        lv_obj_set_pos(pipe_top_cap[i],x-3,top-12);lv_obj_set_pos(pipe_bottom_cap[i],x-3,bottom);
    }
}
static uint16_t rgb565(uint32_t color) { return (uint16_t)(((color>>19)<<11)|(((color>>10)&63)<<5)|((color>>3)&31)); }
static void snake_cell(watch_cell_t cell,uint16_t color)
{
    int x=cell.x*16+1,y=cell.y*16+1;
    for(int dy=0;dy<14;++dy) for(int dx=0;dx<14;++dx) {
        if((dx<2 || dx>11) && (dy<2 || dy>11)) continue;
        snake_pixels[(y+dy)*288+x+dx]=color;
    }
}
static void snake_render(void)
{
    const watch_snake_state_t *s=watch_snake_vm_state();
    uint16_t bg=rgb565(0x061421),grid=rgb565(0x173046);
    for(unsigned y=0;y<288;++y) for(unsigned x=0;x<288;++x)
        snake_pixels[y*288+x]=(x%16==0 || y%16==0) ? grid : bg;
    if(s->phase!=WATCH_GAME_WON) snake_cell(s->food,rgb565(0xFF797F));
    for(unsigned i=s->length;i>0;--i) snake_cell(s->body[i-1],rgb565(i==1 ? 0xBCF5D0 : 0x80DEA7));
    int x=s->body[0].x*16,y=s->body[0].y*16;
    bool horizontal=s->direction==WATCH_RIGHT || s->direction==WATCH_LEFT;
    int a=(s->direction==WATCH_RIGHT || s->direction==WATCH_DOWN) ? 10 : 4;
    for(int eye=0;eye<2;++eye) for(int j=0;j<2;++j) for(int k=0;k<2;++k) {
        int ex=horizontal ? a : 4+eye*6,ey=horizontal ? 4+eye*6 : a;
        snake_pixels[(y+ey+j)*288+x+ex+k]=bg;
    }
    lv_obj_invalidate(snake_canvas);score_label(snake_score,s->score);hint(snake_hint,s->phase,true);
    set_label(snake_pause,s->phase==WATCH_GAME_PAUSED ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
}
static void flappy_touch(lv_event_t *e)
{
    (void)e;const watch_flappy_state_t *s=watch_flappy_vm_state();
    bool starting=s->phase==WATCH_GAME_READY || s->phase==WATCH_GAME_OVER;
    if(s->phase==WATCH_GAME_OVER) watch_flappy_vm_new(lv_tick_get());
    if(starting) { arcade_accumulator=0;arcade_last_tick=lv_tick_get(); }
    watch_flappy_vm_flap();flappy_render();
}
static void snake_touch(lv_event_t *e)
{
    lv_indev_t *input=lv_indev_active();if(!input) return;
    lv_point_t point;lv_indev_get_point(input,&point);
    if(lv_event_get_code(e)==LV_EVENT_PRESSED) {
        snake_anchor=point;
        const watch_snake_state_t *s=watch_snake_vm_state();
        bool starting=s->phase==WATCH_GAME_READY || s->phase==WATCH_GAME_OVER || s->phase==WATCH_GAME_WON;
        if(s->phase==WATCH_GAME_OVER || s->phase==WATCH_GAME_WON) watch_snake_vm_new(lv_tick_get());
        if(starting) { arcade_last_tick=lv_tick_get();arcade_accumulator=0; }
        watch_snake_vm_start();snake_render();
    } else {
        int dx=point.x-snake_anchor.x,dy=point.y-snake_anchor.y;
        if(dx*dx+dy*dy>=12*12) {
            watch_snake_vm_turn(dx,dy);snake_anchor=point;
        }
    }
}
static void pause_clicked(lv_event_t *e)
{
    unsigned screen=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(screen==FLAPPY) { watch_flappy_vm_pause();flappy_render(); }
    else { watch_snake_vm_pause();snake_render(); }
    arcade_accumulator=0;arcade_last_tick=lv_tick_get();
}
static void arcade_tick(lv_timer_t *timer)
{
    (void)timer;uint32_t now=lv_tick_get(),elapsed=now-arcade_last_tick;arcade_last_tick=now;
    lv_obj_t *screen=lv_screen_active();
    if(!display_awake || displayed_notification!=-2 || (screen!=screens[FLAPPY] && screen!=screens[SNAKE])) {
        arcade_accumulator=0;return;
    }
    bool flappy=screen==screens[FLAPPY];
    watch_game_phase_t phase=flappy ? watch_flappy_vm_state()->phase : watch_snake_vm_state()->phase;
    if(phase!=WATCH_GAME_RUNNING) { arcade_accumulator=0;return; }
    /* Bound catch-up after a debugger stop; ordinary slow frames retain elapsed time. */
    arcade_accumulator+=elapsed>160 ? 160 : elapsed;
    unsigned interval=flappy ? 20 : 160-(watch_snake_vm_state()->score>40 ? 80 : watch_snake_vm_state()->score*2);
    bool changed=false;
    while(arcade_accumulator>=interval) {
        arcade_accumulator-=interval;
        if(flappy) watch_flappy_vm_step();else watch_snake_vm_step();changed=true;
    }
    if(changed) { if(flappy) flappy_render();else snake_render(); }
}
void arcade_leave(unsigned screen)
{
    if(screen==FLAPPY && watch_flappy_vm_state()->phase==WATCH_GAME_RUNNING) watch_flappy_vm_pause();
    if(screen==SNAKE && watch_snake_vm_state()->phase==WATCH_GAME_RUNNING) watch_snake_vm_pause();
    arcade_accumulator=0;
}
void flappy_open(void)
{
    watch_flappy_vm_new(lv_tick_get());arcade_last_tick=lv_tick_get();arcade_accumulator=0;
    flappy_render();watch_navigation_show(FLAPPY);
}
void snake_open(void)
{
    watch_snake_vm_new(lv_tick_get());arcade_last_tick=lv_tick_get();arcade_accumulator=0;
    snake_render();watch_navigation_show(SNAKE);
}
void arcade_init(void)
{
    page_header(FLAPPY,"",MENU);page_header(SNAKE,"",MENU);
    lv_obj_t *sky=lv_image_create(screens[FLAPPY]);lv_image_set_src(sky,&flappy_sky);lv_obj_remove_flag(sky,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(screens[SNAKE],lv_color_hex(0x070D18),0);
    for(unsigned i=0;i<3;++i) {
        pipe_top[i]=plain(screens[FLAPPY],0,100,46,100,0x77CD89,4);
        pipe_bottom[i]=plain(screens[FLAPPY],0,300,46,108,0x77CD89,4);
        pipe_top_cap[i]=plain(screens[FLAPPY],0,0,52,12,0xFFF0C7,4);
        pipe_bottom_cap[i]=plain(screens[FLAPPY],0,0,52,12,0xFFF0C7,4);
    }
    flappy_bird=lv_image_create(screens[FLAPPY]);lv_image_set_src(flappy_bird,&bird_sprite);lv_obj_remove_flag(flappy_bird,LV_OBJ_FLAG_CLICKABLE);
    flappy_score=label_at(screens[FLAPPY],"00",44,&lv_font_montserrat_48);lv_obj_set_style_text_color(flappy_score,lv_color_hex(0xFFF0C7),0);
    flappy_pause=round_action(screens[FLAPPY],LV_SYMBOL_PAUSE,100,57,48,0x24445C,pause_clicked,(void *)(uintptr_t)FLAPPY);
    flappy_hint=label_at(screens[FLAPPY],"",320,&lv_font_montserrat_20);lv_obj_set_style_text_align(flappy_hint,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(flappy_hint,lv_color_hex(0xFFF0C7),0);
    snake_canvas=lv_canvas_create(screens[SNAKE]);lv_canvas_set_buffer(snake_canvas,snake_pixels,288,288,LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(snake_canvas,96,104);lv_obj_remove_flag(snake_canvas,LV_OBJ_FLAG_CLICKABLE);
    snake_score=label_at(screens[SNAKE],"00",44,&lv_font_montserrat_48);lv_obj_set_style_text_color(snake_score,lv_color_hex(0xBCF5D0),0);
    snake_pause=round_action(screens[SNAKE],LV_SYMBOL_PAUSE,100,57,48,0x24445C,pause_clicked,(void *)(uintptr_t)SNAKE);
    snake_hint=label_at(screens[SNAKE],"",410,&lv_font_montserrat_16);lv_obj_set_style_text_align(snake_hint,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(snake_hint,lv_color_hex(0xBCF5D0),0);
    lv_obj_add_event_cb(screens[FLAPPY],flappy_touch,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(screens[SNAKE],snake_touch,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(screens[SNAKE],snake_touch,LV_EVENT_PRESSING,NULL);
    watch_flappy_vm_new(1);watch_snake_vm_new(1);flappy_render();snake_render();
    arcade_last_tick=lv_tick_get();lv_timer_create(arcade_tick,20,NULL);
}
