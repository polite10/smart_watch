set pagination off
set confirm off
set remotetimeout 20
target remote localhost:61235
tbreak watch_ui_set_datetime
continue
dump binary memory ../output/settings-before-reboot.bin &watch_data (&watch_data+1)
call lv_obj_send_event(lv_obj_get_parent(note_labels[3]),LV_EVENT_CLICKED,0)
call lv_textarea_set_text(note_text,"Yeniden baslatma testi")
call lv_obj_send_event(lv_obj_get_child(screens[NOTE_EDIT],4),LV_EVENT_CLICKED,0)
set watch_data.water[0].ml = 0
call lv_obj_send_event(lv_obj_get_child(screens[WATER],6),LV_EVENT_CLICKED,0)
monitor reset
tbreak watch_ui_set_datetime
continue
set $failures = 0
if smartwatch_status != 4
  set $failures = $failures + 1
end
if smartwatch_rtc_lse != 1
  set $failures = $failures + 1
end
if watch_data.water[0].ml != 200
  set $failures = $failures + 1
end
set $cmp = (int)strcmp(watch_data.notes[3],"Yeniden baslatma testi")
if $cmp != 0
  set $failures = $failures + 1
end
printf "REBOOT: status=%u crystal=%u water=%u note_match=%d failures=%d\n", smartwatch_status, smartwatch_rtc_lse, watch_data.water[0].ml, $cmp==0, $failures
restore ../output/settings-before-reboot.bin binary &watch_data
set $ok = watch_model_save()
printf "RESTORE: %d\n", $ok
call watch_model_init()
call apps_refresh()
call notifications_refresh()
call lv_screen_load(screens[MENU])
detach
quit
