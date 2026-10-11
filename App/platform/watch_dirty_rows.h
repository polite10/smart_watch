#ifndef WATCH_DIRTY_ROWS_H
#define WATCH_DIRTY_ROWS_H
#include <stdint.h>
#include <stdbool.h>
/* One span per row keeps tracking bounded even when LVGL splits a frame into
 * many small flushes. Adjacent equal spans become one DMA rectangle. */
typedef struct { int16_t left[480],right[480]; uint32_t pixels; } watch_dirty_rows_t;
static inline void watch_dirty_reset(watch_dirty_rows_t *dirty)
{
    for(unsigned y=0;y<480;++y) { dirty->left[y]=480;dirty->right[y]=-1; }
    dirty->pixels=0;
}
static inline void watch_dirty_add(watch_dirty_rows_t *dirty,int x1,int y1,int x2,int y2)
{
    if(x1<0) x1=0;
    if(x2>479) x2=479;
    if(y1<0) y1=0;
    if(y2>479) y2=479;
    if(x1>x2 || y1>y2) return;
    for(int y=y1;y<=y2;++y) {
        int before=dirty->right[y]>=dirty->left[y] ? dirty->right[y]-dirty->left[y]+1 : 0;
        if(x1<dirty->left[y]) dirty->left[y]=(int16_t)x1;
        if(x2>dirty->right[y]) dirty->right[y]=(int16_t)x2;
        dirty->pixels+=(dirty->right[y]-dirty->left[y]+1)-before;
    }
}
#endif
