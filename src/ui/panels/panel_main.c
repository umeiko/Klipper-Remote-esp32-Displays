/*
 * 主菜单：状态卡片 + 功能网格 + 急停/重启（对标 KlipperScreen main_menu）
 */
#include "../theme.h"
#include "../assets/icons.h"
#include "../panel_mgr.h"
#include "../ui_anim.h"
#include "printer.h"
#include "../widgets/confirm.h"
#include <stdio.h>

static lv_obj_t *lbl_state;
static lv_obj_t *dot_state;
static lv_obj_t *lbl_file;
static lv_obj_t *card_status;

static void on_menu(lv_event_t *e)
{
    panel_mgr_open((const char *)lv_event_get_user_data(e));
}

static void on_status_click(lv_event_t *e)
{
    LV_UNUSED(e);
    if (printer_state() == PRINTER_STATE_PRINTING || printer_state() == PRINTER_STATE_PAUSED)
        panel_mgr_open("job_status");
}

static void do_estop(void *ud)
{
    LV_UNUSED(ud);
    printer_emergency_stop();   /* 真实实现：Moonraker printer.emergency_stop */
    ui_toast("已急停（M112）", THEME_COL_ERROR);
}

static void do_restart(void *ud)
{
    LV_UNUSED(ud);
    printer_firmware_restart(); /* 真实实现：Moonraker printer.firmware_restart */
    ui_toast("已发送重启指令", THEME_COL_WARN);
}

static void on_estop(lv_event_t *e)
{
    LV_UNUSED(e);
    confirm_open("确认急停？\n打印机将立即停止所有运动和加热", "急停", do_estop, NULL);
}

static void on_restart(lv_event_t *e)
{
    LV_UNUSED(e);
    confirm_open("确认重启下位机？\n（FIRMWARE_RESTART）", "重启", do_restart, NULL);
}

static void update_state(void)
{
    static const struct { const char *text; uint32_t col; } st[] = {
        [PRINTER_STATE_STANDBY]      = {"空闲",     THEME_COL_TEXT_DIM},
        [PRINTER_STATE_PRINTING]     = {"打印中",   THEME_COL_OK},
        [PRINTER_STATE_PAUSED]       = {"已暂停",   THEME_COL_WARN},
        [PRINTER_STATE_COMPLETE]     = {"打印完成", THEME_COL_ACCENT},
        [PRINTER_STATE_DISCONNECTED] = {"未连接",   THEME_COL_TEXT_DIM},
        [PRINTER_STATE_ERROR]        = {"Klipper 异常", THEME_COL_ERROR},
    };
    printer_state_t s = printer_state();
    lv_label_set_text(lbl_state, st[s].text);
    lv_obj_set_style_bg_color(dot_state, theme_col(st[s].col), 0);
    if (s == PRINTER_STATE_PRINTING || s == PRINTER_STATE_PAUSED) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%s  %d.%d%%", printer_filename(),
                 printer_progress_permille() / 10, printer_progress_permille() % 10);
        lv_label_set_text(lbl_file, buf);
    } else {
        lv_label_set_text(lbl_file, "");
    }
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    /* 状态卡片 */
    card_status = theme_card(scr);
    lv_obj_set_size(card_status, 304, 40);
    lv_obj_align(card_status, LV_ALIGN_TOP_MID, 0, THEME_TITLEBAR_H + 4);
    lv_obj_add_flag(card_status, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card_status, on_status_click, LV_EVENT_CLICKED, NULL);

    dot_state = lv_obj_create(card_status);
    lv_obj_remove_style_all(dot_state);
    lv_obj_set_size(dot_state, 10, 10);
    lv_obj_set_style_radius(dot_state, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot_state, LV_OPA_COVER, 0);
    lv_obj_align(dot_state, LV_ALIGN_LEFT_MID, 2, 0);

    lbl_state = theme_label(card_status, "", THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(lbl_state, LV_ALIGN_LEFT_MID, 18, 0);

    lbl_file = theme_label(card_status, "", THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_align(lbl_file, LV_ALIGN_RIGHT_MID, -4, 0);

    /* 功能网格 3x2（KlipperScreen material-dark 图标） */
    static const struct { const lv_image_dsc_t *icon; const char *text, *panel; } items[] = {
        {&img_heater,   "温度", "temperature"},
        {&img_move,     "移动", "move"},
        {&img_extrude,  "挤出", "extrude"},
        {&img_files,    "文件", "files"},
        {&img_printer,  "打印", "job_status"},
        {&img_settings, "设置", "settings"},
    };
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, 304, 124);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, THEME_TITLEBAR_H + 48);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(grid, THEME_GAP, 0);
    lv_obj_set_style_pad_column(grid, THEME_GAP, 0);

    for (unsigned i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        lv_obj_t *b = theme_menu_button_img(grid, items[i].icon, items[i].text);
        lv_obj_set_size(b, 96, 59);
        lv_obj_add_event_cb(b, on_menu, LV_EVENT_CLICKED, (void *)items[i].panel);
    }

    /* 底部：急停（高优先级，红色实心）+ 重启下位机 */
    lv_obj_t *b_estop = theme_button(scr, LV_SYMBOL_WARNING, "急停", 0);
    lv_obj_set_style_bg_color(b_estop, theme_col(THEME_COL_ERROR), 0);
    lv_obj_set_size(b_estop, 148, 28);
    lv_obj_align(b_estop, LV_ALIGN_BOTTOM_LEFT, 8, -4);
    lv_obj_add_event_cb(b_estop, on_estop, LV_EVENT_CLICKED, NULL);

    lv_obj_t *b_restart = theme_button(scr, LV_SYMBOL_POWER, "重启下位机", 0);
    lv_obj_set_size(b_restart, 148, 28);
    lv_obj_align(b_restart, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
    lv_obj_add_event_cb(b_restart, on_restart, LV_EVENT_CLICKED, NULL);

    return scr;
}

panel_def_t panel_main_def = {
    .name = "main", .title = "Klipper Remote",
    .create = create,
    .on_show = update_state,
    .on_tick = update_state,
};
