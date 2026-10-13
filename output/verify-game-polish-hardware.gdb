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
set smartwatch_frame_total_max_ms=0
set $frames=smartwatch_frame_count
set $started=uwTick
tbreak folder_finished
call folder_show(1)
continue
printf "FOLDER elapsed_ms=%u frames=%u max_total_ms=%u\n",uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_total_max_ms
if smartwatch_frame_count-$frames<6 || smartwatch_frame_total_max_ms>60
  set $failures=$failures+1
end
finish
call flappy_open()
tbreak refresh_rtc
continue
set flappy.pipes[0].bee=1
call watch_flappy_vm_flap()
call flappy_render()
set smartwatch_frame_total_max_ms=0
set $frames=smartwatch_frame_count
set $started=uwTick
tbreak refresh_rtc
continue
printf "FLAPPY elapsed_ms=%u frames=%u max_total_ms=%u\n",uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_total_max_ms
if smartwatch_frame_count-$frames<28 || smartwatch_frame_total_max_ms>60
  set $failures=$failures+1
end
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-polish-flappy-framebuffer.bin $fb ($fb+0xb4000)
call snake_open()
tbreak refresh_rtc
continue
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-polish-snake-framebuffer.bin $fb ($fb+0xb4000)
call watch_snake_vm_start()
set smartwatch_frame_total_max_ms=0
set $frames=smartwatch_frame_count
set $started=uwTick
tbreak refresh_rtc
continue
printf "SNAKE elapsed_ms=%u frames=%u max_total_ms=%u\n",uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_total_max_ms
if smartwatch_frame_count-$frames<20 || smartwatch_frame_total_max_ms>60
  set $failures=$failures+1
end
call puzzle_open()
tbreak refresh_rtc
continue
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-polish-puzzle-framebuffer.bin $fb ($fb+0xb4000)
call watch_navigation_show(DMA_STATUS)
set $uart='led_button_demo.c'::uart_completed
set $bytes='led_button_demo.c'::uart_bytes
set $heartbeat=smartwatch_heartbeat
tbreak refresh_rtc
continue
printf "DMA completed_before=%u completed_after=%u bytes_before=%u bytes_after=%u message_bytes=%u errors=%u\n",$uart,'led_button_demo.c'::uart_completed,$bytes,'led_button_demo.c'::uart_bytes,'led_button_demo.c'::uart_message_bytes,'led_button_demo.c'::uart_errors
printf "FINAL UI_STATUS=%u HEARTBEAT_BEFORE=%u HEARTBEAT_AFTER=%u GPU_TIMEOUTS=%u\n",smartwatch_status,$heartbeat,smartwatch_heartbeat,smartwatch_gpu_timeouts
if smartwatch_status!=4 || smartwatch_heartbeat<=$heartbeat || smartwatch_gpu_timeouts!=0 || 'led_button_demo.c'::uart_errors!=0 || 'led_button_demo.c'::uart_completed<=$uart || 'led_button_demo.c'::uart_bytes<=$bytes
  set $failures=$failures+1
end
set $fb=front_buffer?hlcd_gfxmmu.Init.Buffers.Buf1Address:hlcd_gfxmmu.Init.Buffers.Buf0Address
dump binary memory ../output/device-polish-dma-framebuffer.bin $fb ($fb+0xb4000)
dump binary memory ../output/settings-after-game-polish.bin 0x083fc000 0x08400000
call watch_navigation_show(HOME)
call lv_timer_resume($input_timer)
printf "GAME_POLISH_FAILURES=%u\n",$failures
if $failures==0
  echo GAME_POLISH_HARDWARE_PASSED\n
end
detach
quit $failures
