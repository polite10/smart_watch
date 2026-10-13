#include "views/watch_view_internal.h"
#include "viewmodels/watch_arcade_vm.h"
#include "views/watch_art.h"
#include "views/watch_fonts.h"
#include <math.h>
#include "lvgl/src/libs/lz4/lz4.h"
#include "views/watch_flappy_scenery.h"
#include "views/watch_folder_frames.h"
#include "views/watch_game_backdrops.h"

_Static_assert(WATCH_SCENERY_BYTES <= WATCH_FOLDER_FRAME_SIZE * WATCH_FOLDER_FRAME_SIZE * 3,
    "Flappy scenery must fit the shared folder cache");

static lv_obj_t *flappy_bird,*flappy_score,*flappy_best,*flappy_hint,*flappy_pause;
static lv_obj_t *pipe_top[3],*pipe_bottom[3],*pipe_top_cap[3],*pipe_bottom_cap[3];
static lv_obj_t *clouds[3],*hills[3],*near_hills[3],*bees[WATCH_FLAPPY_BEES],*pipe_flowers[3],*ground_details[1];
static uint32_t scenery_steps;
static lv_image_dsc_t scenery_pictures[WATCH_SCENERY_COUNT];
static lv_obj_t *game_backgrounds[2];
static lv_image_dsc_t game_background_picture;
_Static_assert(WATCH_GAME_BACKDROP_BYTES<=WATCH_FOLDER_FRAME_SIZE*WATCH_FOLDER_FRAME_SIZE*3,"Game backdrop fits shared cache");
void watch_game_background_init(unsigned screen)
{
    unsigned index=screen==SNAKE ? 1 : 0;
    game_background_picture=(lv_image_dsc_t){
        .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=320,.h=320,.stride=640},
        .data_size=WATCH_GAME_BACKDROP_BYTES,.data=watch_menu_scenery_buffer()};
    lv_obj_t *image=lv_image_create(screens[screen]);game_backgrounds[index]=image;
    lv_image_set_src(image,&game_background_picture);lv_obj_set_pos(image,0,0);lv_obj_set_size(image,480,480);
    lv_image_set_inner_align(image,LV_IMAGE_ALIGN_STRETCH);
    lv_obj_remove_flag(image,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
}
void watch_game_background_load(unsigned screen)
{
    unsigned index=screen==SNAKE ? 1 : 0;
    void *pixels=watch_menu_scenery_buffer();
    int decoded=LZ4_decompress_safe((const char *)watch_game_backdrops[index],pixels,
        watch_game_backdrop_sizes[index],WATCH_GAME_BACKDROP_BYTES);
    if(decoded==WATCH_GAME_BACKDROP_BYTES)lv_obj_invalidate(game_backgrounds[index]);
}
static void scenery_cache(void)
{
    uint8_t *pixels=watch_menu_scenery_buffer();
    int size=LZ4_decompress_safe((const char *)watch_flappy_scenery,
        (char *)pixels,watch_flappy_scenery_size,WATCH_SCENERY_BYTES);
    if(size!=WATCH_SCENERY_BYTES) return;
    uint32_t offset=0;
    for(unsigned i=0;i<WATCH_SCENERY_COUNT;++i) {
        unsigned w=watch_scenery_widths[i],h=watch_scenery_heights[i];
        scenery_pictures[i]=(lv_image_dsc_t){
            .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565A8,.w=w,.h=h,.stride=w*2},
            .data_size=w*h*3,.data=pixels+offset};
        offset+=w*h*3;
    }
    for(unsigned i=0;i<3;++i) {
        lv_image_set_src(clouds[i],&scenery_pictures[0]);
        lv_image_set_src(hills[i],&scenery_pictures[1]);
        lv_image_set_src(near_hills[i],&scenery_pictures[2]);
    }
    for(unsigned i=0;i<WATCH_FLAPPY_BEES;++i)lv_image_set_src(bees[i],&scenery_pictures[5]);
    for(unsigned i=0;i<3;++i)lv_image_set_src(pipe_flowers[i],&scenery_pictures[i%2 ? 3 : 4]);
    lv_image_set_src(ground_details[0],&scenery_pictures[7]);
}
static lv_obj_t *snake_canvas,*snake_score,*snake_best,*snake_hint,*snake_pause;
static uint16_t snake_pixels[288*288] __attribute__((aligned(4)));
static uint32_t arcade_last_tick,arcade_accumulator;
static lv_point_t snake_anchor,snake_origin;
static bool snake_dragged,snake_contact,snake_picture_valid,snake_food_visible;
static watch_cell_t snake_from[WATCH_SNAKE_CELLS],snake_picture_food;
static lv_point_t snake_drawn[WATCH_SNAKE_CELLS];
static unsigned snake_drawn_length;
static watch_direction_t snake_drawn_direction;
static watch_direction_t snake_from_direction;
static unsigned snake_bug_frame;
volatile uint32_t smartwatch_snake_pixel_writes;

