set pagination off
set confirm off
target remote localhost:61235
tbreak watch_ui_set_datetime
continue
call lv_screen_load(screens[MENU])
call lv_display_refr_timer(0)
dump binary memory ../output/menu-framebuffer.bin 0x24000000 0x24168000
call lv_screen_load(screens[WATER])
call lv_display_refr_timer(0)
dump binary memory ../output/water-framebuffer.bin 0x24000000 0x24168000
call lv_screen_load(screens[CALCULATOR])
call lv_display_refr_timer(0)
dump binary memory ../output/calculator-framebuffer.bin 0x24000000 0x24168000
call lv_obj_send_event(lv_obj_get_parent(note_labels[0]),LV_EVENT_CLICKED,0)
call lv_display_refr_timer(0)
dump binary memory ../output/notes-framebuffer.bin 0x24000000 0x24168000
call lv_obj_send_event(lv_obj_get_parent(alarm_labels[0]),LV_EVENT_CLICKED,0)
call lv_display_refr_timer(0)
dump binary memory ../output/alarm-framebuffer.bin 0x24000000 0x24168000
call watch_faces_open()
call lv_display_refr_timer(0)
dump binary memory ../output/face-picker-framebuffer.bin 0x24000000 0x24168000
call lv_screen_load(screens[MENU])
call lv_display_refr_timer(0)
printf "STATUS: %u HEARTBEAT: %u\n", smartwatch_status, smartwatch_heartbeat
detach
quit
