#include "models/watch_tilt.h"
static int absolute(int value) { return value<0 ? -value : value; }
void watch_tilt_reset(watch_tilt_t *tilt) { *tilt=(watch_tilt_t){0}; }
bool watch_tilt_sample(watch_tilt_t *t,int x,int y,int z,int *dx,int *dy)
{
    /* Reject free-fall, shocks and bad samples; expect approximately 1 g. */
    int64_t magnitude=(int64_t)x*x+(int64_t)y*y+(int64_t)z*z;
    if(magnitude<640000 || magnitude>1440000) {
        if(!t->calibrated) watch_tilt_reset(t);
        return false;
    }
    if(!t->calibrated) {
        if(t->samples && (absolute(x-t->sum_x/(int)t->samples)>80 || absolute(y-t->sum_y/(int)t->samples)>80))
            watch_tilt_reset(t);
        t->sum_x+=x;t->sum_y+=y;
        if(++t->samples>=16) {
            t->origin_x=t->sum_x/16;t->origin_y=t->sum_y/16;
            t->filtered_x=t->origin_x;t->filtered_y=t->origin_y;t->calibrated=true;
        }
        return false;
    }
    t->filtered_x+=(x-t->filtered_x)/3;t->filtered_y+=(y-t->filtered_y)/3;
    int horizontal=t->filtered_x-t->origin_x,vertical=t->filtered_y-t->origin_y;
    int ax=absolute(horizontal),ay=absolute(vertical);
    if(ax<220 && ay<220) return false;
    /* Ambiguous diagonal tilts keep the current heading. */
    if(ax>=ay+60) { *dx=horizontal;*dy=0;return true; }
    if(ay>=ax+60) { *dx=0;*dy=vertical;return true; }
    return false;
}
