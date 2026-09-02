#pragma once
/*
 * Moonraker WebSocket 客户端（esp32：esp_websocket_client；desktop：空桩）。
 * 职责：连接 ws://host:port/websocket，完成握手
 *   identify → server.info(等 klippy_connected) → objects.list → objects.subscribe
 * 之后把订阅快照与 notify_status_update 增量转交 printer_model_apply_status()，
 * 断线指数退避自动重连。
 */
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOONRAKER_OFFLINE = 0,   /* 未配置 / 未启动 / 断线等待重连 */
    MOONRAKER_CONNECTING,    /* WS 连接中或握手中 */
    MOONRAKER_READY,         /* 订阅完成，数据在更新 */
} moonraker_state_t;

/* 读 moonraker.conf，有配置且 WiFi 已连时启动连接（可重复调用，幂等） */
void moonraker_start(void);

/* 配置变更后调用：断开并按新配置重连 */
void moonraker_reload(void);

moonraker_state_t moonraker_state(void);

/* 发一条 JSON-RPC（fire-and-forget，无回调）。返回是否已发出。
 * params_json 为 JSON 片段（对象/数组文本），可为 NULL（无 params）。 */
bool moonraker_send_rpc(const char *method, const char *params_json);

#ifdef __cplusplus
}
#endif
