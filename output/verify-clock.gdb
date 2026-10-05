set pagination off
set confirm off
set remotetimeout 20
target remote localhost:61235
tbreak refresh_rtc
continue
if clock_valid
  call HAL_RTCEx_BKUPWrite(&rtc,1,0)
  monitor reset
  tbreak refresh_rtc
  continue
end
set $failures=0
define check
  if !$arg0
    printf "FAIL: %s\n", $arg1
    set $failures=$failures+1
  else
    printf "PASS: %s\n", $arg1
  end
end
check (smartwatch_status==4) "Application running"
check (sizeof(watch_data)==916) "Existing flash format retained"
check (!clock_valid&&!watch_app_vm_state()->clock_valid) "Untrusted RTC does not use build timestamp as actual time"
check (lv_screen_active()==screens[CLOCK_EDIT]) "Lost time opens manual editor"
dump binary memory ../output/settings-before-clock-test.bin &watch_data (&watch_data+1)
check (current_date==0) "Unknown time does not advance water history"
set $cmp=(int)strcmp(lv_label_get_text(face_clock[0]),"--:--")
check ($cmp==0) "Unknown digital time hidden"
check (lv_obj_has_flag(hour_hand,LV_OBJ_FLAG_HIDDEN)) "Unknown analog time hidden"
check (lv_obj_has_flag(notification,LV_OBJ_FLAG_HIDDEN)) "Unknown time suppresses notifications"
call lv_display_refr_timer(0)
dump binary memory ../output/clock-setup-framebuffer.bin 0x24000000 0x24168000
call water_add(0)
check (current_date==0) "Water input requires valid date"
check (watch_datetime_valid(2024,2,29,23,59,59)) "Leap day accepted"
check (!watch_datetime_valid(2026,2,29,12,0,0)) "Non-leap February rejected"
check (!watch_datetime_valid(2026,4,31,12,0,0)) "April 31 rejected"
check (!watch_datetime_valid(2100,1,1,0,0,0)) "RTC century range enforced"
check (!watch_datetime_valid(2026,10,4,24,0,0)) "Invalid hour rejected"
call lv_roller_set_selected(clock_year,26,LV_ANIM_OFF)
call lv_roller_set_selected(clock_month,0,LV_ANIM_OFF)
call clock_date_changed(0)
call lv_roller_set_selected(clock_day,30,LV_ANIM_OFF)
call lv_roller_set_selected(clock_month,1,LV_ANIM_OFF)
call lv_obj_send_event(clock_month,LV_EVENT_VALUE_CHANGED,0)
check (lv_roller_get_selected(clock_day)==27) "Month change clamps January 31 to February 28"
call lv_roller_set_selected(clock_year,24,LV_ANIM_OFF)
call lv_obj_send_event(clock_year,LV_EVENT_VALUE_CHANGED,0)
set $days=((lv_roller_t *)clock_day)->option_cnt
check ($days==29) "Leap-year editor offers February 29"
set $ok=watch_clock_set_datetime(2026,2,29,12,0,0)
check (!$ok&&!clock_valid) "Invalid date cannot mark RTC trusted"
set watch_data.alarms[0].hour=12
set watch_data.alarms[0].minute=34
set watch_data.alarms[0].enabled=1
set watch_data.alarms[0].daily=1
set watch_data.alarms[0].fired_day=0
set pending_alarms=1
set snooze_until[0]=1
set $ok=watch_clock_set_datetime(2026,10,4,12,34,56)
check ($ok&&clock_valid&&watch_app_vm_state()->clock_valid) "Manual clock apply updates hardware and UI"
check (!pending_alarms&&!snooze_until[0]) "Clock correction clears obsolete alarm deadlines"
check (watch_data.alarms[0].fired_day==20261004&&watch_data.alarms[0].enabled) "Matching minute does not immediately ring or disable alarm"
check (!lv_obj_has_flag(hour_hand,LV_OBJ_FLAG_HIDDEN)) "Valid time restores analog hands"
check (current_hour==12&&current_minute==34&&current_second==56) "All faces receive applied time"
check (HAL_RTCEx_BKUPRead(&rtc,1)==0x54494D32) "Trusted marker written to backup domain"
call clock_editor_open()
call lv_roller_set_selected(clock_hour,13,LV_ANIM_OFF)
call lv_roller_set_selected(clock_minute,35,LV_ANIM_OFF)
call clock_save(0)
check (current_hour==13&&current_minute==35&&current_second==0) "Save button applies minute with zero seconds"
check (lv_screen_active()==screens[HOME]) "Save returns to clock face"
call clock_editor_open()
call lv_roller_set_selected(clock_hour,18,LV_ANIM_OFF)
call lv_screen_load(screens[MENU])
check (current_hour==13) "Unsaved editor changes do not alter clock"
set watch_data.water[0].day=20261004
set watch_data.water[0].ml=600
set watch_data.water[1].day=20261003
set watch_data.water[1].ml=400
call watch_model_time_changed(20261003,844992000,12,0)
set $changed=watch_model_tick(20261003,844992000,12,0)
check (watch_data.water[0].day==20261003&&watch_data.water[0].ml==400) "Date correction reuses existing water record"
check (watch_data.water[1].day==20261004&&watch_data.water[1].ml==600) "Other recorded day retained without duplicates"
restore ../output/settings-before-clock-test.bin binary &watch_data
set $ok=watch_model_save()
check ($ok) "Original persistent records restored"
monitor reset
tbreak refresh_rtc
continue
check (clock_valid&&watch_app_vm_state()->clock_valid&&smartwatch_status==4) "Powered reset retains trusted RTC"
check (lv_screen_active()==screens[HOME]) "Powered reset opens saved clock face"
call lv_screen_load(screens[SETTINGS])
call lv_display_refr_timer(0)
dump binary memory ../output/clock-settings-framebuffer.bin 0x24000000 0x24168000
call HAL_RTCEx_BKUPWrite(&rtc,1,0)
monitor reset
tbreak refresh_rtc
continue
check (!clock_valid&&!watch_app_vm_state()->clock_valid&&current_date==0) "Simulated backup-marker loss leaves clock untrusted"
check (lv_screen_active()==screens[CLOCK_EDIT]) "Simulated time loss asks for manual date and time"
dump binary memory ../output/settings-after-clock-test.bin &watch_data (&watch_data+1)
printf "TOTAL FAILURES: %d\n", $failures
printf "STATUS: %u LSE: %u\n", smartwatch_status, smartwatch_rtc_lse
detach
quit
