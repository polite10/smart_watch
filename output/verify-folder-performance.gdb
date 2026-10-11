set pagination off
set confirm off
set remotetimeout 20
set $failures=0
target remote localhost:61238
tbreak refresh_rtc
continue
set $input=lv_indev_get_next(0)
set $input_timer=lv_indev_get_read_timer($input)
call lv_timer_pause($input_timer)
call lv_indev_reset($input,0)
call watch_navigation_show(MENU)
tbreak refresh_rtc
continue
set smartwatch_frame_max_ms=0
set $frames=smartwatch_frame_count
set $conversions=smartwatch_rgb565_dma_transfers
set $started=uwTick
tbreak folder_finished
call folder_show(1)
continue
if smartwatch_frame_count-$frames < 6 || smartwatch_rgb565_dma_transfers == $conversions || uwTick-$started > 600
  set $failures=$failures+1
end
printf "FOLDER_OPEN elapsed_ms=%u frames=%u max_flush_ms=%u last_total_ms=%u dma_conversions=%u\n", uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_max_ms,smartwatch_frame_total_ms,smartwatch_rgb565_dma_transfers-$conversions
finish
tbreak refresh_rtc
continue
set smartwatch_frame_max_ms=0
set $frames=smartwatch_frame_count
set $conversions=smartwatch_rgb565_dma_transfers
set $started=uwTick
tbreak folder_finished
call folder_show(0)
continue
if smartwatch_frame_count-$frames < 6 || smartwatch_rgb565_dma_transfers == $conversions || uwTick-$started > 600
  set $failures=$failures+1
end
printf "FOLDER_CLOSE elapsed_ms=%u frames=%u max_flush_ms=%u last_total_ms=%u dma_conversions=%u\n", uwTick-$started,smartwatch_frame_count-$frames,smartwatch_frame_max_ms,smartwatch_frame_total_ms,smartwatch_rgb565_dma_transfers-$conversions
finish
tbreak refresh_rtc
continue
call watch_navigation_show(HOME)
call lv_timer_resume($input_timer)
printf "HARDWARE_FOLDER_FAILURES=%u\n",$failures
detach
quit $failures
