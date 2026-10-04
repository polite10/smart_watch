set pagination off
set confirm off
set remotetimeout 20
target remote localhost:61235
tbreak watch_ui_set_datetime
continue
set $failures=0
define check
  if !$arg0
    printf "FAIL: %s\n", $arg1
    set $failures=$failures+1
  else
    printf "PASS: %s\n", $arg1
  end
end
check (smartwatch_status==4) "UI running"
check (sizeof(watch_data)==916) "Version-1 storage size retained"
dump binary memory ../output/settings-before-faces-test.bin &watch_data (&watch_data+1)
set $original_style=watch_data.face_style
set $original_color=watch_data.face_color
call watch_faces_open()
call lv_obj_send_event(style_buttons[2],LV_EVENT_CLICKED,0)
call lv_obj_send_event(color_buttons[1],LV_EVENT_CLICKED,0)
check (watch_data.face_style==$original_style&&watch_data.face_color==$original_color) "Preview does not apply or save"
call watch_faces_open()
check (draft_style==$original_style&&draft_color==$original_color) "Reopening cancels uncommitted changes"
set $i=0
while $i<3
  call lv_obj_send_event(style_buttons[$i],LV_EVENT_CLICKED,0)
  call lv_obj_send_event(color_buttons[$i],LV_EVENT_CLICKED,0)
  call lv_obj_send_event(apply_button,LV_EVENT_CLICKED,0)
  check (watch_data.face_style==$i&&watch_data.face_color==$i) "Face and color applied"
  set $j=0
  while $j<3
    set $hidden=lv_obj_has_flag(face_roots[$j],LV_OBJ_FLAG_HIDDEN)
    check (($j==$i&&!$hidden)||($j!=$i&&$hidden)) "Only selected face visible"
    set $j=$j+1
  end
  call lv_display_refr_timer(0)
  if $i==0
    dump binary memory ../output/face-pastel-framebuffer.bin 0x24000000 0x24168000
  end
  if $i==1
    dump binary memory ../output/face-neon-framebuffer.bin 0x24000000 0x24168000
  end
  if $i==2
    dump binary memory ../output/face-classic-framebuffer.bin 0x24000000 0x24168000
  end
  set $i=$i+1
  call watch_faces_open()
end
call lv_display_refr_timer(0)
dump binary memory ../output/face-picker-framebuffer.bin 0x24000000 0x24168000
call watch_faces_set_datetime(2026,10,4,0,5,30,1)
set $cmp=(int)strcmp(lv_label_get_text(face_clock[0]),"12:05")
check ($cmp==0) "12-hour midnight display"
call watch_faces_set_datetime(2026,10,4,13,5,30,1)
set $cmp=(int)strcmp(lv_label_get_text(period_labels[1]),"PM")
check ($cmp==0) "Afternoon marker"
set $cmp=(int)strcmp(lv_label_get_text(seconds_label),"30")
check ($cmp==0) "Live seconds display"
call watch_faces_set_datetime(2026,10,4,3,0,0,0)
check (hand_points[0][1].x==325&&hand_points[0][1].y==225) "Analog hour hand at 3 oclock"
check (hand_points[1][1].x==240&&hand_points[1][1].y==100) "Analog minute hand at 12"
call lv_obj_send_event(face_roots[2],LV_EVENT_LONG_PRESSED,0)
check (draft_style==2&&draft_color==2) "Long press opens current selection"
monitor reset
tbreak watch_ui_set_datetime
continue
check (smartwatch_status==4&&watch_data.face_style==2&&watch_data.face_color==2) "Face and color survive hardware reset"
set $hidden=lv_obj_has_flag(face_roots[2],LV_OBJ_FLAG_HIDDEN)
check (!$hidden) "Saved face visible on startup"
set watch_data.face_style=$original_style
set watch_data.face_color=$original_color
set $ok=watch_model_save()
check ($ok) "Original face preference restored"
call watch_faces_apply_saved()
dump binary memory ../output/settings-after-faces-test.bin &watch_data (&watch_data+1)
call lv_screen_load(screens[HOME])
call lv_display_refr_timer(0)
printf "TOTAL FAILURES: %d\n", $failures
printf "STATUS: %u HEARTBEAT: %u\n", smartwatch_status, smartwatch_heartbeat
detach
quit
