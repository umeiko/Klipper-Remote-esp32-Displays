/*
 * BSP: desktop（SDL2，Windows/Linux 同构）
 * 逻辑分辨率与 CYD 2432S028R 一致（320x240 横屏），窗口 2x 缩放，鼠标模拟触摸。
 */
#include "bsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bsp_init(void)
{
    lv_init();

    lv_display_t *disp = lv_sdl_window_create(320, 240);
    lv_sdl_window_set_zoom(disp, 2);
    lv_sdl_window_set_title(disp, "Klipper Remote (desktop)");
    lv_sdl_mouse_create();
}

/* ---------- 开机动画推屏（boot_anim 调用；首次调用时建全屏 canvas） ---------- */
static lv_obj_t  *boot_scr;
static lv_obj_t  *boot_canvas;
static uint16_t  *boot_buf;      /* 320*240 */

static void boot_cleanup_cb(lv_timer_t *t)
{
    /* 主界面加载后清理开场资源（此时主屏幕已激活，删旧屏幕安全） */
    if (boot_scr) lv_obj_delete(boot_scr);
    free(boot_buf);
    boot_scr = NULL;
    boot_buf = NULL;
    lv_timer_delete(t);
}

void bsp_lcd_push(int x, int y, int w, int h, const uint16_t *px)
{
    if (!boot_buf) {
        boot_buf = malloc(320 * 240 * 2);
        if (!boot_buf) return;
        memset(boot_buf, 0, 320 * 240 * 2);
        boot_scr = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
        lv_screen_load(boot_scr);
        boot_canvas = lv_canvas_create(boot_scr);
        lv_canvas_set_buffer(boot_canvas, boot_buf, 320, 240, LV_COLOR_FORMAT_RGB565);
        lv_timer_create(boot_cleanup_cb, 100, NULL);   /* 动画播完进主循环后自动清理 */
    }
    for (int r = 0; r < h; r++)
        memcpy(boot_buf + (y + r) * 320 + x, px + (size_t)r * w, (size_t)w * 2);
    lv_obj_invalidate(boot_canvas);
    lv_refr_now(NULL);
}

void bsp_delay_ms(uint32_t ms)
{
    lv_delay_ms(ms);
}

lv_display_t *bsp_get_display(void)
{
    return lv_display_get_default();
}

/* 桌面端 LVGL 单线程运行（LV_USE_OS=LV_OS_NONE），锁为空操作 */
void bsp_lvgl_lock(void)   {}
void bsp_lvgl_unlock(void) {}

void bsp_restart(void)
{
    /* 桌面端无「重启」概念：退出进程，重新启动即按新配置加载 */
    printf("bsp_restart: exit for restart\n");
    exit(0);
}

void bsp_set_brightness(int pct)
{
    /* 桌面端无背光硬件，仅打印便于调试 */
    printf("bsp_set_brightness: %d%%\n", pct);
}

void bsp_fade_out(uint32_t ms)
{
    /* 桌面端无背光，模拟耗时即可 */
    printf("bsp_fade_out: %ums\n", (unsigned)ms);
    lv_delay_ms(ms);
}

void bsp_set_screen_timeout(uint32_t sec)
{
    /* 桌面端不息屏，仅打印便于调试 */
    printf("bsp_set_screen_timeout: %us\n", (unsigned)sec);
}

void bsp_time_sync_from_host(const char *host, uint16_t port)
{
    /* 桌面端直接用本机时间，无需兜底 */
    (void)host; (void)port;
}
