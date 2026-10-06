#include "models/watch_arcade.h"
#include "viewmodels/watch_arcade_vm.h"
#include "viewmodels/watch_ida_vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void same_cells(const watch_snake_state_t *s)
{
    for(unsigned i=0;i<s->length;++i) {
        assert(s->body[i].x<18 && s->body[i].y<18);
        assert(s->food.x!=s->body[i].x || s->food.y!=s->body[i].y);
        for(unsigned j=i+1;j<s->length;++j) assert(s->body[i].x!=s->body[j].x || s->body[i].y!=s->body[j].y);
    }
}
int main(void)
{
    watch_snake_state_t s;
    for(unsigned i=0;i<512;++i) { watch_snake_new(&s,i);assert(s.length==4 && s.score==0 && s.phase==WATCH_GAME_READY);same_cells(&s); }
    watch_snake_new(&s,2);s.phase=WATCH_GAME_RUNNING;s.food=(watch_cell_t){9,9};watch_snake_step(&s);
    assert(s.length==5 && s.score==1 && s.body[0].x==9);same_cells(&s);
    assert(!watch_snake_turn(&s,-60,0));assert(!watch_snake_turn(&s,2,3));
    assert(watch_snake_turn(&s,15,-50));assert(!watch_snake_turn(&s,0,60));watch_snake_step(&s);
    assert(s.direction==WATCH_UP && s.body[0].y==8);
    assert(watch_snake_turn(&s,-60,15));watch_snake_step(&s);assert(s.direction==WATCH_LEFT);watch_snake_step(&s);watch_snake_step(&s);
    assert(watch_snake_turn(&s,0,50));watch_snake_step(&s);assert(s.direction==WATCH_DOWN);
    watch_snake_pause(&s);watch_snake_state_t copy=s;watch_snake_step(&s);assert(!memcmp(&s,&copy,sizeof s));watch_snake_pause(&s);assert(s.phase==WATCH_GAME_RUNNING);
    watch_snake_new(&s,3);s.phase=WATCH_GAME_RUNNING;s.body[0].x=17;watch_snake_step(&s);assert(s.phase==WATCH_GAME_OVER);
    watch_snake_new(&s,4);s.phase=WATCH_GAME_RUNNING;s.direction=WATCH_UP;
    s.body[0]=(watch_cell_t){2,2};s.body[1]=(watch_cell_t){3,2};s.body[2]=(watch_cell_t){3,1};s.body[3]=(watch_cell_t){2,1};s.food=(watch_cell_t){10,10};
    watch_snake_step(&s);assert(s.phase==WATCH_GAME_RUNNING && s.body[0].y==1); /* old tail is legal */
    s.body[0]=(watch_cell_t){2,2};s.body[1]=(watch_cell_t){2,1};s.body[2]=(watch_cell_t){3,1};s.body[3]=(watch_cell_t){3,2};
    watch_snake_step(&s);assert(s.phase==WATCH_GAME_OVER);
    /* Hamiltonian body fills all but the next head cell; eating it wins safely. */
    watch_snake_new(&s,5);s.phase=WATCH_GAME_RUNNING;s.length=323;s.direction=WATCH_LEFT;
    s.body[0]=(watch_cell_t){1,0};unsigned at=1;
    for(unsigned y=0;y<18;++y) for(unsigned x=0;x<18;++x) if(!(y==0 && x<2)) s.body[at++]=(watch_cell_t){x,y};
    s.food=(watch_cell_t){0,0};watch_snake_step(&s);assert(s.phase==WATCH_GAME_WON && s.length==324);
    assert(!watch_snake_turn(&s,0,30));
    watch_flappy_state_t f;watch_flappy_new(&f,6);assert(f.phase==WATCH_GAME_READY);watch_flappy_flap(&f);assert(f.phase==WATCH_GAME_RUNNING && f.velocity<0);
    float old=f.y;watch_flappy_step(&f);assert(f.y<old);
    watch_flappy_pause(&f);watch_flappy_state_t frozen=f;watch_flappy_step(&f);assert(!memcmp(&f,&frozen,sizeof f));watch_flappy_pause(&f);assert(f.phase==WATCH_GAME_RUNNING);
    watch_flappy_new(&f,7);f.phase=WATCH_GAME_RUNNING;f.pipes[0]=(watch_pipe_t){110,235,false};watch_flappy_step(&f);assert(f.score==1);watch_flappy_step(&f);assert(f.score==1);
    f.pipes[0].x=25;watch_flappy_step(&f);assert(f.pipes[0].x>f.pipes[2].x && !f.pipes[0].scored);
    watch_flappy_new(&f,8);f.phase=WATCH_GAME_RUNNING;f.pipes[0].x=170;f.y=f.pipes[0].gap_y-70;watch_flappy_step(&f);assert(f.phase==WATCH_GAME_OVER);
    watch_flappy_new(&f,9);f.phase=WATCH_GAME_RUNNING;f.y=110;watch_flappy_step(&f);assert(f.phase==WATCH_GAME_OVER);
    watch_flappy_new(&f,10);f.phase=WATCH_GAME_RUNNING;f.y=393;watch_flappy_step(&f);assert(f.phase==WATCH_GAME_OVER);
    watch_flappy_new(&f,11);assert(f.score==0 && f.phase==WATCH_GAME_READY);
    watch_snake_vm_new(12);watch_snake_vm_start();watch_snake_vm_turn(0,-40);watch_snake_vm_step();assert(watch_snake_vm_state()->direction==WATCH_UP);
    watch_flappy_vm_new(13);watch_flappy_vm_flap();watch_flappy_vm_step();assert(watch_flappy_vm_state()->phase==WATCH_GAME_RUNNING);
    assert(!strcmp(watch_ida_vm_state()->mode,"SIM") && !strcmp(watch_ida_vm_state()->time,"14:32"));
    puts("PASS: 512 boards; all drag directions, reversal/queued turns, collisions, growth, full-board win, pause, restart, Flappy physics/scoring and static IDA.");
}
