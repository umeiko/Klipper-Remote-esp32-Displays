/*
 * 文件列表（对标 KlipperScreen gcodes 简化版）：点文件名开始打印
 */
#include "../theme.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"
#include "../mock_printer.h"
#include <stdio.h>
#include <string.h>

static const struct { const char *name; float size_mb; } mock_files[] = {
    {"calibration_cube.gcode", 0.4f},
    {"3dbenchy.gcode", 3.2f},
    {"voron_cube.gcode", 12.8f},
    {"fan_duct_v2.gcode", 5.1f},
    {"phone_stand.gcode", 8.6f},
    {"ercf_gate.gcode", 1.9f},
};

static void on_file(lv_event_t *e)
{
    const char *name = (const char *)lv_event_get_user_data(e);
    if (mock_state() == MOCK_STATE_PRINTING) {
        ui_toast("正在打印中，无法开始新任务", THEME_COL_ERROR);
        return;
    }
    mock_print_start(name);
    ui_toast("开始打印", THEME_COL_OK);
    panel_mgr_open("job_status");
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 304, 240 - THEME_TITLEBAR_H - 12);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, THEME_TITLEBAR_H + 4);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, THEME_GAP, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    for (unsigned i = 0; i < sizeof(mock_files) / sizeof(mock_files[0]); i++) {
        lv_obj_t *row = theme_card(list);
        lv_obj_set_size(row, 284, 40);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, on_file, LV_EVENT_CLICKED, (void *)mock_files[i].name);

        lv_obj_t *ic = theme_label(row, LV_SYMBOL_FILE, THEME_FONT_ICON, THEME_COL_ACCENT);
        lv_obj_align(ic, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *name = theme_label(row, mock_files[i].name, THEME_FONT_S, THEME_COL_TEXT);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 24, 0);

        char sz[16];
        theme_fmt_float(sz, sizeof(sz), mock_files[i].size_mb, 1);
        strncat(sz, "MB", sizeof(sz) - strlen(sz) - 1);
        lv_obj_t *size = theme_label(row, sz, THEME_FONT_S, THEME_COL_TEXT_DIM);
        lv_obj_align(size, LV_ALIGN_RIGHT_MID, -4, 0);
    }

    return scr;
}

panel_def_t panel_files_def = {
    .name = "files", .title = "打印文件",
    .create = create,
    .on_show = NULL,
    .on_tick = NULL,
};
