set pagination off
set confirm off
set remotetimeout 15
target remote localhost:61235
printf "INITIAL STATUS: %u FRAMES: %u\n", smartwatch_status, smartwatch_frame_count
if smartwatch_status != 4
  printf "ERROR: firmware is not running\n"
  detach
  quit 1
end
tbreak refresh_rtc
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
check (smartwatch_status==4) "UI running on real STM32"
check (sizeof(watch_data)==916) "Persistent data format unchanged"
check (hlcd_gfxmmu.Init.Buffers.Buf1Address!=0) "Second physical GFXMMU buffer configured"
check (smartwatch_frame_count>0) "Real display has presented completed frames"
check (!drawing_frame) "Renderer is idle at UI boundary"
dump binary memory ../output/settings-after-curved-boot.bin &watch_data (&watch_data+1)
set $original_style=watch_data.face_style
dump binary memory ../output/settings-flash-after.bin 0x083fc000 0x08400000
dump binary memory ../output/device-firmware.bin 0x08000000 (0x08000000+$firmware_size)
set smartwatch_frame_max_ms=0
call lv_screen_load(screens[MENU])
call lv_obj_invalidate(screens[MENU])
call lv_display_refr_timer(0)
printf "MENU FRAME: %u ms\n", smartwatch_frame_ms
printf "BUFFERS: %x %x FRONT %u GFX %x %x DMA FG %x OUT %x CR %x\n", hlcd_gfxmmu.Init.Buffers.Buf0Address,hlcd_gfxmmu.Init.Buffers.Buf1Address,front_buffer,hlcd_gfxmmu.Instance->B0CR,hlcd_gfxmmu.Instance->B1CR,display_dma.Instance->FGMAR,display_dma.Instance->OMAR,display_dma.Instance->CR
dump binary memory ../output/device-drawbuffer.bin &draw_buffer (&draw_buffer+1)
check (smartwatch_frame_ms<100) "Menu rendered and presented within 100ms"
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-menu-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_child_count(screens[MENU])==9) "Menu has title and eight icons without back button"
check (lv_obj_get_x(page_titles[MENU])+lv_obj_get_width(page_titles[MENU])/2==240) "App title centered on real display"
set $d=(lv_indev_data_t*)lv_malloc(sizeof(lv_indev_data_t))
set $d->point.x=240
set $d->point.y=240
set $d->state=LV_INDEV_STATE_RELEASED
call watch_ui_touch($d,99000)
set $d->state=LV_INDEV_STATE_RELEASED
call watch_ui_touch($d,99500)
call lv_indev_reset(0,0)
set display_awake=true
call watch_ui_display_power(true)
call lv_screen_load(screens[HOME])
set $d->point.x=240
set $d->point.y=240
set $d->state=LV_INDEV_STATE_PRESSED
call watch_ui_touch($d,100000)
set $d->state=LV_INDEV_STATE_RELEASED
call watch_ui_touch($d,100040)
check (!display_awake) "Single tap turns physical panel off"
set $d->state=LV_INDEV_STATE_PRESSED
call watch_ui_touch($d,100200)
set $d->state=LV_INDEV_STATE_RELEASED
call watch_ui_touch($d,100240)
check (display_awake&&lv_screen_active()==screens[HOME]) "Wake tap keeps clock visible"
set $d->state=LV_INDEV_STATE_PRESSED
call watch_ui_touch($d,100400)
set $d->state=LV_INDEV_STATE_PRESSED
set $d->point.x=280
call watch_ui_touch($d,100450)
set $d->state=LV_INDEV_STATE_RELEASED
call watch_ui_touch($d,100500)
check (lv_screen_active()==screens[MENU]) "Swipe unlocks to menu on device"
call lv_screen_load(screens[CALENDAR])
call lv_display_refr_timer(0)
printf "CALENDAR FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-calendar-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_child_count(calendar)==53) "Circular calendar created on device"
set $d->point.x=24
set $d->point.y=240
set $d->state=LV_INDEV_STATE_PRESSED
call watch_ui_touch($d,101000)
set $d->state=LV_INDEV_STATE_PRESSED
set $d->point.x=108
call watch_ui_touch($d,101080)
set $d->state=LV_INDEV_STATE_RELEASED
call watch_ui_touch($d,101100)
check (lv_screen_active()==screens[MENU]) "Left-edge swipe returns to menu on device"
call lv_obj_send_event(lv_obj_get_parent(note_labels[0]),LV_EVENT_CLICKED,0)
call lv_display_refr_timer(0)
printf "KEYBOARD FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-keyboard-framebuffer.bin $fb ($fb+0xb4000)
check (note_mode==0) "Alphabet keyboard opens in letters mode"
check (lv_label_get_text(note_keys[25])[0]==122) "Last alphabet letter visible on same keyboard"
call watch_faces_open()
call lv_display_refr_timer(0)
printf "FACES FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-faces-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_width(style_buttons[0])==140&&lv_obj_get_height(style_buttons[0])==140) "Circular face preview size verified"
check (lv_obj_get_child_count(screens[FACES])==7) "Four face gallery has no color controls"
check (lv_obj_get_width(apply_button)==64&&lv_obj_get_height(apply_button)==64) "Face confirmation is a 64px round button"
call lv_screen_load(screens[SETTINGS])
call lv_display_refr_timer(0)
printf "SETTINGS FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-settings-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_height(settings_rows[0])==64&&lv_obj_get_height(settings_rows[3])==64) "Settings use large capsule targets"
check (lv_obj_get_width(settings_switch)==46) "Theme switch visible on device"
call lv_screen_load(screens[CALCULATOR])
call lv_display_refr_timer(0)
printf "CALCULATOR FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-calculator-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_y(lv_obj_get_child(screens[CALCULATOR],4))==lv_obj_get_y(lv_obj_get_child(screens[CALCULATOR],7))) "First calculator row is straight"
check (lv_obj_get_y(lv_obj_get_child(screens[CALCULATOR],16))==lv_obj_get_y(lv_obj_get_child(screens[CALCULATOR],19))) "Last calculator row is straight"
call lv_screen_load(screens[WATER])
call lv_display_refr_timer(0)
printf "WATER FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-water-framebuffer.bin $fb ($fb+0xb4000)
call lv_screen_load(screens[ALARMS])
call lv_display_refr_timer(0)
printf "ALARMS FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-alarms-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_height(lv_obj_get_parent(alarm_labels[0]))==88) "Alarm time cards have large targets"
set watch_data.face_style=NEON
call lv_screen_load(screens[HOME])
call watch_faces_apply_saved()
call lv_display_refr_timer(0)
printf "NEON FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-neon-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_y(face_alarm[NEON])==376) "Neon alarm status moved inside ring"
set watch_data.face_style=CLASSIC
call watch_faces_apply_saved()
call lv_display_refr_timer(0)
printf "CLASSIC FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-classic-framebuffer.bin $fb ($fb+0xb4000)
check (lv_obj_get_child_count(face_roots[CLASSIC])==71) "Analog dial omits the redundant digital seconds label"
check (lv_obj_get_x(center_pin)+lv_obj_get_width(center_pin)/2==240&&lv_obj_get_y(center_pin)+lv_obj_get_height(center_pin)/2==240) "Analog hands pivot at the exact screen center"
check (lv_obj_get_y(classic_numbers[0])+lv_obj_get_height(classic_numbers[0])/2==104&&lv_obj_get_y(classic_numbers[3])+lv_obj_get_height(classic_numbers[3])/2==376) "Analog vertical numerals have equal spacing from center"
set watch_data.face_style=ORBIT
call watch_faces_apply_saved()
call lv_display_refr_timer(0)
printf "ORBIT FRAME: %u ms\n", smartwatch_frame_ms
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-orbit-framebuffer.bin $fb ($fb+0xb4000)
check (!lv_obj_has_flag(face_roots[ORBIT],LV_OBJ_FLAG_HIDDEN)) "New Orbit face renders on device"
set watch_data.face_style=$original_style
call watch_faces_apply_saved()
call lv_free($d)
call lv_screen_load(screens[HOME])
call lv_display_refr_timer(0)
printf "HOME FRAME: %u ms MAX: %u FRAMES: %u\n", smartwatch_frame_ms,smartwatch_frame_max_ms,smartwatch_frame_count
check (smartwatch_status==4) "No display timeout or hardware error"
check (hlcd_gfxmmu.ErrorCode==0) "No GFXMMU mapping errors"
check (display_dma.ErrorCode==0) "No DMA2D transfer errors"
check (display_dma.Instance->FGMAR!=0&&display_dma.Instance->OMAR!=0) "DMA2D performed buffer transfers"
check ((*(unsigned*)0x40030400)&1) "Instruction cache enabled on device"
check (smartwatch_frame_max_ms<150) "Full-screen rendering stays below the 150ms regression limit"
printf "HARDWARE FAILURES: %u\n", $failures
detach
quit
