/*
 * WiFi 实现：Windows 桌面（netsh wlan）
 * 扫描/连接都是阻塞命令，放进后台线程，UI 轮询状态。
 * netsh 输出是 OEM 代码页（中文系统为 GBK），每行先转成 UTF-8 再解析，
 * 同时兼容英文（"State : connected"）与中文（"状态 : 已连接"）关键词。
 */
#include "../bsp_wifi.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define SCAN_MAX      24
#define CONNECT_TIMEOUT_S 15

static struct {
    CRITICAL_SECTION  lock;
    volatile int      scan_state;   /* 0 idle / 1 running / 2 done / 3 failed */
    bsp_wifi_ap_t     aps[SCAN_MAX];
    int               ap_count;
    volatile bsp_wifi_state_t conn_state;
    volatile int      net_connected; /* 后台轮询缓存：接口当前是否连着任意 AP */
    int               inited;
    char              target_ssid[BSP_WIFI_SSID_MAX + 1];
} W = { .scan_state = 0, .conn_state = BSP_WIFI_IDLE };

static void oem_to_utf8(const char *in, char *out, size_t out_sz)
{
    /* 有的系统（开了 UTF-8 Beta 或 chcp 65001）netsh 直接输出 UTF-8，
       先按 UTF-8 试探，合法则原样保留，否则按 OEM/ACP 代码页转码 */
    wchar_t w[512];
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in, -1, w, 512) > 0) {
        strncpy(out, in, out_sz - 1);
        out[out_sz - 1] = 0;
        return;
    }
    MultiByteToWideChar(CP_ACP, 0, in, -1, w, 512);
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out, (int)out_sz, NULL, NULL);
}

static void utf8_to_oem(const char *in, char *out, size_t out_sz)
{
    wchar_t w[256];
    MultiByteToWideChar(CP_UTF8, 0, in, -1, w, 256);
    WideCharToMultiByte(CP_ACP, 0, w, -1, out, (int)out_sz, NULL, NULL);
}

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

/* ---------- 扫描 ---------- */

static DWORD WINAPI scan_thread(void *unused)
{
    (void)unused;
    bsp_wifi_ap_t local[SCAN_MAX];
    int n = 0, cur = -1;

    FILE *fp = _popen("netsh wlan show networks mode=bssid 2>NUL", "r");
    if (!fp) { W.scan_state = 3; return 0; }

    char raw[512], line[512];
    while (fgets(raw, sizeof(raw), fp)) {
        oem_to_utf8(raw, line, sizeof(line));

        if (strstr(line, "SSID") && !strstr(line, "BSSID")) {
            char *colon = strchr(line, ':');
            if (!colon) continue;
            char *ssid = trim(colon + 1);
            if (!ssid[0]) { cur = -1; continue; }          /* 隐藏网络 */
            int dup = 0;
            for (int i = 0; i < n; i++)
                if (strcmp(local[i].ssid, ssid) == 0) { dup = 1; break; }
            if (dup) { cur = -1; continue; }
            if (n >= SCAN_MAX) { cur = -1; continue; }
            cur = n++;
            memset(&local[cur], 0, sizeof(local[cur]));
            strncpy(local[cur].ssid, ssid, BSP_WIFI_SSID_MAX);
        } else if (cur >= 0 && (strstr(line, "Authentication") || strstr(line, "身份验证"))) {
            local[cur].secure = !(strstr(line, "Open") || strstr(line, "开放"));
        } else if (cur >= 0 && local[cur].rssi == 0 &&
                   (strstr(line, "Signal") || strstr(line, "信号"))) {
            int pct = 0;
            if (sscanf(strchr(line, ':') ? strchr(line, ':') + 1 : line, "%d%%", &pct) == 1)
                local[cur].rssi = pct / 2 - 100;           /* 百分比粗略换算 dBm */
        }
    }
    _pclose(fp);

    EnterCriticalSection(&W.lock);
    memcpy(W.aps, local, n * sizeof(bsp_wifi_ap_t));
    W.ap_count = n;
    LeaveCriticalSection(&W.lock);
    W.scan_state = 2;
    return 0;
}

/* ---------- 连接 ---------- */

static int check_connected(const char *ssid)
{
    FILE *fp = _popen("netsh wlan show interfaces 2>NUL", "r");
    if (!fp) return 0;

    int state_ok = 0, ssid_ok = 0;
    char raw[512], line[512];
    while (fgets(raw, sizeof(raw), fp)) {
        oem_to_utf8(raw, line, sizeof(line));
        if ((strstr(line, "State") || strstr(line, "状态")) && strchr(line, ':')) {
            char *v = trim(strchr(line, ':') + 1);
            if (strstr(v, "connected") || strstr(v, "已连接")) state_ok = 1;
        } else if (strstr(line, "SSID") && !strstr(line, "BSSID") && strchr(line, ':')) {
            char *v = trim(strchr(line, ':') + 1);
            if (strcmp(v, ssid) == 0) ssid_ok = 1;
        }
    }
    _pclose(fp);
    return state_ok && ssid_ok;
}

