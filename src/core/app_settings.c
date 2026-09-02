/*
 * 应用配置读写：key=value 行格式，见 app_settings.h
 */
#include "app_settings.h"
#include "bsp_conf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CONF_BUF_SIZE 512

/* 在 buf（key=value 行集合）里找 key，把值拷到 out。返回是否命中 */
static bool kv_get(const char *buf, const char *key, char *out, size_t out_len)
{
    size_t klen = strlen(key);
    const char *p = buf;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t line_len = eol ? (size_t)(eol - p) : strlen(p);
        if (line_len && p[0] != '#' && strncmp(p, key, klen) == 0 && p[klen] == '=') {
            size_t vlen = line_len - klen - 1;
            const char *v = p + klen + 1;
            /* 去掉行尾 \r */
            while (vlen && (v[vlen - 1] == '\r' || v[vlen - 1] == ' ')) vlen--;
            if (vlen >= out_len) vlen = out_len - 1;
            memcpy(out, v, vlen);
            out[vlen] = 0;
            return true;
        }
        if (!eol) break;
        p = eol + 1;
    }
    return false;
}

bool settings_load_wifi(wifi_conf_t *out)
{
    char buf[CONF_BUF_SIZE];
    memset(out, 0, sizeof(*out));
    if (bsp_conf_read("network.conf", buf, sizeof(buf)) < 0) return false;
    if (!kv_get(buf, "ssid", out->ssid, sizeof(out->ssid)) || !out->ssid[0]) return false;
    kv_get(buf, "pass", out->pass, sizeof(out->pass));   /* 开放网络可空 */
    out->valid = true;
    return true;
}

bool settings_save_wifi(const wifi_conf_t *in)
{
    char buf[CONF_BUF_SIZE];
    snprintf(buf, sizeof(buf), "# WiFi credentials\nssid=%s\npass=%s\n", in->ssid, in->pass);
    return bsp_conf_write("network.conf", buf) == 0;
}

bool settings_load_moonraker(moonraker_conf_t *out)
{
    char buf[CONF_BUF_SIZE];
    memset(out, 0, sizeof(*out));
    out->port = 7125;
    if (bsp_conf_read("moonraker.conf", buf, sizeof(buf)) < 0) return false;
    if (!kv_get(buf, "host", out->host, sizeof(out->host)) || !out->host[0]) return false;
    char port[8];
    if (kv_get(buf, "port", port, sizeof(port)) && port[0]) {
        int p = atoi(port);
        if (p > 0 && p < 65536) out->port = (uint16_t)p;
    }
    kv_get(buf, "api_key", out->api_key, sizeof(out->api_key));
    out->valid = true;
    return true;
}

bool settings_save_moonraker(const moonraker_conf_t *in)
{
    char buf[CONF_BUF_SIZE];
    snprintf(buf, sizeof(buf), "# Moonraker connection\nhost=%s\nport=%u\napi_key=%s\n",
             in->host, (unsigned)in->port, in->api_key);
    return bsp_conf_write("moonraker.conf", buf) == 0;
}
