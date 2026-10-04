set pagination off
set confirm off
set remotetimeout 20
target remote localhost:61235
tbreak watch_ui_set_datetime
continue
set $failures = 0
define check
  if !$arg0
    printf "FAIL: %s\n", $arg1
    set $failures = $failures + 1
  else
    printf "PASS: %s\n", $arg1
  end
end
check (smartwatch_status==4) "UI running"
dump binary memory ../output/settings-before-tests.bin &watch_data (&watch_data+1)
set $epoch = watch_epoch(2026,10,4,10,0,0)
set $changed = watch_model_tick(20261004,$epoch,10,0)
set watch_data.water[0].ml = 0
call watch_water_add(1)
check (watch_data.water[0].ml==200) "One glass is 200 ml"
call watch_water_add(-1)
call watch_water_add(-1)
check (watch_data.water[0].ml==0) "Undo never below zero"
call watch_water_add(500)
check (watch_data.water[0].ml==20000) "Consumption upper bound"
set watch_data.water[0].ml = 400
set $changed = watch_model_tick(20261005,$epoch+86400,10,0)
check (watch_data.water[0].ml==0&&watch_data.water[1].ml==400&&watch_data.water[1].day==20261004) "Midnight archive and reset"
set watch_data.water_goal = 2000
call watch_water_goal(-10000)
check (watch_data.water_goal==200) "Goal minimum"
call watch_water_goal(10000)
check (watch_data.water_goal==6000) "Goal maximum"
set watch_data.water_goal = 2000
set $changed = watch_model_tick(20261004,$epoch,10,0)
set watch_data.water[0].ml = 0
call watch_reminder_set(30)
set $changed = watch_model_tick(20261004,$epoch+1799,10,29)
set $due = watch_water_reminder_due()
check (!$due) "No early water reminder"
set $changed = watch_model_tick(20261004,$epoch+1800,10,30)
set $due = watch_water_reminder_due()
check ($due) "Water reminder at 30 minutes"
call watch_water_add(1)
set $due = watch_water_reminder_due()
check (!$due) "Drinking clears reminder"
set watch_data.water[0].ml = 2000
set $changed = watch_model_tick(20261004,$epoch+3600,11,0)
set $due = watch_water_reminder_due()
check (!$due) "Goal completion suppresses reminders"
set watch_data.water[0].ml = 0
call watch_reminder_set(30)
set $changed = watch_model_tick(20261004,$epoch+13*3600,23,0)
set $due = watch_water_reminder_due()
check (!$due) "Quiet hours suppress water reminders"
call watch_reminder_set(0)
set $changed = watch_model_tick(20261004,$epoch,10,0)
set watch_data.alarms[0].hour = 10
set watch_data.alarms[0].minute = 1
set watch_data.alarms[0].daily = 1
set watch_data.alarms[0].enabled = 1
call watch_alarm_changed(0)
set $changed = watch_model_tick(20261004,$epoch+59,10,0)
set $active = watch_alarm_active()
check (!$active) "Alarm waits for set time"
set $changed = watch_model_tick(20261004,$epoch+60,10,1)
set $active = watch_alarm_active()
check ($active) "Alarm fires at set time"
call watch_alarm_ack(0,0)
set $changed = watch_model_tick(20261004,$epoch+61,10,1)
set $active = watch_alarm_active()
check (!$active) "Alarm does not repeat in same minute"
set $changed = watch_model_tick(20261005,$epoch+86460,10,1)
set $active = watch_alarm_active()
check ($active) "Daily alarm repeats next day"
call watch_alarm_ack(0,1)
set $changed = watch_model_tick(20261005,$epoch+86759,10,5)
set $active = watch_alarm_active()
check (!$active) "Snooze waits five minutes"
set $changed = watch_model_tick(20261005,$epoch+86760,10,6)
set $active = watch_alarm_active()
check ($active) "Snooze fires after five minutes"
call watch_alarm_ack(0,0)
set watch_data.alarms[1].hour = 10
set watch_data.alarms[1].minute = 7
set watch_data.alarms[1].daily = 0
set watch_data.alarms[1].enabled = 1
call watch_alarm_changed(1)
set $changed = watch_model_tick(20261005,$epoch+86820,10,7)
set $active = watch_alarm_active()
check ($active&&watch_data.alarms[1].enabled==0) "One-shot alarm disables after firing"
call watch_alarm_ack(1,0)
set $a = watch_epoch(2024,2,29,0,0,0)
set $b = watch_epoch(2024,3,1,0,0,0)
check ($b-$a==86400) "Leap-day timestamp"
set $ok = watch_calculate("2+3*4",(double*)draw_buffer)
check ($ok&&*(double*)draw_buffer==14) "Calculator precedence"
set $ok = watch_calculate("-2.5*4+20/2",(double*)draw_buffer)
check ($ok&&*(double*)draw_buffer==0) "Calculator decimals and negatives"
set $ok = watch_calculate("1/0",(double*)draw_buffer)
check (!$ok) "Division by zero rejected"
set $ok = watch_calculate("2+",(double*)draw_buffer)
check (!$ok) "Incomplete expression rejected"
set $ok = watch_calculate("NaN",(double*)draw_buffer)
check (!$ok) "Non-number rejected"
set $ok = watch_calculate("1000000000000*100",(double*)draw_buffer)
check (!$ok) "Overflow rejected"
set $i = 0
while $i < 18
  set watch_data.water[0].ml = $i*200
  set $ok = watch_model_save()
  if !$ok
    set $failures = $failures + 1
    printf "FAIL: journal write %d\n", $i
  end
  set $i = $i + 1
end
call watch_model_init()
check (watch_data.water[0].ml==3400&&latest_sequence>=18) "Journal rotation and reload"
call lv_obj_send_event(lv_obj_get_parent(note_labels[0]),LV_EVENT_CLICKED,0)
call lv_textarea_set_text(note_text,"Kalici not testi")
call lv_obj_send_event(lv_obj_get_child(screens[NOTE_EDIT],4),LV_EVENT_CLICKED,0)
call watch_model_init()
set $cmp = (int)strcmp(watch_data.notes[0],"Kalici not testi")
check ($cmp==0) "Note survives model reload"
restore ../output/settings-before-tests.bin binary &watch_data
set $ok = watch_model_save()
check ($ok) "Original settings restored"
call watch_model_init()
call apps_refresh()
call notifications_refresh()
call lv_screen_load(screens[MENU])
call lv_display_refr_timer(0)
dump binary memory ../output/menu-framebuffer.bin 0x24000000 0x24168000
printf "TOTAL FAILURES: %d\n", $failures
detach
quit
