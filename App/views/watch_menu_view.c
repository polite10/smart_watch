#include "views/watch_view_internal.h"
#include "watch_icons.h"
#include "views/watch_art.h"
#include <math.h>

static lv_obj_t *menu_buttons[9],*menu_folder,*menu_games[3],*menu_game_images[3];
static bool folder_open,folder_moving;
static unsigned folder_progress;
static const int small_x[]={240,277,203},small_y[]={197,261,261};
static const int large_x[]={240,305,175},large_y[]={165,278,278};
static void folder_layout(void *unused,int32_t value)
{
    (void)unused; folder_progress=(unsigned)value;
    int size=190+value*110/256;
    lv_obj_set_size(menu_folder,size,size);lv_obj_set_pos(menu_folder,240-size/2,240-size/2);
    for(unsigned i=0;i<3;++i) {
        int diameter=64+value*24/256;
        int x=small_x[i]+(large_x[i]-small_x[i])*value/256;
        int y=small_y[i]+(large_y[i]-small_y[i])*value/256;
        lv_obj_set_size(menu_games[i],diameter,diameter);lv_obj_set_pos(menu_games[i],x-diameter/2,y-diameter/2);
        lv_image_set_scale(menu_game_images[i],256+value*96/256);lv_obj_center(menu_game_images[i]);
    }
}
static void folder_finished(lv_anim_t *animation)
{
    (void)animation; folder_moving=false;
    for(unsigned i=0;i<3;++i) lv_obj_add_flag(menu_games[i],LV_OBJ_FLAG_CLICKABLE);
}
static void folder_show(bool open)
{
    if(folder_moving || folder_open==open) return;
    folder_open=open;folder_moving=true;
    for(unsigned i=0;i<9;++i) {
        lv_obj_set_style_bg_opa(menu_buttons[i],open ? 80 : LV_OPA_COVER,0);
        lv_obj_t *image=lv_obj_get_child(menu_buttons[i],1);
        lv_obj_set_style_image_opa(image,open ? 80 : LV_OPA_COVER,0);
        if(open) lv_obj_remove_flag(menu_buttons[i],LV_OBJ_FLAG_CLICKABLE);
        else lv_obj_add_flag(menu_buttons[i],LV_OBJ_FLAG_CLICKABLE);
    }
    for(unsigned i=0;i<3;++i) lv_obj_remove_flag(menu_games[i],LV_OBJ_FLAG_CLICKABLE);
    lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,menu_folder);
    lv_anim_set_exec_cb(&a,folder_layout);lv_anim_set_values(&a,(int32_t)folder_progress,open ? 256 : 0);
    lv_anim_set_duration(&a,180);lv_anim_set_path_cb(&a,lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&a,folder_finished);lv_anim_start(&a);
}
static void folder_clicked(lv_event_t *e) { (void)e;folder_show(!folder_open); }
static void background_clicked(lv_event_t *e) { (void)e;if(folder_open) folder_show(false); }
static void game_clicked(lv_event_t *e)
{
    if(folder_moving) return;
    if(!folder_open) { folder_show(true);return; }
    navigate(e);
}
void watch_menu_reset(void)
{
    lv_anim_delete(menu_folder,folder_layout);folder_open=folder_moving=false;folder_layout(NULL,0);
    for(unsigned i=0;i<9;++i) {
        lv_obj_add_flag(menu_buttons[i],LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(menu_buttons[i],LV_OPA_COVER,0);
        lv_obj_set_style_image_opa(lv_obj_get_child(menu_buttons[i],1),LV_OPA_COVER,0);
    }
    for(unsigned i=0;i<3;++i) lv_obj_add_flag(menu_games[i],LV_OBJ_FLAG_CLICKABLE);
}
bool watch_menu_back(void)
{
    if(!folder_open && !folder_moving) return false;
    watch_menu_reset();return true;
}
void watch_menu_init(void)
{
    page_header(MENU,"",HOME);
    lv_obj_t *wallpaper=lv_image_create(screens[MENU]);lv_image_set_src(wallpaper,&menu_wallpaper);
    lv_obj_remove_flag(wallpaper,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_pos(wallpaper,0,0);
    const unsigned targets[]={ALARMS,CALCULATOR,NOTES,FACES,CALENDAR,SETTINGS,HOME,WATER,IDA};
    const lv_image_dsc_t *icons[]={&icon_alarm,&icon_calculator,&icon_notes,&icon_faces,&icon_calendar,&icon_settings,&icon_clock,&icon_water,&icon_ida};
    const uint32_t colors[]={0xFF9B9E,0x96E1C2,0xE5C191,0xD7AFF7,0xFFC5A2,0xADC4D6,0x9CAFFF,0x82D9F4,0x96E1C2};
    for(unsigned i=0;i<9;++i) {
        float angle=(-90.0f+40.0f*i)*0.01745329252f;
        int x=(int)lroundf(240+178*cosf(angle)),y=(int)lroundf(240+178*sinf(angle));
        lv_obj_t *label=round_action(screens[MENU],"",x-240,y-40,80,colors[i],navigate,(void *)(uintptr_t)targets[i]);
        menu_buttons[i]=lv_obj_get_parent(label);
        lv_obj_t *image=lv_image_create(menu_buttons[i]);lv_image_set_src(image,icons[i]);lv_obj_center(image);
        lv_obj_remove_flag(image,LV_OBJ_FLAG_CLICKABLE);
    }
    menu_folder=lv_button_create(screens[MENU]);lv_obj_remove_flag(menu_folder,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(menu_folder,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(menu_folder,lv_color_hex(0xBCD4EC),0);
    lv_obj_set_style_bg_opa(menu_folder,32,0);lv_obj_set_style_border_width(menu_folder,1,0);
    lv_obj_set_style_border_color(menu_folder,lv_color_hex(0x849DB5),0);lv_obj_set_style_shadow_width(menu_folder,0,0);
    lv_obj_add_event_cb(menu_folder,folder_clicked,LV_EVENT_CLICKED,NULL);
    const unsigned games[]={PUZZLE,FLAPPY,SNAKE};
    const lv_image_dsc_t *game_icons[]={&icon_puzzle,&icon_flappy,&icon_snake};
    const uint32_t game_colors[]={0xFFD18B,0xFFD18B,0xB7DC8B};
    for(unsigned i=0;i<3;++i) {
        lv_obj_t *label=round_action(screens[MENU],"",0,0,64,game_colors[i],game_clicked,(void *)(uintptr_t)games[i]);
        menu_games[i]=lv_obj_get_parent(label);lv_obj_set_align(menu_games[i],LV_ALIGN_TOP_LEFT);
        menu_game_images[i]=lv_image_create(menu_games[i]);lv_image_set_src(menu_game_images[i],game_icons[i]);
        lv_obj_remove_flag(menu_game_images[i],LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_add_event_cb(screens[MENU],background_clicked,LV_EVENT_CLICKED,NULL);
    watch_menu_reset();
}
