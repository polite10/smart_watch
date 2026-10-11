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
    assert(f.pipes[1].x-f.pipes[0].x==160);
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
    watch_snake_vm_new(1);watch_snake_vm_choose_control(WATCH_SNAKE_CONTROL_TILT);
    for(unsigned i=0;i<15;++i) watch_snake_vm_tilt(0,0,1000);
    assert(!watch_snake_vm_calibrated() && watch_snake_vm_state()->phase==WATCH_GAME_READY);
    watch_snake_vm_tilt(0,0,1000);assert(watch_snake_vm_calibrated() && watch_snake_vm_state()->phase==WATCH_GAME_RUNNING);
    for(unsigned i=0;i<8;++i) watch_snake_vm_tilt(0,-400,916);
    watch_snake_vm_step();assert(watch_snake_vm_state()->direction==WATCH_UP);
    for(unsigned i=0;i<8;++i) watch_snake_vm_tilt(0,400,916);
    assert(!watch_snake_vm_state()->turn_queued); /* Opposite tilt is rejected. */
    watch_snake_vm_pause();watch_snake_state_t paused=*watch_snake_vm_state();
    for(unsigned i=0;i<8;++i) watch_snake_vm_tilt(-400,0,916);
    assert(!memcmp(&paused,watch_snake_vm_state(),sizeof paused));
    watch_tilt_t tilt;watch_tilt_reset(&tilt);int dx=0,dy=0;
    for(unsigned i=0;i<16;++i) assert(!watch_tilt_sample(&tilt,0,0,1000,&dx,&dy));
    for(unsigned i=0;i<20;++i) assert(!watch_tilt_sample(&tilt,30,-25,1000,&dx,&dy));
    assert(!watch_tilt_sample(&tilt,1500,1500,1500,&dx,&dy));
    watch_tilt_reset(&tilt);for(unsigned i=0;i<16;++i) watch_tilt_sample(&tilt,0,0,1000,&dx,&dy);
    bool direction=false;for(unsigned i=0;i<10;++i) direction|=watch_tilt_sample(&tilt,400,0,916,&dx,&dy);
    assert(direction && dx>0 && dy==0);
    watch_tilt_reset(&tilt);for(unsigned i=0;i<16;++i) watch_tilt_sample(&tilt,0,0,1000,&dx,&dy);
    direction=false;for(unsigned i=0;i<10;++i) direction|=watch_tilt_sample(&tilt,-400,0,916,&dx,&dy);
    assert(direction && dx<0 && dy==0);
    watch_tilt_reset(&tilt);for(unsigned i=0;i<16;++i) watch_tilt_sample(&tilt,0,0,1000,&dx,&dy);
    direction=false;for(unsigned i=0;i<10;++i) direction|=watch_tilt_sample(&tilt,0,400,916,&dx,&dy);
    assert(direction && dx==0 && dy>0);
    watch_tilt_reset(&tilt);for(unsigned i=0;i<16;++i) watch_tilt_sample(&tilt,0,0,1000,&dx,&dy);
    for(unsigned i=0;i<10;++i) assert(!watch_tilt_sample(&tilt,400,400,824,&dx,&dy));
    watch_tilt_reset(&tilt);for(unsigned i=0;i<32;++i) watch_tilt_sample(&tilt,i%2 ? 300 : -300,0,954,&dx,&dy);
    assert(!tilt.calibrated); /* Moving hands must not establish a neutral pose. */
    watch_dirty_rows_t dirty;watch_dirty_reset(&dirty);
    watch_dirty_add(&dirty,20,30,29,39);assert(dirty.pixels==100);
    watch_dirty_add(&dirty,25,30,34,39);assert(dirty.pixels==150);
    watch_dirty_add(&dirty,-30,-5,4,4);assert(dirty.pixels==175);
    watch_dirty_add(&dirty,500,500,550,550);assert(dirty.pixels==175);
    watch_dirty_reset(&dirty);watch_dirty_add(&dirty,0,0,479,479);assert(dirty.pixels==480*480);
    puts("PASS: records, failed-save retry, reload, tilt calibration/filter/reversal, dirty row clipping/union and denser Flappy obstacles.");
    puts("PASS: 512 boards; all drag directions, reversal/queued turns, collisions, growth, full-board win, pause, restart, Flappy physics/scoring and static IDA.");
}
