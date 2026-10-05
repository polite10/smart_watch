#include "views/watch_view_internal.h"
bool display_awake=true;

void navigate(lv_event_t *e)
{
    unsigned screen = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(screen == FACES) watch_faces_open();
    else if(screen == CLOCK_EDIT) clock_editor_open();
    else lv_screen_load(screens[screen]);
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
        if(now-released <= 260 && dx*dx+dy*dy <= 48*48) {
            /* First release is still withheld: neither tap activates an app control. */
            pending = false; consumed = true; down = true;
            lv_indev_reset(lv_indev_active(), NULL);
            lv_screen_load(screens[HOME]);
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
            lv_screen_load(screens[back_targets[contact_screen]]);
        }
        if(dx*dx+dy*dy >= 24*24) {
            if(!moved && home_contact && lv_screen_active() == screens[HOME])
                lv_screen_load(screens[MENU]);
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
        } else if(!moved && now-started < 200) {
            pending = true; released = now;
            data->point = last; data->state = LV_INDEV_STATE_PRESSED;
        }
    } else if(pending) {
        data->point = last;
        if(now-released < 260) data->state = LV_INDEV_STATE_PRESSED;
        else pending = false;
    }
}

void watch_navigation_init(void) { display_awake=true; }
