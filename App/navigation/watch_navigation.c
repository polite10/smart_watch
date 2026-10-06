#include "views/watch_view_internal.h"
bool display_awake=true;
enum { TAP_WAIT_MS=120 };

static void enter_x(void *obj, int32_t value) { lv_obj_set_style_translate_x(obj,value,0); }

static void reset_motion(lv_obj_t *page)
{
    for(unsigned i=0; i<lv_obj_get_child_count(page); ++i) {
        lv_obj_t *child=lv_obj_get_child(page,i);
        lv_anim_delete(child,enter_x);
        enter_x(child,0);
    }
}

void watch_navigation_show(unsigned screen)
{
    if(screen>=SCREEN_COUNT) return;
    lv_obj_t *previous=lv_screen_active();
    if(previous==screens[screen]) return;
    if(previous==screens[MENU]) watch_menu_reset();
    if(previous==screens[FLAPPY]) arcade_leave(FLAPPY);
    if(previous==screens[SNAKE]) arcade_leave(SNAKE);
    if(previous) reset_motion(previous);
    reset_motion(screens[screen]);
    /* Load immediately so gesture arbitration sees the new page without a delay.
       Small content translations need no full-screen opacity/compositing layer. */
    lv_screen_load(screens[screen]);
    if(screen==HOME || screen==MENU || screen==FLAPPY || screen==SNAKE || screen==IDA) return; /* Lock and notification response remain immediate. */
    for(unsigned i=0; i<lv_obj_get_child_count(screens[screen]); ++i) {
        lv_obj_t *child=lv_obj_get_child(screens[screen],i);
        if(child==page_titles[screen] || lv_obj_has_flag(child,LV_OBJ_FLAG_HIDDEN)) continue;
        lv_anim_t animation; lv_anim_init(&animation);
        lv_anim_set_var(&animation,child);
        lv_anim_set_exec_cb(&animation,enter_x);
        lv_anim_set_values(&animation,10,0);
        lv_anim_set_duration(&animation,70);

        lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);
        lv_anim_start(&animation);
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

void watch_ui_touch(lv_indev_data_t *data, uint32_t now)
{
    static bool down, consumed, home_contact, moved, pending, edge_contact;
    static unsigned contact_screen;
    static lv_point_t origin, last;
    static uint32_t started, released;
    bool pressed = data->state == LV_INDEV_STATE_PRESSED;
    /* Notification controls keep their usual taps and cannot be dismissed by locking. */
    if(displayed_notification != -2) {
        down = pending = consumed = false;
        if(!display_awake) { display_awake = true; watch_ui_display_power(true); }
        return;
    }
    if(pending && pressed && !down) {
        int dx = data->point.x-last.x, dy = data->point.y-last.y;
        if(now-released <= TAP_WAIT_MS && dx*dx+dy*dy <= 48*48) {
            /* First release is still withheld: neither tap activates an app control. */
            pending = false; consumed = true; down = true;
            lv_indev_reset(lv_indev_active(), NULL);
            watch_navigation_show(HOME);
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        /* Deliver the first release before accepting a separate contact. */
        pending = false;
        data->point = last; data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    if(pressed && !down) {
        origin = last = data->point; started = now;
        down = true; moved = false;
        bool waking = !display_awake;
        home_contact = !waking && lv_screen_active() == screens[HOME];
        consumed = waking || home_contact;
        contact_screen = SCREEN_COUNT;
        for(unsigned i=0; i<SCREEN_COUNT; ++i) if(lv_screen_active() == screens[i]) contact_screen = i;
        edge_contact = !consumed && contact_screen < SCREEN_COUNT && origin.x <= 64;
        if(!display_awake) { display_awake = true; watch_ui_display_power(true); }
    }
    if(pressed) {
        last = data->point;
        int dx = last.x-origin.x, dy = last.y-origin.y;
        if(edge_contact && !consumed && dx >= 64 && dy*dy <= dx*dx/2) {
            consumed = moved = true; pending = false; edge_contact = false;
            lv_indev_reset(lv_indev_active(),NULL);
            if(contact_screen!=MENU || !watch_menu_back())
                watch_navigation_show(back_targets[contact_screen]);
        }
        if(dx*dx+dy*dy >= 24*24) {
            if(!moved && home_contact && lv_screen_active() == screens[HOME])
                watch_navigation_show(MENU);
            moved = true;
        }
        if(consumed) data->state = LV_INDEV_STATE_RELEASED;
    } else if(down) {
        down = false;
        if(consumed) {
            if(home_contact && !moved && lv_screen_active() == screens[HOME] && now-started < 500) {
                display_awake = false; watch_ui_display_power(false);
            }
            consumed = home_contact = false;
        } else if(!moved && now-started < 200 && contact_screen!=MENU && contact_screen!=FLAPPY && contact_screen!=SNAKE) {
            pending = true; released = now;
            data->point = last; data->state = LV_INDEV_STATE_PRESSED;
        }
    } else if(pending) {
        data->point = last;
        if(now-released < TAP_WAIT_MS) data->state = LV_INDEV_STATE_PRESSED;
        else pending = false;
    }
}

void watch_navigation_init(void) { display_awake=true; }
