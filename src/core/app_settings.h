#pragma once
/*
 * 应用配置：WiFi 凭据 + Moonraker 连接参数。
 * 文件格式为简单 key=value 行（# 注释），两端通用，不引 JSON 库：
 *   network.conf  : ssid=... / pass=...
 *   moonraker.conf: host=... / port=7125 / api_key=...（可空，trusted_clients 免鉴权）
 * 存储介质由 bsp_conf 决定（esp32=LittleFS，desktop=本地文件）。
 */
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ssid[33];
    char pass[64];
    bool valid;
} wifi_conf_t;

typedef struct {
    char     host[64];
    uint16_t port;              /* 缺省 7125 */
    char     api_key[64];       /* 可空 */
    bool     valid;
} moonraker_conf_t;

bool settings_load_wifi(wifi_conf_t *out);
bool settings_save_wifi(const wifi_conf_t *in);

bool settings_load_moonraker(moonraker_conf_t *out);
bool settings_save_moonraker(const moonraker_conf_t *in);

#ifdef __cplusplus
}
#endif
