#include "bsp.h"
#include "bsp_wifi.h"
#include "ui_app.h"
#include "app_settings.h"

void debug_cli_start(void);

void app_main(void)
{
    bsp_init();            /* 显示 + 触摸 + LVGL 任务（核 1） */

    bsp_lvgl_lock();
    ui_app_create();       /* 与 desktop 后端共享的同一份 UI 代码 */
    bsp_lvgl_unlock();

    /* WiFi 自动回连：有 network.conf 就用保存的凭据连接（Moonraker 客户端
       由 printer_model 的 2s 轮询在 WiFi 就绪后拉起） */
    wifi_conf_t wc;
    if (settings_load_wifi(&wc))
        bsp_wifi_connect(wc.ssid, wc.pass[0] ? wc.pass : NULL);

    debug_cli_start();       /* 串口调试命令行（help 查看） */
}
