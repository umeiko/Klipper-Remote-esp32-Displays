#pragma once
/*
 * 危险操作确认弹窗（急停/重启等）：遮罩 + 说明文字 + 取消/红色确认
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*confirm_cb_t)(void *ud);

/* 同一时刻只允许一个确认弹窗；ok_text 传 NULL 默认 "确认" */
void confirm_open(const char *text, const char *ok_text, confirm_cb_t cb, void *ud);

#ifdef __cplusplus
}
#endif
