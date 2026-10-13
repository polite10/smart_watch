#include "models/watch_arcade.h"
#include "viewmodels/watch_arcade_vm.h"
#include "viewmodels/watch_ida_vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "services/watch_records_service.h"
#include "viewmodels/watch_games_vm.h"
#include "platform/watch_dirty_rows.h"
static watch_records_t stored_records;
static bool port_ok=true;
static unsigned save_count;
static const watch_data_t user_data={.version=1,.water_goal=2000};
const watch_data_t *watch_model_data(void) { return &user_data; }
bool watch_storage_records_load(watch_records_t *records)
{ *records=stored_records;return stored_records.version==1; }
bool watch_storage_records_save(const watch_data_t *data,const watch_records_t *records)
{
    assert(data==&user_data);++save_count;
    if(!port_ok) return false;
    stored_records=*records;return true;
}
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
    watch_records_service_init();
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
    /* Every edge wraps; a long straight run remains alive across repeated seams. */
    for(unsigned direction=0;direction<4;++direction) {
        watch_snake_new(&s,3);s.phase=WATCH_GAME_RUNNING;s.direction=direction;
        s.bug_count=0; /* Isolate legacy edge-wrap rules from insect collisions. */
        for(unsigned i=0;i<4;++i) s.body[i]=(watch_cell_t){
            direction==WATCH_RIGHT ? 17-i : direction==WATCH_LEFT ? i : 9,
            direction==WATCH_DOWN ? 17-i : direction==WATCH_UP ? i : 9};
        s.food=(watch_cell_t){1,1};watch_snake_step(&s);
        assert(s.phase==WATCH_GAME_RUNNING);
        assert(s.body[0].x==(direction==WATCH_RIGHT ? 0 : direction==WATCH_LEFT ? 17 : 9));
        assert(s.body[0].y==(direction==WATCH_DOWN ? 0 : direction==WATCH_UP ? 17 : 9));
        for(unsigned i=0;i<WATCH_SNAKE_SIDE*3;++i) { watch_snake_step(&s);assert(s.phase==WATCH_GAME_RUNNING);same_cells(&s); }
    }
    watch_snake_new(&s,3);s.phase=WATCH_GAME_RUNNING;
    s.body[0]=(watch_cell_t){17,9};s.food=(watch_cell_t){0,9};watch_snake_step(&s);
    assert(s.phase==WATCH_GAME_RUNNING && s.length==5 && s.score==1 && s.body[0].x==0);
    watch_snake_new(&s,3);s.phase=WATCH_GAME_RUNNING;
    s.body[0]=(watch_cell_t){17,9};s.body[1]=(watch_cell_t){0,9};s.food=(watch_cell_t){1,1};watch_snake_step(&s);
    assert(s.phase==WATCH_GAME_OVER); /* Wrapping into the body is still fatal. */
    watch_snake_new(&s,4);s.phase=WATCH_GAME_RUNNING;s.direction=WATCH_UP;
    s.bug_count=0;
    s.body[0]=(watch_cell_t){2,2};s.body[1]=(watch_cell_t){3,2};s.body[2]=(watch_cell_t){3,1};s.body[3]=(watch_cell_t){2,1};s.food=(watch_cell_t){10,10};
    watch_snake_step(&s);assert(s.phase==WATCH_GAME_RUNNING && s.body[0].y==1); /* old tail is legal */
    s.body[0]=(watch_cell_t){2,2};s.body[1]=(watch_cell_t){2,1};s.body[2]=(watch_cell_t){3,1};s.body[3]=(watch_cell_t){3,2};
    watch_snake_step(&s);assert(s.phase==WATCH_GAME_OVER);
    /* Hamiltonian body fills all but the next head cell; eating it wins safely. */
    watch_snake_new(&s,5);s.phase=WATCH_GAME_RUNNING;s.length=323;s.direction=WATCH_LEFT;
    s.bug_count=0;
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
    /* A collectible flower awards once; passing the gap still remains safe. */
    for(unsigned i=1;i<3;++i) {
        watch_flappy_new(&f,20+i);f.phase=WATCH_GAME_RUNNING;
        f.pipes[i]=(watch_pipe_t){.x=62.2f,.gap_y=240,.scored=true,.flower=true};
        float fx,fy;assert(watch_flappy_flower(i,&f.pipes[i],&fx,&fy));
        f.y=fy;f.velocity=-0.25f;
        watch_flappy_step(&f);assert(f.phase==WATCH_GAME_RUNNING && f.score==1 && f.pipes[i].collected);
        watch_flappy_step(&f);assert(f.score==1);
        watch_flappy_new(&f,20+i);f.phase=WATCH_GAME_RUNNING;
        f.pipes[i]=(watch_pipe_t){.x=62.2f,.gap_y=240,.scored=true,.flower=true};f.y=240;f.velocity=-0.25f;
        watch_flappy_step(&f);assert(f.phase==WATCH_GAME_RUNNING);
    }
    watch_snake_vm_new(12);watch_snake_vm_start();watch_snake_vm_turn(0,-40);watch_snake_vm_step();assert(watch_snake_vm_state()->direction==WATCH_UP);
    watch_flappy_vm_new(13);watch_flappy_vm_flap();watch_flappy_vm_step();assert(watch_flappy_vm_state()->phase==WATCH_GAME_RUNNING);
    watch_ida_vm_init();
    assert(!strcmp(watch_ida_vm_state()->mode,"SIM") && !strcmp(watch_ida_vm_state()->time,"--:--"));
    assert(f.pipes[1].x-f.pipes[0].x==WATCH_FLAPPY_SPACING);
    watch_records_service_init();
    watch_records_service_update(WATCH_RECORD_SNAKE,9);
    watch_records_service_update(WATCH_RECORD_SNAKE,4);
    watch_records_service_update(WATCH_RECORD_FLAPPY,12);
    watch_records_service_update(WATCH_RECORD_PUZZLE,80);
    watch_records_service_update(WATCH_RECORD_PUZZLE,60);
    watch_records_service_update(WATCH_RECORD_PUZZLE,100);
    assert(watch_games_vm_records()->snake_score==9 && watch_games_vm_records()->flappy_score==12 && watch_games_vm_records()->puzzle_moves==60);
    port_ok=false;assert(!watch_games_vm_save_records());
    port_ok=true;assert(watch_games_vm_save_records());
    unsigned count=save_count;assert(watch_games_vm_save_records() && count==save_count);
    watch_records_service_init();assert(watch_games_vm_records()->snake_score==9 && watch_games_vm_records()->puzzle_moves==60);
    watch_dirty_rows_t dirty;watch_dirty_reset(&dirty);
    watch_dirty_add(&dirty,20,30,29,39);assert(dirty.pixels==100);
    watch_dirty_add(&dirty,25,30,34,39);assert(dirty.pixels==150);
    watch_dirty_add(&dirty,-30,-5,4,4);assert(dirty.pixels==175);
    watch_dirty_add(&dirty,500,500,550,550);assert(dirty.pixels==175);
    watch_dirty_reset(&dirty);watch_dirty_add(&dirty,0,0,479,479);assert(dirty.pixels==480*480);
    puts("PASS: records, failed-save retry, reload, dirty row clipping/union and denser Flappy obstacles.");
    puts("PASS: 512 boards; all drag directions, reversal/queued turns, collisions, growth, full-board win, pause, restart, Flappy physics/scoring and IDA initialization.");
}
