#include "views/watch_view_internal.h"
static lv_obj_t *storage_notice,*notification,*notification_title,*notification_text,*notification_first,*notification_second;
int displayed_notification=-2;

void storage_refresh(void)
{
    if(!storage_notice) return;
    if(watch_app_vm_state()->storage_ok) lv_obj_add_flag(storage_notice,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(storage_notice,LV_OBJ_FLAG_HIDDEN);
}
void save_data(void) { watch_app_vm_save(); storage_refresh(); }
static void notification_action(lv_event_t *e)
{ watch_notification_vm_action((uintptr_t)lv_event_get_user_data(e)==0); storage_refresh(); apps_refresh(); notifications_refresh(); }
void notifications_refresh(void)
{
    watch_notification_state_t state=watch_notification_vm_state();
    if(state.selected==-2) { lv_obj_add_flag(notification,LV_OBJ_FLAG_HIDDEN); displayed_notification=-2; return; }
    if(state.selected!=displayed_notification) {
        if(!display_awake) { display_awake=true; watch_ui_display_power(true); }
        set_label(notification_title,state.title); set_label(notification_text,state.text);
        set_label(notification_first,state.first); set_label(notification_second,state.second);
        displayed_notification=state.selected;
    }
    lv_obj_remove_flag(notification,LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(notification);
}

void notifications_init(void)
{
    storage_notice = label_at(lv_layer_top(), "Kayit basarisiz. Tekrar deneyin.", 405, &lv_font_montserrat_14);
    lv_obj_set_width(storage_notice,260);
    lv_obj_set_style_text_align(storage_notice,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(storage_notice, lv_color_hex(0xFF596F), 0);
    lv_obj_add_flag(storage_notice, LV_OBJ_FLAG_HIDDEN);
    notification = lv_obj_create(lv_layer_top());
    lv_obj_set_size(notification, 480, 480); lv_obj_set_pos(notification, 0, 0);
    lv_obj_set_style_bg_color(notification, lv_color_hex(0x101B2D), 0);
    lv_obj_set_style_text_color(notification, lv_color_hex(0xF0F4FC), 0);
    lv_obj_set_style_border_width(notification, 0, 0);
    lv_obj_set_style_pad_all(notification, 0, 0);
    lv_obj_remove_flag(notification, LV_OBJ_FLAG_SCROLLABLE);
    notification_title = label_at(notification, "", 102, &lv_font_montserrat_24);
    notification_text = label_at(notification, "", 167, &lv_font_montserrat_16);
    lv_obj_set_style_text_align(notification_text, LV_TEXT_ALIGN_CENTER, 0);
    notification_first = button_at(notification, "", 0, 250, 264, notification_action, (void *)(uintptr_t)0);
    notification_second = button_at(notification, "", 0, 322, 264, notification_action, (void *)(uintptr_t)1);
    lv_obj_add_flag(notification, LV_OBJ_FLAG_HIDDEN);

}
