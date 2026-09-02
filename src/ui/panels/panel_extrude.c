/*
 * 挤出/退料（对标 KlipperScreen extrude 简化版）
 */
#include "../theme.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"
#include "printer.h"
#include "../widgets/toggle_group.h"
#include <stdio.h>

#define MIN_EXTRUDE_TEMP 170.0f

static const float amt_table[] = {5.0f, 10.0f, 25.0f, 50.0f};
static int amt_idx = 1;
static lv_obj_t *lbl_temp;

static void on_amt(int idx, void *ud)
{
    LV_UNUSED(ud);
    amt_idx = idx;
}

static void on_extrude(lv_event_t *e)
{
    float dir = (intptr_t)lv_event_get_user_data(e) ? 1.0f : -1.0f;
    if (printer_temp_ext() < MIN_EXTRUDE_TEMP) {
        ui_toast("喷嘴温度过低，无法挤出", THEME_COL_ERROR);
        return;
    }
    printer_extrude(dir * amt_table[amt_idx]);
    ui_toast(dir > 0 ? "挤出中…" : "回抽中…", THEME_COL_EXTRUDER);
}

static void update_temp(void)
{
    lv_label_set_text_fmt(lbl_temp, "喷嘴 %d" "\xC2\xB0" "C（挤出需 ≥ %d" "\xC2\xB0" "C）",
                          (int)(printer_temp_ext() + 0.5f), (int)MIN_EXTRUDE_TEMP);
    lv_obj_set_style_text_color(lbl_temp,
        theme_col(printer_temp_ext() >= MIN_EXTRUDE_TEMP ? THEME_COL_TEXT_DIM : THEME_COL_WARN), 0);
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    static const char *amts[] = {"5", "10", "25", "50"};
    lv_obj_t *tg = toggle_group_create(scr, amts, 4, amt_idx, on_amt, NULL);
    lv_obj_set_size(tg, 304, 30);
    lv_obj_align(tg, LV_ALIGN_TOP_MID, 0, THEME_TITLEBAR_H + 6);

    lbl_temp = theme_label(scr, "", THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_MID, 0, THEME_TITLEBAR_H + 48);

    lv_obj_t *b_out = theme_button(scr, LV_SYMBOL_UPLOAD, "挤出", 1);
    lv_obj_set_size(b_out, 144, 64);
    lv_obj_align(b_out, LV_ALIGN_BOTTOM_LEFT, 10, -60);
    lv_obj_add_event_cb(b_out, on_extrude, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    lv_obj_t *b_in = theme_button(scr, LV_SYMBOL_DOWNLOAD, "回抽", 0);
    lv_obj_set_size(b_in, 144, 64);
    lv_obj_align(b_in, LV_ALIGN_BOTTOM_RIGHT, -10, -60);
    lv_obj_add_event_cb(b_in, on_extrude, LV_EVENT_CLICKED, (void *)(intptr_t)0);

    lv_obj_t *b_load = theme_button(scr, NULL, "装料", 0);
    lv_obj_set_size(b_load, 144, 32);
    lv_obj_align(b_load, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    lv_obj_t *b_unload = theme_button(scr, NULL, "退料", 0);
    lv_obj_set_size(b_unload, 144, 32);
    lv_obj_align(b_unload, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    return scr;
}

panel_def_t panel_extrude_def = {
    .name = "extrude", .title = "挤出",
    .create = create,
    .on_show = update_temp,
    .on_tick = update_temp,
};
