#ifndef WATCH_DRAW_DMA2D_H
#define WATCH_DRAW_DMA2D_H
/* LVGL's blend extension is called after clipping and stride calculation.
 * Unsupported/transparent cases retain LVGL's normal software renderer. */
lv_result_t watch_display_blend_rgb565(lv_draw_sw_blend_image_dsc_t *dsc);
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_ARGB8888(dsc) watch_display_blend_rgb565(dsc)
#endif
