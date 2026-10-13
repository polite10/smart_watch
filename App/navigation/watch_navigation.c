#include "views/watch_view_internal.h"
bool display_awake=true;
/* Global interaction tuning. Views only register their parent screen. */
enum { DOUBLE_TAP_MS=300, DOUBLE_TAP_RADIUS=32, TAP_MAX_MS=200,
       EDGE_WIDTH=64, BACK_DISTANCE=64, HOME_DISTANCE=24, PAGE_SLIDE_MS=320 };
static bool blank_tap_pending,slide_active;
static uint32_t slide_started;
static lv_screen_load_anim_t home_slide=LV_SCR_LOAD_ANIM_MOVE_LEFT;

static void enter_x(void *obj, int32_t value) { lv_obj_set_style_translate_x(obj,value,0); }

static void reset_motion(lv_obj_t *page)
{
    for(unsigned i=0; i<lv_obj_get_child_count(page); ++i) {
        lv_obj_t *child=lv_obj_get_child(page,i);
        lv_anim_delete(child,enter_x);
        enter_x(child,0);
    }
}

static void navigation_show(unsigned screen,bool going_back)
{
    if(screen>=SCREEN_COUNT) return;
    lv_obj_t *previous=lv_screen_active();
    if(previous==screens[screen]) return;
    blank_tap_pending=false;
    if(previous==screens[MENU]) watch_menu_reset();
    if(previous==screens[PUZZLE]) (void)watch_games_vm_save_records();
    if(previous==screens[FLAPPY]) arcade_leave(FLAPPY);
    if(previous==screens[SNAKE]) arcade_leave(SNAKE);
    if(previous) reset_motion(previous);
    reset_motion(screens[screen]);
    /* All edge-back routes share the same two-screen slide, including games,
       settings and editors. Forward app entry keeps the lighter content motion. */
    if(going_back || (previous==screens[MENU] && screen==HOME) || (previous==screens[HOME] && screen==MENU)) {
        slide_active=true;slide_started=lv_tick_get();
        lv_screen_load_anim_t direction=going_back || screen==HOME ? LV_SCR_LOAD_ANIM_MOVE_RIGHT : home_slide;
        lv_screen_load_anim(screens[screen],direction,PAGE_SLIDE_MS,0,false);
        return;
    }
    slide_active=false;
    lv_screen_load(screens[screen]);
    lv_obj_update_layout(screens[screen]);
    if(screen==HOME || screen==MENU) return;
    for(unsigned i=0; i<lv_obj_get_child_count(screens[screen]); ++i) {
        lv_obj_t *child=lv_obj_get_child(screens[screen],i);
        if(child==page_titles[screen] || lv_obj_has_flag(child,LV_OBJ_FLAG_HIDDEN)) continue;
        /* Full-screen artwork stays put while the app's content arrives. */
        if(lv_obj_get_width(child)>=480 && lv_obj_get_height(child)>=480) continue;
        lv_anim_t animation; lv_anim_init(&animation);
        lv_anim_set_var(&animation,child);
        lv_anim_set_exec_cb(&animation,enter_x);
        lv_anim_set_values(&animation,64,0);
        lv_anim_set_duration(&animation,440);

        lv_anim_set_path_cb(&animation,lv_anim_path_ease_in_out);
        lv_anim_start(&animation);
    }
}

void watch_navigation_show(unsigned screen) { navigation_show(screen,false); }

void watch_navigation_back(void)
{
    lv_obj_t *active=lv_screen_active();
    for(unsigned screen=0;screen<SCREEN_COUNT;++screen) {
        if(active!=screens[screen])continue;
        if(screen==HOME || (screen==MENU && watch_menu_back()))return;
        navigation_show(back_targets[screen],true);
        return;
    }
}

void navigate(lv_event_t *e)
{
    unsigned screen = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(screen == FACES) watch_faces_open();
    else if(screen == CLOCK_EDIT) clock_editor_open();
    else if(screen == PUZZLE) puzzle_open();
    else if(screen == FLAPPY) flappy_open();
    else if(screen == SNAKE) snake_open();
    else watch_navigation_show(screen);
}

/* One gesture policy for the whole application: only empty launcher space
   may return home on a double tap. App controls are never delayed. */
