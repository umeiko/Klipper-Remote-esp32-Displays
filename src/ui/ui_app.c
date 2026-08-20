#include "ui_app.h"
#include "panel_mgr.h"
#include "titlebar.h"
#include "mock_printer.h"
#include "ui_anim.h"
#include "theme.h"

void ui_app_create(void)
{
    titlebar_init();       /* 常驻标题栏（layer_top），必须先于 panel_mgr_init */
    panel_mgr_init();      /* 加载主面板 */
    mock_printer_init();   /* 数据节拍（mock，里程碑 1 替换为真实 Moonraker 数据） */
    ui_toast("Connected: mock-printer", THEME_COL_OK);
}

void ui_app_open(const char *panel)
{
    panel_mgr_open(panel);
}
