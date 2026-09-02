#include "ui_app.h"
#include "panel_mgr.h"
#include "titlebar.h"
#include "printer.h"
#include "ui_anim.h"
#include "theme.h"

void ui_app_create(void)
{
    titlebar_init();       /* 常驻标题栏（layer_top），必须先于 panel_mgr_init */
    panel_mgr_init();      /* 加载主面板 */
    printer_set_refresh_hook(panel_mgr_tick);   /* 数据层 → UI 刷新回调 */
    printer_init();        /* 数据节拍：esp32=真实 Moonraker 模型，desktop=mock */
}

void ui_app_open(const char *panel)
{
    panel_mgr_open(panel);
}
