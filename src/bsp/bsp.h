#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 板级初始化：显示 + 触摸 + LVGL 节拍任务 */
void bsp_init(void);

lv_display_t *bsp_get_display(void);

/* LVGL 线程互斥（所有 LVGL API 调用必须持锁） */
void bsp_lvgl_lock(void);
void bsp_lvgl_unlock(void);

/* 重启设备（esp32=esp_restart，desktop=退出进程；用于语言切换等需重建全部 UI 的场景） */
void bsp_restart(void);

/* 直接把一块 RGB565 像素推上屏 + 毫秒延时（开机动画等 LVGL 场景外渲染用；
   esp32 在 LVGL 任务已启动后调用须先持 bsp_lvgl_lock） */
void bsp_lcd_push(int x, int y, int w, int h, const uint16_t *px);
void bsp_delay_ms(uint32_t ms);

/* 背光亮度 0-100（desktop 为空操作；esp32 走 LEDC PWM） */
void bsp_set_brightness(int pct);

/* 背光从当前值在 ms 内渐变到纯黑（语言切换重启前的淡出；阻塞至渐变完成） */
void bsp_fade_out(uint32_t ms);

/* 自动息屏超时（秒，0=永不）。超时后关背光，任意触摸唤醒并恢复亮度 */
void bsp_set_screen_timeout(uint32_t sec);

/* 内网时间兜底：从 Moonraker 主机的 HTTP Date 头同步系统时间。
   SNTP 已同步则跳过；异步执行不阻塞调用方（desktop 空操作）。 */
void bsp_time_sync_from_host(const char *host, uint16_t port);

#ifdef __cplusplus
}
#endif