static lv_obj_t *plain(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *obj=lv_obj_create(parent);lv_obj_set_pos(obj,x,y);lv_obj_set_size(obj,w,h);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj,lv_color_hex(color),0);lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_radius(obj,radius,0);lv_obj_set_style_pad_all(obj,0,0);
    return obj;
}
static void scenery_render(void)
{
    for(unsigned i=0;i<3;++i) {
        lv_obj_set_pos(clouds[i],(int)((i*220+660-scenery_steps*16/10%660)%660)-128,126+(int)i*49);
        lv_obj_set_pos(hills[i],(int)((i*240+720-scenery_steps*7/10%720)%720)-270,260+(i%2)*6);
        lv_obj_set_pos(near_hills[i],(int)((i*240+720-scenery_steps*19/10%720)%720)-270,336+(i%2)*5);
    }
    if(ground_details[0])lv_obj_set_x(ground_details[0],-(int)(scenery_steps*27/10%96));
}
static void scenery_init(void)
{
    for(unsigned i=0;i<3;++i) {
        clouds[i]=lv_image_create(screens[FLAPPY]);lv_obj_remove_flag(clouds[i],LV_OBJ_FLAG_CLICKABLE);
        hills[i]=lv_image_create(screens[FLAPPY]);lv_obj_remove_flag(hills[i],LV_OBJ_FLAG_CLICKABLE);
        near_hills[i]=lv_image_create(screens[FLAPPY]);lv_obj_remove_flag(near_hills[i],LV_OBJ_FLAG_CLICKABLE);
    }
    /* Preserve layer order: all clouds, then distant hills, then near hills. */
    for(unsigned i=0;i<3;++i)lv_obj_move_foreground(clouds[i]);
    for(unsigned i=0;i<3;++i)lv_obj_move_foreground(hills[i]);
    for(unsigned i=0;i<3;++i)lv_obj_move_foreground(near_hills[i]);
    plain(screens[FLAPPY],0,408,480,72,0xEFCB8B,0);plain(screens[FLAPPY],0,408,480,5,0xFFF2CC,0);
    /* One repeating strip keeps the ground in one dirty region, rather than
     * six separate GPU submissions. Its extra 96 pixels cover the wrap. */
    ground_details[0]=lv_image_create(screens[FLAPPY]);
    lv_obj_set_pos(ground_details[0],0,428);
    lv_obj_remove_flag(ground_details[0],LV_OBJ_FLAG_CLICKABLE);
    for(unsigned i=0;i<WATCH_FLAPPY_BEES;++i) {
        bees[i]=lv_image_create(screens[FLAPPY]);lv_obj_remove_flag(bees[i],LV_OBJ_FLAG_CLICKABLE);
        lv_image_set_pivot(bees[i],14,11);
    }
    scenery_render();
}
static void score_label(lv_obj_t *label,unsigned score)
{
    char text[16];snprintf(text,sizeof text,"%02u",score);set_label(label,text);
}
static void best_label(lv_obj_t *label,unsigned score)
{
    char text[48];snprintf(text,sizeof text,watch_games_vm_records_saved() ? "En iyi: %u" : "En iyi: %u / Kayıt hatası",score);set_label(label,text);
}
static void hint(lv_obj_t *label,watch_game_phase_t phase,bool snake)
{
    const char *text=phase==WATCH_GAME_READY ? (snake ? "Dokun veya kaydır" : "Dokun ve uç") :
        phase==WATCH_GAME_PAUSED ? "Duraklatıldı" : phase==WATCH_GAME_OVER ? "Oyun bitti\nTekrar dokun" :
        phase==WATCH_GAME_WON ? "Tebrikler!\nTekrar dokun" : "";
    set_label(label,text);
    if(*text) lv_obj_remove_flag(label,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(label,LV_OBJ_FLAG_HIDDEN);
}
static void save_records(lv_obj_t *label)
{
    if(!watch_games_vm_save_records()) set_label(label,"Rekor kaydedilemedi");
}
static void flappy_render(void)
{
    const watch_flappy_state_t *s=watch_flappy_vm_state();score_label(flappy_score,s->score);
    best_label(flappy_best,watch_games_vm_records()->flappy_score);hint(flappy_hint,s->phase,false);
    set_label(flappy_pause,s->phase==WATCH_GAME_PAUSED ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    lv_obj_set_pos(flappy_bird,142,(int)s->y-28);
    /* Velocity changes by 0.25 per 20 ms step, giving a gentle 1.5 degree tilt.
     * Keep the pose when paused and reset it when a new game is ready. */
    int rotation=s->phase==WATCH_GAME_READY ? 0 : (int)(s->velocity*60.0f);
    if(rotation < -240) rotation=-240;
    if(rotation > 300) rotation=300;
    lv_image_set_rotation(flappy_bird,rotation);
    scenery_render();
    for(unsigned i=0;i<WATCH_FLAPPY_BEES;++i) {
        float x,y;
        if(watch_flappy_bee(s,i,&x,&y)) {
            lv_image_set_src(bees[i],&scenery_pictures[5+(s->steps/4+i)%2]);
            lv_obj_set_pos(bees[i],(int)x-14,(int)y-11);
            lv_image_set_rotation(bees[i],(int)(70*sinf((s->steps%6000)*0.07f+i)));
            lv_obj_remove_flag(bees[i],LV_OBJ_FLAG_HIDDEN);
        } else lv_obj_add_flag(bees[i],LV_OBJ_FLAG_HIDDEN);
    }
    for(unsigned i=0;i<3;++i) {
        int x=(int)s->pipes[i].x,top=(int)s->pipes[i].gap_y-WATCH_FLAPPY_GAP_HALF;
        int bottom=(int)s->pipes[i].gap_y+WATCH_FLAPPY_GAP_HALF;
        lv_obj_t *parts[]={pipe_top[i],pipe_bottom[i],pipe_top_cap[i],pipe_bottom_cap[i]};
        for(unsigned j=0;j<4;++j) {
            if(x>480) lv_obj_add_flag(parts[j],LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(parts[j],LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_pos(pipe_top[i],x,100);lv_obj_set_height(pipe_top[i],top-100);
        lv_obj_set_pos(pipe_bottom[i],x,bottom);lv_obj_set_height(pipe_bottom[i],408-bottom);
        lv_obj_set_pos(pipe_top_cap[i],x-3,top-12);lv_obj_set_pos(pipe_bottom_cap[i],x-3,bottom);
        float fx,fy;
        if(pipe_flowers[i]) {
            bool flower=watch_flappy_flower(i,&s->pipes[i],&fx,&fy);
            if(!flower || x>480)lv_obj_add_flag(pipe_flowers[i],LV_OBJ_FLAG_HIDDEN);
            else {
                lv_obj_set_pos(pipe_flowers[i],(int)fx-WATCH_FLAPPY_FLOWER_RADIUS,(int)fy-WATCH_FLAPPY_FLOWER_RADIUS);
                lv_obj_remove_flag(pipe_flowers[i],LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}
static uint16_t rgb565(uint32_t color)
{ return (uint16_t)(((color>>19)<<11)|(((color>>10)&63)<<5)|((color>>3)&31)); }
static int snake_wrap_pixel(int value)
{ return value<0 ? value+288 : value>=288 ? value-288 : value; }
static uint16_t snake_ground_pixel(unsigned x,unsigned y)
{
    unsigned tx=x/16,ty=y/16,dx=x%16,dy=y%16,hash=tx*37+ty*19;
    const uint32_t earth[]={0x586443,0x5C6745,0x626C49,0x566141};
    uint32_t color=earth[((x*13)^(y*29)^(x*y*7))%4];
    /* Fine soil grain and low grass keep the board readable; taller ferns
     * live outside the playable area in the garden backdrop. */
    if(hash%9==0 && dy>=6 && dy<=12) {
        if(dx==5+(12-dy)/2 || dx==10-(12-dy)/3 || (dx==8 && dy>=8))
            color=dy<10 ? 0x9BAC66 : 0x71894E;
    }
    if(hash%13==0 && dx>=5 && dx<=8 && dy>=7 && dy<=9)color=dy==7 ? 0xA19D77 : 0x7C8060;
    return rgb565(color);
}
static void snake_bug(watch_cell_t cell,unsigned frame)
{
    for(int y=1;y<16;++y)for(int x=0;x<16;++x) {
        uint32_t color=0;
        int dx=x-8,dy=y-9,leg=frame%2;
        if((x>=1 && x<=4) || (x>=12 && x<=15)) {
            if(y==5+leg || y==9 || y==13-leg)color=0x151F1B;
            if((x==1 || x==15) && (y==4+leg || y==14-leg))color=0xC3B18B;
        }
        if(dx*dx*2+dy*dy<39)color=dx< -2 ? 0xF3B871 : dx<1 ? 0xDCA15F : 0xAC653E;
        if(dx*dx*2+dy*dy>=29 && dx*dx*2+dy*dy<39)color=0x523F2D;
        if(x==8 && y>=5 && y<=14)color=0x533C29;
        if((x==5 || x==11) && (y==8 || y==11))color=0x453726;
        if(x==5 && y==6)color=0xFFE0A0;
        if(y>=2 && y<=4 && x>=5 && x<=10)color=0x223B30;
        if(y==1 && (x==3 || x==12))color=0x17251E;
        if(y==3 && (x==6 || x==9))color=0xF1EACB;
        if(color)snake_pixels[(cell.y*16+y)*288+cell.x*16+x]=rgb565(color);
    }
}
static int snake_motion(int from,int to,unsigned fraction)
{
    int delta=to-from;
    if(delta>1) delta-=WATCH_SNAKE_SIDE;
    else if(delta<-1) delta+=WATCH_SNAKE_SIDE;
    return from*16+delta*16*(int)fraction/256;
}
static void snake_invalidate(int x,int y)
{
    /* A segment crossing a seam dirties two edges, never the middle of the board. */
    x=snake_wrap_pixel(x);y=snake_wrap_pixel(y);
    int widths[]={288-x<16 ? 288-x : 16,0};widths[1]=16-widths[0];
    int heights[]={288-y<16 ? 288-y : 16,0};heights[1]=16-heights[0];
    for(unsigned row=0;row<2;++row) for(unsigned col=0;col<2;++col) {
        if(!widths[col] || !heights[row]) continue;
        int left=col ? 0 : x,top=row ? 0 : y;
        lv_area_t area={96+left,104+top,96+left+widths[col]-1,104+top+heights[row]-1};
        lv_obj_invalidate_area(snake_canvas,&area);
    }
}
static void snake_clear(int x,int y)
{
    for(int dy=0;dy<16;++dy) for(int dx=0;dx<16;++dx) {
        unsigned px=snake_wrap_pixel(x+dx),py=snake_wrap_pixel(y+dy);
        snake_pixels[py*288+px]=snake_ground_pixel(px,py);
        ++smartwatch_snake_pixel_writes;
    }
    snake_invalidate(x,y);
}
static void snake_pixel(int x,int y,uint32_t color)
{
    snake_pixels[snake_wrap_pixel(y)*288+snake_wrap_pixel(x)]=rgb565(color);
    ++smartwatch_snake_pixel_writes;
}
static int snake_delta(int from,int to)
{
    int delta=to-from;
    if(delta>144)delta-=288;
    if(delta< -144)delta+=288;
    return delta;
}
static void snake_tube(lv_point_t tail,lv_point_t front,unsigned radius,unsigned skin)
{
    int vx=snake_delta(tail.x,front.x),vy=snake_delta(tail.y,front.y);
    int length=vx*vx+vy*vy;
    int x1=(vx<0 ? vx : 0)-(int)radius,x2=(vx>0 ? vx : 0)+(int)radius;
    int y1=(vy<0 ? vy : 0)-(int)radius,y2=(vy>0 ? vy : 0)+(int)radius;
    for(int y=y1;y<=y2;++y)for(int x=x1;x<=x2;++x) {
        int along=length ? (x*vx+y*vy)*256/length : 0;
        if(along<0)along=0;
        if(along>256)along=256;
        int dx=x-vx*along/256,dy=y-vy*along/256,dist=dx*dx+dy*dy;
        if(dist>(int)(radius*radius))continue;
        uint32_t color=dist>(int)((radius-1)*(radius-1)) ? 0x244A34 :
            dy>2 ? 0x387347 : dy< -2 ? 0xA2C979 : 0x69AA5B;
        if(dist<9 && ((x+y*2+(int)skin*3)&7)==0)color=0x4E8A4C;
        snake_pixel(tail.x+8+x,tail.y+8+y,color);
    }
}
static void snake_head(lv_point_t position,float angle)
{
    float c=cosf(angle),s=sinf(angle);
    for(int y=-7;y<=7;++y)for(int x=-7;x<=7;++x) {
        float hx=x*c+y*s,hy=-x*s+y*c;
        if(hx*hx+hy*hy*1.4f>49)continue;
        uint32_t color=y>3 ? 0x3D7744 : y< -2 ? 0xB9D88B : 0x87BD6F;
        if(hx>1 && hx<4 && (hy>2 || hy< -2))color=0xE9E8BB;
        if(hx>2 && hx<4 && (hy>3 || hy< -3))color=0x192F27;
        if(hx>5 && hy> -1 && hy<1)color=0x39513C;
        snake_pixel(position.x+8+x,position.y+8+y,color);
    }
}
static void snake_food_draw(watch_cell_t food)
{
    int cx=food.x*16+8,cy=food.y*16+9;
    for(int y=-5;y<=5;++y)for(int x=-5;x<=5;++x)if(x*x+y*y<=25)
        snake_pixel(cx+x,cy+y,x< -1 && y<0 ? 0xFFB095 : y>2 ? 0xA74640 : 0xE76F59);
    snake_pixel(cx,cy-6,0x57462E);snake_pixel(cx+1,cy-7,0xA1BF71);
}
static void snake_render(unsigned fraction)
{
    const watch_snake_state_t *s=watch_snake_vm_state();
    lv_point_t positions[WATCH_SNAKE_CELLS];
    smartwatch_snake_pixel_writes=0;
    bool changed=!snake_picture_valid || snake_drawn_length!=s->length || snake_drawn_direction!=s->direction;
    unsigned bug_frame=s->phase==WATCH_GAME_RUNNING ? lv_tick_get()/160 : snake_bug_frame;
    bool bugs_changed=bug_frame!=snake_bug_frame;
    bool food=s->phase!=WATCH_GAME_WON;
    changed|=snake_food_visible!=food || (food && (snake_picture_food.x!=s->food.x || snake_picture_food.y!=s->food.y));
    for(unsigned i=0;i<s->length;++i) {
        positions[i].x=snake_motion(snake_from[i].x,s->body[i].x,fraction);
        positions[i].y=snake_motion(snake_from[i].y,s->body[i].y,fraction);
        if(i>=snake_drawn_length || positions[i].x!=snake_drawn[i].x || positions[i].y!=snake_drawn[i].y) changed=true;
    }
    if(changed || bugs_changed) {
        if(!snake_picture_valid) {
            for(unsigned y=0;y<288;++y)for(unsigned x=0;x<288;++x)snake_pixels[y*288+x]=snake_ground_pixel(x,y);
            smartwatch_snake_pixel_writes=288*288;
            lv_obj_invalidate(snake_canvas);
        } else {
            for(unsigned i=0;i<snake_drawn_length;++i) snake_clear(snake_drawn[i].x,snake_drawn[i].y);
            if(snake_food_visible) snake_clear(snake_picture_food.x*16,snake_picture_food.y*16);
            if(bugs_changed)for(unsigned i=0;i<s->bug_count;++i)snake_clear(s->bugs[i].x*16,s->bugs[i].y*16);
        }
        for(unsigned i=0;i<s->bug_count;++i)snake_bug(s->bugs[i],bug_frame+i);
        if(food) snake_food_draw(s->food);
        for(unsigned i=s->length-1;i>0;--i) {
            snake_tube(positions[i],positions[i-1],i==s->length-1 ? 4 : 6,i);
        }
        int turn=(int)s->direction-(int)snake_from_direction;
        if(turn>2)turn-=4;
        if(turn< -2)turn+=4;
        snake_head(positions[0],(snake_from_direction+turn*fraction/256.0f)*1.57079633f);
        for(unsigned i=0;i<s->length;++i)snake_invalidate(positions[i].x,positions[i].y);
        memcpy(snake_drawn,positions,s->length*sizeof *positions);
        snake_drawn_length=s->length;snake_drawn_direction=s->direction;
        snake_picture_food=s->food;snake_food_visible=food;snake_picture_valid=true;
        snake_bug_frame=bug_frame;
    }
    score_label(snake_score,s->score);best_label(snake_best,watch_games_vm_records()->snake_score);
    hint(snake_hint,s->phase,true);
    set_label(snake_pause,s->phase==WATCH_GAME_PAUSED ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
}
static void snake_new(void)
{
    save_records(snake_best);watch_snake_vm_new(lv_tick_get());
    const watch_snake_state_t *s=watch_snake_vm_state();memcpy(snake_from,s->body,s->length*sizeof *snake_from);
    snake_from_direction=s->direction;snake_bug_frame=0;
    snake_picture_valid=false;arcade_last_tick=lv_tick_get();arcade_accumulator=0;
    snake_render(256);
}
static void flappy_touch(lv_event_t *e)
{
    (void)e;const watch_flappy_state_t *s=watch_flappy_vm_state();
    bool starting=s->phase==WATCH_GAME_READY || s->phase==WATCH_GAME_OVER;
    if(s->phase==WATCH_GAME_OVER) { save_records(flappy_best);watch_flappy_vm_new(lv_tick_get()); }
    if(starting) { arcade_accumulator=0;arcade_last_tick=lv_tick_get(); }
    watch_flappy_vm_flap();flappy_render();
}
static void snake_touch(lv_event_t *e)
{
    lv_indev_t *input=lv_indev_active();if(!input) return;
    lv_point_t point;lv_indev_get_point(input,&point);
    lv_event_code_t code=lv_event_get_code(e);
    if(code==LV_EVENT_PRESSED) {
        snake_anchor=snake_origin=point;snake_dragged=false;snake_contact=true;
        const watch_snake_state_t *s=watch_snake_vm_state();
        bool starting=s->phase==WATCH_GAME_READY || s->phase==WATCH_GAME_OVER || s->phase==WATCH_GAME_WON;
        if(s->phase==WATCH_GAME_OVER || s->phase==WATCH_GAME_WON) snake_new();
        if(starting) {
            arcade_last_tick=lv_tick_get();arcade_accumulator=0;watch_snake_vm_start();snake_render(256);
        }
    } else if(code==LV_EVENT_PRESSING && snake_contact) {
        int dx=point.x-snake_anchor.x,dy=point.y-snake_anchor.y;
        if(dx*dx+dy*dy>=12*12) { snake_dragged=true;watch_snake_vm_turn(dx,dy);snake_anchor=point; }
    } else if(code==LV_EVENT_RELEASED) {
        int dx=point.x-snake_origin.x,dy=point.y-snake_origin.y;
        if(snake_contact && !snake_dragged && dx*dx+dy*dy<12*12) {
            dx=point.x-240;dy=point.y-248;
            /* The dominant quadrant selects a direction; the center is neutral. */
            if(dx*dx+dy*dy>=32*32) watch_snake_vm_turn(dx,dy);
        }
        snake_contact=false;
    }
}
static void pause_clicked(lv_event_t *e)
{
    unsigned screen=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(screen==FLAPPY) { watch_flappy_vm_pause();flappy_render();save_records(flappy_best); }
    else {
        watch_snake_vm_pause();snake_render(256);save_records(snake_best);
    }
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
    if(flappy && phase==WATCH_GAME_READY) {
        arcade_accumulator+=elapsed>160 ? 160 : elapsed;
        bool changed=false;
        while(arcade_accumulator>=20) { arcade_accumulator-=20;++scenery_steps;changed=true; }
        if(changed)scenery_render();
        return;
    }
    if(phase!=WATCH_GAME_RUNNING) { arcade_accumulator=0;return; }
    arcade_accumulator+=elapsed>160 ? 160 : elapsed;
    unsigned interval=flappy ? 20 : 160-(watch_snake_vm_state()->score>40 ? 80 : watch_snake_vm_state()->score*2);
    bool changed=false;
    while(arcade_accumulator>=interval) {
        arcade_accumulator-=interval;
        if(flappy) { watch_flappy_vm_step();++scenery_steps; }
        else {
            const watch_snake_state_t *s=watch_snake_vm_state();
            snake_from_direction=s->direction;
            memcpy(snake_from,s->body,s->length*sizeof *snake_from);
            unsigned old_length=s->length;watch_snake_vm_step();
            s=watch_snake_vm_state();if(s->length>old_length) snake_from[old_length]=snake_from[old_length-1];
        }
        changed=true;
        watch_game_phase_t next=flappy ? watch_flappy_vm_state()->phase : watch_snake_vm_state()->phase;
        if(next!=WATCH_GAME_RUNNING) break;
    }
    if(flappy) { if(changed) flappy_render(); }
    else snake_render(watch_snake_vm_state()->phase==WATCH_GAME_RUNNING ? arcade_accumulator*256/interval : 256);
    watch_game_phase_t next=flappy ? watch_flappy_vm_state()->phase : watch_snake_vm_state()->phase;
    if(next!=phase) save_records(flappy ? flappy_best : snake_best);
}
void arcade_leave(unsigned screen)
{
    if(screen==FLAPPY && watch_flappy_vm_state()->phase==WATCH_GAME_RUNNING) watch_flappy_vm_pause();
    if(screen==SNAKE) {
        if(watch_snake_vm_state()->phase==WATCH_GAME_RUNNING) watch_snake_vm_pause();
        snake_contact=false;
    }
    save_records(screen==FLAPPY ? flappy_best : snake_best);arcade_accumulator=0;
}
void flappy_open(void)
{
    scenery_cache();
    scenery_steps=0;save_records(flappy_best);watch_flappy_vm_new(lv_tick_get());arcade_last_tick=lv_tick_get();arcade_accumulator=0;
    flappy_render();watch_navigation_show(FLAPPY);
}
void snake_open(void)
{
    watch_game_background_load(SNAKE);snake_contact=false;snake_new();
    watch_navigation_show(SNAKE);
}
void arcade_init(void)
{
    page_header(FLAPPY,"",MENU);watch_game_background_init(SNAKE);page_header(SNAKE,"",MENU);
    lv_obj_t *sky=lv_image_create(screens[FLAPPY]);lv_image_set_src(sky,&flappy_sky);lv_obj_remove_flag(sky,LV_OBJ_FLAG_CLICKABLE);
    scenery_init();
    lv_obj_set_style_bg_color(screens[SNAKE],lv_color_hex(0x243F35),0);
    for(unsigned i=0;i<3;++i) {
        pipe_top[i]=plain(screens[FLAPPY],0,100,46,100,0x77CD89,4);
        pipe_bottom[i]=plain(screens[FLAPPY],0,300,46,108,0x77CD89,4);
        pipe_top_cap[i]=plain(screens[FLAPPY],0,0,52,12,0xFFF0C7,4);
        pipe_bottom_cap[i]=plain(screens[FLAPPY],0,0,52,12,0xFFF0C7,4);
        pipe_flowers[i]=lv_image_create(screens[FLAPPY]);lv_obj_remove_flag(pipe_flowers[i],LV_OBJ_FLAG_CLICKABLE);
    }
    flappy_bird=lv_image_create(screens[FLAPPY]);lv_image_set_src(flappy_bird,&bird_sprite);lv_obj_remove_flag(flappy_bird,LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_pivot(flappy_bird,28,28);
    flappy_score=label_at(screens[FLAPPY],"00",34,&lv_font_montserrat_48);lv_obj_set_style_text_color(flappy_score,lv_color_hex(0xFFF0C7),0);
    flappy_best=label_at(screens[FLAPPY],"En iyi: 0",82,&watch_font_turkish_16);
    flappy_pause=round_action(screens[FLAPPY],LV_SYMBOL_PAUSE,100,57,48,0x24445C,pause_clicked,(void *)(uintptr_t)FLAPPY);
    flappy_hint=label_at(screens[FLAPPY],"",320,&watch_font_turkish_20);lv_obj_set_style_text_align(flappy_hint,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(flappy_hint,lv_color_hex(0xFFF0C7),0);
    snake_canvas=lv_canvas_create(screens[SNAKE]);lv_canvas_set_buffer(snake_canvas,snake_pixels,288,288,LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(snake_canvas,96,104);lv_obj_remove_flag(snake_canvas,LV_OBJ_FLAG_CLICKABLE);
    snake_score=label_at(screens[SNAKE],"00",34,&lv_font_montserrat_48);lv_obj_set_style_text_color(snake_score,lv_color_hex(0xBCF5D0),0);
    snake_best=label_at(screens[SNAKE],"En iyi: 0",82,&watch_font_turkish_16);
    snake_pause=round_action(screens[SNAKE],LV_SYMBOL_PAUSE,100,57,48,0x24445C,pause_clicked,(void *)(uintptr_t)SNAKE);
    snake_hint=label_at(screens[SNAKE],"",410,&watch_font_turkish_16);lv_obj_set_style_text_align(snake_hint,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(snake_hint,lv_color_hex(0xBCF5D0),0);
    lv_obj_add_event_cb(screens[FLAPPY],flappy_touch,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(screens[SNAKE],snake_touch,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(screens[SNAKE],snake_touch,LV_EVENT_PRESSING,NULL);
    lv_obj_add_event_cb(screens[SNAKE],snake_touch,LV_EVENT_RELEASED,NULL);
    watch_flappy_vm_new(1);watch_snake_vm_new(1);flappy_render();
    memcpy(snake_from,watch_snake_vm_state()->body,4*sizeof *snake_from);snake_render(256);
    arcade_last_tick=lv_tick_get();lv_timer_create(arcade_tick,20,NULL);
}
