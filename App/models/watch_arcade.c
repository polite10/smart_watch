#include "models/watch_arcade.h"
#include <string.h>
#include <math.h>

static uint32_t random_next(uint32_t *value)
{
    uint32_t x=*value ? *value : 0x9E3779B9u;
    x^=x<<13; x^=x>>17; x^=x<<5;
    return *value=x;
}
static bool occupied(const watch_snake_state_t *s,watch_cell_t cell,unsigned length)
{
    for(unsigned i=0;i<length;++i) if(s->body[i].x==cell.x && s->body[i].y==cell.y) return true;
    return false;
}
static void snake_food(watch_snake_state_t *s)
{
    unsigned empty=WATCH_SNAKE_CELLS-s->length-s->bug_count;
    if(!empty) { s->phase=WATCH_GAME_WON; return; }
    unsigned pick=random_next(&s->random)%empty;
    for(unsigned y=0;y<WATCH_SNAKE_SIDE;++y) for(unsigned x=0;x<WATCH_SNAKE_SIDE;++x) {
        watch_cell_t cell={(uint8_t)x,(uint8_t)y};
        bool bug=false;
        for(unsigned i=0;i<s->bug_count;++i)bug|=s->bugs[i].x==x && s->bugs[i].y==y;
        if(!bug && !occupied(s,cell,s->length) && pick--==0) { s->food=cell; return; }
    }
}
void watch_snake_new(watch_snake_state_t *s,uint32_t seed)
{
    memset(s,0,sizeof *s); s->random=seed; s->length=4;
    s->direction=s->queued=WATCH_RIGHT; s->phase=WATCH_GAME_READY;
    for(unsigned i=0;i<s->length;++i) s->body[i]=(watch_cell_t){(uint8_t)(8-i),9};
    /* One insect per separated region, clear of the initial route and body. */
    for(unsigned i=0;i<WATCH_SNAKE_BUGS;++i) {
        unsigned x=2+random_next(&s->random)%14;
        unsigned y=i==0 ? 2+random_next(&s->random)%3 : i==1 ? 12+random_next(&s->random)%4 : 6+random_next(&s->random)%2;
        s->bugs[s->bug_count++]=(watch_cell_t){x,y};
    }
    snake_food(s);
}
bool watch_snake_turn(watch_snake_state_t *s,int dx,int dy)
{
    if(s->phase!=WATCH_GAME_READY && s->phase!=WATCH_GAME_RUNNING) return false;
    int ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy;
    if(ax<12 && ay<12) return false;
    watch_direction_t next=ax>=ay ? (dx>0 ? WATCH_RIGHT : WATCH_LEFT) : (dy>0 ? WATCH_DOWN : WATCH_UP);
    if(next==s->direction || next==((s->direction+2)%4) || s->turn_queued) return false;
    s->queued=next; s->turn_queued=true;
    return true;
}
void watch_snake_step(watch_snake_state_t *s)
{
    if(s->phase!=WATCH_GAME_RUNNING) return;
    if(s->turn_queued) { s->direction=s->queued; s->turn_queued=false; }
    int x=s->body[0].x,y=s->body[0].y;
    if(s->direction==WATCH_RIGHT) ++x;
    else if(s->direction==WATCH_LEFT) --x;
    else if(s->direction==WATCH_DOWN) ++y;
    else --y;
    /* Opposite edges join; insects and the occupied body end a running game. */
    x=(x+WATCH_SNAKE_SIDE)%WATCH_SNAKE_SIDE;
    y=(y+WATCH_SNAKE_SIDE)%WATCH_SNAKE_SIDE;
    watch_cell_t head={(uint8_t)x,(uint8_t)y};
    for(unsigned i=0;i<s->bug_count;++i)if(s->bugs[i].x==head.x && s->bugs[i].y==head.y) {
        s->phase=WATCH_GAME_OVER;return;
    }
    bool eating=head.x==s->food.x && head.y==s->food.y;
    /* The old tail vacates this tick unless the snake grows. */
    if(occupied(s,head,s->length-(eating ? 0 : 1))) { s->phase=WATCH_GAME_OVER; return; }
    if(eating) { ++s->length; ++s->score; }
    for(unsigned i=s->length-1;i>0;--i) s->body[i]=s->body[i-1];
    s->body[0]=head;
    if(eating) snake_food(s);
}
void watch_snake_pause(watch_snake_state_t *s)
{
    if(s->phase==WATCH_GAME_RUNNING) s->phase=WATCH_GAME_PAUSED;
    else if(s->phase==WATCH_GAME_PAUSED) s->phase=WATCH_GAME_RUNNING;
}
static float gap(uint32_t *random) { return 190.0f+(float)(random_next(random)%101); }
static void pipe_spawn(watch_flappy_state_t *s,watch_pipe_t *p,float x)
{
    *p=(watch_pipe_t){.x=x,.gap_y=gap(&s->random)};
    p->flower=++s->obstacle_count==s->next_flower;
    p->bee=!p->flower && s->obstacle_count%3==0;
    if(p->flower)s->next_flower+=2+random_next(&s->random)%4;
}
void watch_flappy_new(watch_flappy_state_t *s,uint32_t seed)
{
    memset(s,0,sizeof *s); s->random=seed; s->y=235; s->phase=WATCH_GAME_READY;
    s->next_flower=2+random_next(&s->random)%4;
    for(unsigned i=0;i<WATCH_FLAPPY_PIPES;++i)pipe_spawn(s,&s->pipes[i],350.0f+WATCH_FLAPPY_SPACING*i);
}
void watch_flappy_flap(watch_flappy_state_t *s)
{
    if(s->phase==WATCH_GAME_READY) s->phase=WATCH_GAME_RUNNING;
    if(s->phase==WATCH_GAME_RUNNING) s->velocity=-4.4f;
}
/* The free corridor between pipes is 129 px wide. Its center leaves flowers
 * 64 px from both pipe walls, including their projecting caps. */