/* netsh 需要 profile 文件才能连接带密码的网络 */
static void xml_escape(const char *in, char *out, size_t out_sz)
{
    size_t o = 0;
    for (; *in && o + 6 < out_sz; in++) {
        const char *rep = NULL;
        if (*in == '&') rep = "&amp;";
        else if (*in == '<') rep = "&lt;";
        else if (*in == '>') rep = "&gt;";
        else if (*in == '"') rep = "&quot;";
        if (rep) { o += snprintf(out + o, out_sz - o, "%s", rep); }
        else out[o++] = *in;
    }
    out[o] = 0;
}

static DWORD WINAPI connect_thread(void *password)
{
    const char *pass = (const char *)password;

    char ssid_x[128], pass_x[256];
    xml_escape(W.target_ssid, ssid_x, sizeof(ssid_x));
    xml_escape(pass ? pass : "", pass_x, sizeof(pass_x));

    char path[MAX_PATH];
    GetTempPathA(sizeof(path), path);
    strncat(path, "krd_wifi_profile.xml", sizeof(path) - strlen(path) - 1);

    FILE *f = fopen(path, "w");
    if (!f) { W.conn_state = BSP_WIFI_FAILED; return 0; }
    fprintf(f,
        "<?xml version=\"1.0\"?>\n"
        "<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">\n"
        "  <name>%s</name>\n"
        "  <SSIDConfig><SSID><name>%s</name></SSID></SSIDConfig>\n"
        "  <connectionType>ESS</connectionType>\n"
        "  <connectionMode>manual</connectionMode>\n"
        "  <MSM><security>\n"
        "    <authEncryption><authentication>WPA2PSK</authentication>"
        "<encryption>AES</encryption><useOneX>false</useOneX></authEncryption>\n"
        "    <sharedKey><keyType>passPhrase</keyType><protected>false</protected>"
        "<keyMaterial>%s</keyMaterial></sharedKey>\n"
        "  </security></MSM>\n"
        "</WLANProfile>\n", ssid_x, ssid_x, pass_x);
    fclose(f);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "netsh wlan add profile filename=\"%s\" user=current >NUL 2>&1", path);
    system(cmd);

    /* 命令行参数要转回 OEM 代码页，否则中文 SSID 乱码 */
    char ssid_oem[BSP_WIFI_SSID_MAX * 2 + 1];
    utf8_to_oem(W.target_ssid, ssid_oem, sizeof(ssid_oem));
    snprintf(cmd, sizeof(cmd), "netsh wlan connect name=\"%s\" >NUL 2>&1", ssid_oem);
    system(cmd);
    DeleteFileA(path);

    for (int i = 0; i < CONNECT_TIMEOUT_S; i++) {
        Sleep(1000);
        if (check_connected(W.target_ssid)) {
            W.conn_state = BSP_WIFI_CONNECTED;
            return 0;
        }
    }
    W.conn_state = BSP_WIFI_FAILED;
    return 0;
}

/* ---------- 接口 ---------- */

/* 后台轮询接口真实状态（标题栏图标用），5 秒一次 */
static DWORD WINAPI poll_thread(void *unused)
{
    (void)unused;
    for (;;) {
        FILE *fp = _popen("netsh wlan show interfaces 2>NUL", "r");
        int ok = 0;
        if (fp) {
            char raw[512], line[512];
            while (fgets(raw, sizeof(raw), fp)) {
                oem_to_utf8(raw, line, sizeof(line));
                if ((strstr(line, "State") || strstr(line, "状态")) && strchr(line, ':')) {
                    char *v = trim(strchr(line, ':') + 1);
                    if (strstr(v, "connected") || strstr(v, "已连接")) ok = 1;
                }
            }
            _pclose(fp);
        }
        W.net_connected = ok;
        Sleep(5000);
    }
    return 0;
}

void bsp_wifi_init(void)
{
    if (W.inited) return;
    W.inited = 1;
    InitializeCriticalSection(&W.lock);
    CreateThread(NULL, 0, poll_thread, NULL, 0, NULL);
}

void bsp_wifi_scan_start(void)
{
    if (W.scan_state == 1) return;
    W.scan_state = 1;
    CreateThread(NULL, 0, scan_thread, NULL, 0, NULL);
}

int bsp_wifi_scan_poll(bsp_wifi_ap_t *out, int max)
{
    if (W.scan_state == 1) return BSP_WIFI_SCAN_RUNNING;
    if (W.scan_state != 2) return BSP_WIFI_SCAN_FAILED;
    W.scan_state = 0;

    EnterCriticalSection(&W.lock);
    int n = W.ap_count < max ? W.ap_count : max;
    memcpy(out, W.aps, n * sizeof(bsp_wifi_ap_t));
    LeaveCriticalSection(&W.lock);
    return n;
}

void bsp_wifi_connect(const char *ssid, const char *password)
{
    if (W.conn_state == BSP_WIFI_CONNECTING) return;   /* 简单起见：不抢跑 */
    strncpy(W.target_ssid, ssid, BSP_WIFI_SSID_MAX);
    W.target_ssid[BSP_WIFI_SSID_MAX] = 0;
    W.conn_state = BSP_WIFI_CONNECTING;
    /* password 由调用方保证在连接期间有效（UI 侧用静态缓冲） */
    CreateThread(NULL, 0, connect_thread, (void *)password, 0, NULL);
}

bsp_wifi_state_t bsp_wifi_status(void)
{
    return W.conn_state;
}

bool bsp_wifi_connected(void)
{
    return W.net_connected != 0;
}

#endif /* _WIN32 */
