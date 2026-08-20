#include "bsp.h"
#include "ui_app.h"

void app_main(void)
{
    bsp_init();            /* 显示 + 触摸 + LVGL 任务（核 1） */

    bsp_lvgl_lock();
    ui_app_create();       /* 与 desktop 后端共享的同一份 UI 代码 */
    bsp_lvgl_unlock();
}
