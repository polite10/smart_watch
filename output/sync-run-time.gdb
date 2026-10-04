set pagination off
set confirm off
target remote localhost:61235
tbreak watch_ui_set_datetime
continue
call (void)memset(draw_buffer,0,256)
set $t=(RTC_TimeTypeDef*)draw_buffer
set $d=(RTC_DateTypeDef*)&draw_buffer[16]
set $t->Hours=16
set $t->Minutes=20
set $t->Seconds=13
set $d->Year=26
set $d->Month=10
set $d->Date=4
set $d->WeekDay=7
p HAL_RTC_SetTime(&rtc,$t,0)
p HAL_RTC_SetDate(&rtc,$d,0)
p smartwatch_status
p smartwatch_heartbeat
p watch_data.face_style
p watch_data.face_color
call lv_screen_load(screens[HOME])
call lv_display_refr_timer(0)
detach
quit