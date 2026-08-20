/*
 * BSP: desktop（SDL2，Windows/Linux 同构）
 * 逻辑分辨率与 CYD 2432S028R 一致（320x240 横屏），窗口 2x 缩放，鼠标模拟触摸。
 */
#include "bsp.h"

void bsp_init(void)
{
    lv_init();

    lv_display_t *disp = lv_sdl_window_create(320, 240);
    lv_sdl_window_set_zoom(disp, 2);
    lv_sdl_window_set_title(disp, "Klipper Remote (desktop)");
    lv_sdl_mouse_create();
}

lv_display_t *bsp_get_display(void)
{
    return lv_display_get_default();
}

/* 桌面端 LVGL 单线程运行（LV_USE_OS=LV_OS_NONE），锁为空操作 */
void bsp_lvgl_lock(void)   {}
void bsp_lvgl_unlock(void) {}
