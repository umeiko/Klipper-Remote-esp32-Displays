#pragma once
/* 数字键盘弹层（模态）：温度/距离等数值输入 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ok=1 确认 / 0 取消 */
typedef void (*keypad_cb_t)(float value, int ok, void *user_data);

void keypad_open(const char *title, float initial, keypad_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif
