/*
 * WiFi 实现：ESP32（esp_wifi，事件驱动，STA 模式）
 * nvs/netif/event loop 在这里惰性初始化，bsp 显示部分不依赖网络。
 */
#include "../bsp_wifi.h"

#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define TAG "bsp_wifi"
#define SCAN_MAX 16

static struct {
    bool            inited;
    bool            scan_running;
    bool            scan_done;
    wifi_ap_record_t records[SCAN_MAX];
    uint16_t        record_num;
    bsp_wifi_state_t state;
    bool            connected;      /* GOT_IP / DISCONNECTED 事件维护 */
    char            target_ssid[BSP_WIFI_SSID_MAX + 1];
} W;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data);

static void ensure_init(void)
{
    if (W.inited) return;

    /* nvs 可能被写满/版本变更，抹掉重来（KlipperScreen 同款处理） */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi_event, NULL);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    W.inited = true;
}

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        W.record_num = SCAN_MAX;
        esp_wifi_scan_get_ap_records(&W.record_num, W.records);
        W.scan_running = false;
        W.scan_done = true;
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        W.connected = false;
        /* 连接中收到断开 = 失败；已连接时断开暂不追踪（里程碑 1 再做重连） */
        if (W.state == BSP_WIFI_CONNECTING) W.state = BSP_WIFI_FAILED;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        W.state = BSP_WIFI_CONNECTED;
        W.connected = true;
        ESP_LOGI(TAG, "connected, got ip");
    }
}

void bsp_wifi_init(void)
{
    ensure_init();
}

void bsp_wifi_scan_start(void)
{
    ensure_init();
    if (W.scan_running) return;
    W.scan_running = true;
    W.scan_done = false;
    /* 非阻塞扫描，结果由 WIFI_EVENT_SCAN_DONE 带回 */
    esp_wifi_scan_start(NULL, false);
}

int bsp_wifi_scan_poll(bsp_wifi_ap_t *out, int max)
{
    if (W.scan_running) return BSP_WIFI_SCAN_RUNNING;
    if (!W.scan_done)    return BSP_WIFI_SCAN_FAILED;

    int n = W.record_num < max ? W.record_num : max;
    for (int i = 0; i < n; i++) {
        strlcpy(out[i].ssid, (const char *)W.records[i].ssid, sizeof(out[i].ssid));
        out[i].rssi   = W.records[i].rssi;
        out[i].secure = W.records[i].authmode != WIFI_AUTH_OPEN;
    }
    return n;
}

void bsp_wifi_connect(const char *ssid, const char *password)
{
    ensure_init();
    esp_wifi_disconnect();

    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid));
    if (password && password[0])
        strlcpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = password && password[0] ? WIFI_AUTH_WPA_PSK : WIFI_AUTH_OPEN;

    strlcpy(W.target_ssid, ssid, sizeof(W.target_ssid));
    W.state = BSP_WIFI_CONNECTING;
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_connect();
}

bsp_wifi_state_t bsp_wifi_status(void)
{
    return W.state;
}

bool bsp_wifi_connected(void)
{
    return W.connected;
}
