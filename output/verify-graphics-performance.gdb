set pagination off
set confirm off
set remotetimeout 20
set $failures=0
target remote localhost:61239
tbreak refresh_rtc
continue
set $input=lv_indev_get_next(0)
set $input_timer=lv_indev_get_read_timer($input)
call lv_timer_pause($input_timer)
call lv_indev_reset($input,0)
call watch_navigation_show(MENU)
tbreak refresh_rtc
continue
set $round=0
while $round<3
  set smartwatch_frame_total_max_ms=0
  set smartwatch_frame_max_ms=0
  set $frames=smartwatch_frame_count
  set $gpu=smartwatch_gpu_completions
  set $started=uwTick
  tbreak folder_finished
  call folder_show(1)
  continue
  printf "FOLDER_OPEN round=%u elapsed_ms=%u frames=%u max_flush_ms=%u max_total_ms=%u gpu_commands=%u\n", $round,uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_max_ms,smartwatch_frame_total_max_ms,smartwatch_gpu_completions-$gpu
  if smartwatch_frame_count-$frames<8 || uwTick-$started>500 || smartwatch_frame_total_max_ms>60 || smartwatch_gpu_completions==$gpu
    set $failures=$failures+1
  end
  finish
  tbreak refresh_rtc
  continue
  set smartwatch_frame_total_max_ms=0
  set smartwatch_frame_max_ms=0
  set $frames=smartwatch_frame_count
  set $gpu=smartwatch_gpu_completions
  set $started=uwTick
  tbreak folder_finished
  call folder_show(0)
  continue
  printf "FOLDER_CLOSE round=%u elapsed_ms=%u frames=%u max_flush_ms=%u max_total_ms=%u gpu_commands=%u\n", $round,uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_max_ms,smartwatch_frame_total_max_ms,smartwatch_gpu_completions-$gpu
  if smartwatch_frame_count-$frames<8 || uwTick-$started>500 || smartwatch_frame_total_max_ms>60 || smartwatch_gpu_completions==$gpu
    set $failures=$failures+1
  end
  finish
  tbreak refresh_rtc
  continue
  set $round=$round+1
end
# The released wheel must keep presenting frames throughout its longer glide.
call watch_menu_reset()
call ring_layout(0,0)
set ring_velocity=3600
set ring_dragging=1
set ring_sample_tick=uwTick
set smartwatch_frame_total_max_ms=0
set $frames=smartwatch_frame_count
set $started=uwTick
tbreak ring_finished
call watch_menu_ring_end(uwTick)
continue
printf "WHEEL elapsed_ms=%u frames=%u max_total_ms=%u\n",uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_total_max_ms
if smartwatch_frame_count-$frames<12 || uwTick-$started>900 || smartwatch_frame_total_max_ms>60
  set $failures=$failures+1
end
finish
tbreak refresh_rtc
continue
# Count actual presented frames in an entry window; the ARM UI test checks its
# intermediate coordinates and exact 280 ms completion without debugger stops.
set smartwatch_frame_total_max_ms=0
set $frames=smartwatch_frame_count
set $started=uwTick
call watch_navigation_show(SETTINGS)
tbreak refresh_rtc
continue
printf "APP_ENTRY window_ms=%u frames=%u max_total_ms=%u\n",uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_total_max_ms
if smartwatch_frame_count-$frames<5 || smartwatch_frame_total_max_ms>60
  set $failures=$failures+1
end
set $round=0
while $round<3
  call flappy_open()
  tbreak refresh_rtc
  continue
  call watch_flappy_vm_flap()
  call flappy_render()
  set smartwatch_frame_total_max_ms=0
  set smartwatch_frame_max_ms=0
  set smartwatch_render_total_ms=0
  set $frames=smartwatch_frame_count
  set $gpu=smartwatch_gpu_completions
  set $started=uwTick
  tbreak refresh_rtc
  continue
  printf "FLAPPY round=%u elapsed_ms=%u frames=%u max_flush_ms=%u max_total_ms=%u render_total_ms=%u gpu_commands=%u scenery_steps=%u\n", $round,uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_max_ms,smartwatch_frame_total_max_ms,smartwatch_render_total_ms,smartwatch_gpu_completions-$gpu,scenery_steps
  if smartwatch_frame_count-$frames<28 || smartwatch_frame_total_max_ms>60 || smartwatch_gpu_completions==$gpu
    set $failures=$failures+1
  end
  set $round=$round+1
end
# Stable real framebuffer captures for alpha/rotation/layout inspection.
call watch_navigation_show(MENU)
call watch_menu_reset()
call ring_layout(0,0)
tbreak refresh_rtc
continue
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-gpu-menu-framebuffer.bin $fb ($fb+0xb4000)
call folder_show(1)
call lv_anim_delete(0,0)
set folder_moving=1
call lv_obj_add_flag(menu_folder,LV_OBJ_FLAG_HIDDEN)
set $i=0
while $i<3
  call lv_obj_add_flag(menu_games[$i],LV_OBJ_FLAG_HIDDEN)
  set $i=$i+1
end
call lv_obj_remove_flag(folder_canvas,LV_OBJ_FLAG_HIDDEN)
call folder_layout(0,128)
tbreak refresh_rtc
continue
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-gpu-folder-framebuffer.bin $fb ($fb+0xb4000)
call watch_menu_reset()
call flappy_open()
call watch_flappy_vm_new(23)
call flappy_render()
tbreak refresh_rtc
continue
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-gpu-flappy-framebuffer.bin $fb ($fb+0xb4000)
call watch_flappy_vm_flap()
call watch_flappy_vm_pause()
call flappy_render()
tbreak refresh_rtc
continue
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-gpu-flappy-tilt-framebuffer.bin $fb ($fb+0xb4000)
dump binary memory ../output/settings-after-gpu.bin 0x083fc000 0x08400000
printf "FINAL UI_STATUS=%u GPU_TIMEOUTS=%u UART_ERRORS=%u\n",smartwatch_status,smartwatch_gpu_timeouts,'led_button_demo.c'::uart_errors
if smartwatch_status!=4 || smartwatch_gpu_timeouts!=0 || 'led_button_demo.c'::uart_errors!=0
  set $failures=$failures+1
end
call watch_navigation_show(HOME)
call lv_timer_resume($input_timer)
printf "GRAPHICS_FAILURES=%u\n",$failures
if $failures==0
  echo GRAPHICS_CHECK_PASSED\n
end
detach
quit $failures
