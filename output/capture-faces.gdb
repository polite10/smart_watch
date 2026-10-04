set pagination off
set confirm off
set remotetimeout 20
target remote localhost:61235
tbreak watch_ui_set_datetime
continue
set $saved_style=watch_data.face_style
set $saved_color=watch_data.face_color
set watch_data.face_style=0
set watch_data.face_color=0
call watch_faces_apply_saved()
call lv_screen_load(screens[HOME])
call lv_display_refr_timer(0)
dump binary memory ../output/face-pastel-framebuffer.bin 0x24000000 0x24168000
set watch_data.face_style=1
set watch_data.face_color=1
call watch_faces_apply_saved()
call lv_display_refr_timer(0)
dump binary memory ../output/face-neon-framebuffer.bin 0x24000000 0x24168000
set watch_data.face_style=2
set watch_data.face_color=2
call watch_faces_apply_saved()
call lv_display_refr_timer(0)
dump binary memory ../output/face-classic-framebuffer.bin 0x24000000 0x24168000
call watch_faces_open()
call lv_display_refr_timer(0)
dump binary memory ../output/face-picker-framebuffer.bin 0x24000000 0x24168000
set watch_data.face_style=$saved_style
set watch_data.face_color=$saved_color
call watch_faces_apply_saved()
call lv_screen_load(screens[HOME])
call lv_display_refr_timer(0)
printf "RUNNING: status=%u heartbeat=%u face=%u color=%u\n",smartwatch_status,smartwatch_heartbeat,watch_data.face_style,watch_data.face_color
detach
quit
