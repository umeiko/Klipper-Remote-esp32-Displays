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

#ifdef __cplusplus
}
#endif