bool watch_flappy_flower(unsigned index,const watch_pipe_t *pipe,float *x,float *y)
{
    if(!pipe->flower || pipe->collected)return false;
    (void)index;
    *x=pipe->x+110;
    *y=pipe->gap_y;
    return true;
}
bool watch_flappy_bee(const watch_flappy_state_t *s,unsigned index,float *x,float *y)
{
    for(unsigned i=0;i<WATCH_FLAPPY_PIPES;++i)if(s->pipes[i].bee) {
        if(index--)continue;
        *x=s->pipes[i].x+110;
        /* A single side lane leaves the center route free. Flower corridors
         * never contain bees. Both items scroll with their owning obstacle. */
        *y=s->pipes[i].gap_y+(i%2 ? -32 : 32)+6*sinf((s->steps%6000)*0.07f+i);
        return true;
    }
    return false;
}
static bool bird_touches(float y,float x,float item_y,unsigned radius)
{
    float dx=x<156 ? 156-x : x>184 ? x-184 : 0;
    float dy=item_y<y-12 ? y-12-item_y : item_y>y+12 ? item_y-y-12 : 0;
    return dx*dx+dy*dy<radius*radius;
}
void watch_flappy_step(watch_flappy_state_t *s)
{
    if(s->phase!=WATCH_GAME_RUNNING) return;
    /* Fixed 20 ms simulation step: independent of render time. */
    ++s->steps;
    s->velocity+=0.25f; s->y+=s->velocity;
    if(s->y<111 || s->y>393) { s->phase=WATCH_GAME_OVER; return; }
    for(unsigned i=0;i<WATCH_FLAPPY_PIPES;++i) {
        watch_pipe_t *p=&s->pipes[i]; p->x-=2.2f;
        if(p->x<24) {
            float furthest=0;
            for(unsigned j=0;j<WATCH_FLAPPY_PIPES;++j) if(s->pipes[j].x>furthest) furthest=s->pipes[j].x;
            pipe_spawn(s,p,furthest+WATCH_FLAPPY_SPACING);
        }
        /* Bird center x=170, collision half extents 14/12; gap matches the view. */
        if(p->x<184 && p->x+46>156 && (s->y-12<p->gap_y-WATCH_FLAPPY_GAP_HALF || s->y+12>p->gap_y+WATCH_FLAPPY_GAP_HALF)) {
            s->phase=WATCH_GAME_OVER; return;
        }
        float fx,fy;
        if(watch_flappy_flower(i,p,&fx,&fy)) {
            if(bird_touches(s->y,fx,fy,WATCH_FLAPPY_FLOWER_RADIUS)) { p->collected=true;++s->score; }
        }
        if(!p->scored && p->x+46<156) { p->scored=true; ++s->score; }
    }
    for(unsigned i=0;i<WATCH_FLAPPY_BEES;++i) {
        float x,y;
        if(watch_flappy_bee(s,i,&x,&y) && bird_touches(s->y,x,y,WATCH_FLAPPY_BEE_RADIUS)) { s->phase=WATCH_GAME_OVER;return; }
    }
}
void watch_flappy_pause(watch_flappy_state_t *s)
{
    if(s->phase==WATCH_GAME_RUNNING) s->phase=WATCH_GAME_PAUSED;
    else if(s->phase==WATCH_GAME_PAUSED) s->phase=WATCH_GAME_RUNNING;
}