void watch_ui_touch(lv_indev_data_t *data, uint32_t now)
{
    static bool down,consumed,home_contact,moved,edge_contact,ring_contact,blank_contact,second_tap;
    static unsigned contact_screen;
    static lv_point_t origin,last_tap;
    static uint32_t started,released;
    bool pressed=data->state==LV_INDEV_STATE_PRESSED;
    if(displayed_notification!=-2) {
        down=blank_tap_pending=consumed=ring_contact=false;
        if(!display_awake) { display_awake=true;watch_ui_display_power(true); }
        return;
    }
    if(slide_active) {
        if(now-slide_started<PAGE_SLIDE_MS+20) {
            /* Consume contacts begun during a transition until they release. */
            if(pressed) { down=true;consumed=true;home_contact=false;moved=true; }
            else down=false;
            data->state=LV_INDEV_STATE_RELEASED;return;
        }
        slide_active=false;
        /* Input can run before the next refresh. Resolve the final screen
           coordinates before hit-testing the first post-transition contact. */
        lv_obj_update_layout(lv_screen_active());
    }
    if(pressed && !down) {
        origin=data->point;started=now;down=true;moved=false;second_tap=false;
        bool waking=!display_awake;
        home_contact=!waking && lv_screen_active()==screens[HOME];
        consumed=waking || home_contact;
        contact_screen=SCREEN_COUNT;
        for(unsigned i=0;i<SCREEN_COUNT;++i) if(lv_screen_active()==screens[i])contact_screen=i;
        edge_contact=!consumed && contact_screen<SCREEN_COUNT && origin.x<=EDGE_WIDTH;
        blank_contact=!consumed && contact_screen==MENU && watch_menu_blank(origin);
        ring_contact=!consumed && contact_screen==MENU && watch_menu_ring_begin(origin,now);
        int dx=origin.x-last_tap.x,dy=origin.y-last_tap.y;
        second_tap=blank_contact && blank_tap_pending && now-released<=DOUBLE_TAP_MS && dx*dx+dy*dy<=DOUBLE_TAP_RADIUS*DOUBLE_TAP_RADIUS;
        blank_tap_pending=false;
        if(waking) { display_awake=true;watch_ui_display_power(true); }
    }
    if(pressed) {
        int dx=data->point.x-origin.x,dy=data->point.y-origin.y;
        if(edge_contact && !consumed && dx>=BACK_DISTANCE && dy*dy<=dx*dx/2) {
            consumed=moved=true;edge_contact=ring_contact=false;second_tap=false;
            lv_indev_reset(lv_indev_active(),NULL);
            watch_navigation_back();
        }
        if(ring_contact && watch_menu_ring_drag(data->point,now)) {
            if(!consumed)lv_indev_reset(lv_indev_active(),NULL);
            consumed=moved=true;edge_contact=false;second_tap=false;
        }
        if(dx*dx+dy*dy>=HOME_DISTANCE*HOME_DISTANCE) {
            if(!moved && home_contact && lv_screen_active()==screens[HOME]) {
                if(dx*dx>=dy*dy)home_slide=dx>0 ? LV_SCR_LOAD_ANIM_MOVE_RIGHT : LV_SCR_LOAD_ANIM_MOVE_LEFT;
                else home_slide=dy>0 ? LV_SCR_LOAD_ANIM_MOVE_BOTTOM : LV_SCR_LOAD_ANIM_MOVE_TOP;
                watch_navigation_show(MENU);
            }
            moved=true;second_tap=false;
        }
        if(consumed)data->state=LV_INDEV_STATE_RELEASED;
    } else if(down) {
        down=false;
        if(ring_contact && watch_menu_ring_end(now)) {
            consumed=true;lv_indev_reset(lv_indev_active(),NULL);
        }
        ring_contact=false;
        if(consumed) {
            if(home_contact && !moved && lv_screen_active()==screens[HOME] && now-started<500) {
                display_awake=false;watch_ui_display_power(false);
            }
            consumed=home_contact=false;
        } else if(!moved && now-started<TAP_MAX_MS && blank_contact && lv_screen_active()==screens[MENU] && watch_menu_blank(data->point)) {
            if(second_tap) {
                lv_indev_reset(lv_indev_active(),NULL);watch_navigation_show(HOME);
            } else { blank_tap_pending=true;released=now;last_tap=origin; }
        }
    }
}

void watch_navigation_init(void)
{ display_awake=true;blank_tap_pending=slide_active=false;home_slide=LV_SCR_LOAD_ANIM_MOVE_LEFT; }
