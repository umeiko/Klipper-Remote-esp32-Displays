/*
 * 打印状态：进度环 + 文件名 + 时间 + 控制按钮（对标 KlipperScreen job_status 简化版）
 */
#include "../theme.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"
#include "../mock_printer.h"
#include "../widgets/confirm.h"
#include <stdio.h>

static lv_obj_t *arc;
static lv_obj_t *lbl_pct;
static lv_obj_t *lbl_file;
static lv_obj_t *lbl_time;
static lv_obj_t *btn_pause;
static lv_obj_t *lbl_pause_icon;
static lv_obj_t *lbl_pause_text;
static lv_obj_t *btn_cancel;
static int cur_shown_pct10 = 0;   /* 当前显示的千分比（动画起点） */

static void arc_anim_cb(void *obj, int32_t v)
{
    lv_arc_set_value((lv_obj_t *)obj, v / 10);
    lv_label_set_text_fmt(lbl_pct, "%ld.%ld%%", (long)(v / 10), (long)(v % 10));
}

static void fmt_time(char *buf, size_t n, uint32_t s)
{
    snprintf(buf, n, "%02u:%02u:%02u", (unsigned)(s / 3600), (unsigned)((s % 3600) / 60), (unsigned)(s % 60));
}

static void update_ui(void)
{
    mock_state_t s = mock_state();
    lv_label_set_text(lbl_file, mock_filename()[0] ? mock_filename() : "空闲");

    char t1[16], t2[16], buf[48];
    fmt_time(t1, sizeof(t1), mock_print_elapsed_s());
    fmt_time(t2, sizeof(t2), mock_print_eta_s());
    snprintf(buf, sizeof(buf), "已用 %s\n剩余 %s", t1, s == MOCK_STATE_PRINTING ? t2 : "--:--:--");
    lv_label_set_text(lbl_time, buf);

    int32_t target = mock_progress_permille();
    if (target != cur_shown_pct10) {
        ui_anim_to(arc, arc_anim_cb, cur_shown_pct10, target, UI_ANIM_SLOW, lv_anim_path_ease_out);
        cur_shown_pct10 = target;
    }

    int printing = (s == MOCK_STATE_PRINTING);
    int paused = (s == MOCK_STATE_PAUSED);
    lv_label_set_text(lbl_pause_icon, paused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    lv_label_set_text(lbl_pause_text, paused ? "继续" : "暂停");
    if (printing || paused) {
        lv_obj_remove_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(btn_cancel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_cancel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_pause(lv_event_t *e)
{
    LV_UNUSED(e);
    if (mock_state() == MOCK_STATE_PRINTING) mock_print_pause();
    else                                     mock_print_resume();
    update_ui();
}

static void on_cancel(lv_event_t *e)
{
    LV_UNUSED(e);
    mock_print_cancel();
    ui_toast("已取消打印", THEME_COL_WARN);
    panel_mgr_back();
}

static void do_estop(void *ud)
{
    LV_UNUSED(ud);
    mock_emergency_stop();   /* 真实实现：Moonraker printer.emergency_stop */
    ui_toast("已急停（M112）", THEME_COL_ERROR);
}

static void on_estop(lv_event_t *e)
{
    LV_UNUSED(e);
    confirm_open("确认急停？\n打印机将立即停止所有运动和加热", "急停", do_estop, NULL);
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    /* 进度环 */
    arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 120, 120);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);   /* 清掉构造器默认的 135°~270° 指示弧 */
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc, theme_col(THEME_COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 9, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 9, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_align(arc, LV_ALIGN_TOP_LEFT, 14, THEME_TITLEBAR_H + 12);

    lbl_pct = theme_label(arc, "0%", THEME_FONT_L, THEME_COL_TEXT);
    lv_obj_center(lbl_pct);

    /* 右侧信息（限制在进度环右边的竖栏内，避免与圆环重叠） */
    lbl_file = theme_label(scr, "", THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_set_width(lbl_file, 160);
    lv_label_set_long_mode(lbl_file, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(lbl_file, LV_ALIGN_TOP_RIGHT, -10, THEME_TITLEBAR_H + 26);

    lbl_time = theme_label(scr, "", THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_set_width(lbl_time, 160);
    lv_obj_set_style_text_align(lbl_time, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, -10, THEME_TITLEBAR_H + 54);

    /* 底部按钮：暂停 / 取消 / 急停 */
    btn_pause = theme_button(scr, NULL, NULL, 1);
    lv_obj_set_size(btn_pause, 94, 36);
    lv_obj_align(btn_pause, LV_ALIGN_BOTTOM_LEFT, 10, -8);
    lv_obj_add_event_cb(btn_pause, on_pause, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_flow(btn_pause, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_pause, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_pause, 4, 0);
    lbl_pause_icon = theme_label(btn_pause, LV_SYMBOL_PAUSE, THEME_FONT_ICON, THEME_COL_TEXT);
    lbl_pause_text = theme_label(btn_pause, "暂停", THEME_FONT_S, THEME_COL_TEXT);

    /* 取消：次级危险操作，暗底红字；急停：最高优先级，实心红 */
    btn_cancel = theme_button(scr, LV_SYMBOL_STOP, "取消", 0);
    for (int i = 0, n = lv_obj_get_child_count(btn_cancel); i < n; i++)
        lv_obj_set_style_text_color(lv_obj_get_child(btn_cancel, i), theme_col(THEME_COL_ERROR), 0);
    lv_obj_set_size(btn_cancel, 94, 36);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(btn_cancel, on_cancel, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_estop = theme_button(scr, LV_SYMBOL_WARNING, "急停", 0);
    lv_obj_set_style_bg_color(btn_estop, theme_col(THEME_COL_ERROR), 0);
    lv_obj_set_size(btn_estop, 94, 36);
    lv_obj_align(btn_estop, LV_ALIGN_BOTTOM_RIGHT, -10, -8);
    lv_obj_add_event_cb(btn_estop, on_estop, LV_EVENT_CLICKED, NULL);

    return scr;
}

panel_def_t panel_job_status_def = {
    .name = "job_status", .title = "打印状态",
    .create = create,
    .on_show = update_ui,
    .on_tick = update_ui,
};
