/*
 * 点动移动：距离档 + X/Y/Z 步进 + 归位（对标 KlipperScreen move 简化版）
 */
#include "../theme.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"
#include "printer.h"
#include "../widgets/toggle_group.h"
#include <stdio.h>

static const float dist_table[] = {0.1f, 1.0f, 10.0f, 50.0f};
static int dist_idx = 1;
static lv_obj_t *lbl_pos[3];

static void on_dist(int idx, void *ud)
{
    LV_UNUSED(ud);
    dist_idx = idx;
}

static void on_jog(lv_event_t *e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);   /* axis*2 + (dir>0) */
    int axis = code / 2;
    float dir = (code % 2) ? 1.0f : -1.0f;
    printer_jog(axis, dir * dist_table[dist_idx]);
}

static void on_home(lv_event_t *e)
{
    int axis = (int)(intptr_t)lv_event_get_user_data(e);
    printer_home(axis);
    ui_toast(axis < 0 ? "全部轴归位" : "轴归位", THEME_COL_ACCENT);
}

static void update_pos(void)
{
    static const char axis_name[] = {'X', 'Y', 'Z'};
    char vbuf[16];
    for (int a = 0; a < 3; a++) {
        if (printer_homed(a)) {
            theme_fmt_float(vbuf, sizeof(vbuf), printer_pos(a), 1);
            lv_label_set_text_fmt(lbl_pos[a], "%c %s", axis_name[a], vbuf);
        } else {
            lv_label_set_text_fmt(lbl_pos[a], "%c ?", axis_name[a]);
        }
    }
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    /* 距离档 */
    static const char *dists[] = {"0.1", "1", "10", "50"};
    lv_obj_t *tg = toggle_group_create(scr, dists, 4, dist_idx, on_dist, NULL);
    lv_obj_set_size(tg, 304, 28);
    lv_obj_align(tg, LV_ALIGN_TOP_MID, 0, THEME_TITLEBAR_H + 4);

    /* 三轴步进：每行  [-] [轴 位置] [+] */
    static const char *minus[3] = {LV_SYMBOL_LEFT,  LV_SYMBOL_DOWN, LV_SYMBOL_DOWN};
    static const char *plus[3]  = {LV_SYMBOL_RIGHT, LV_SYMBOL_UP,   LV_SYMBOL_UP};
    for (int a = 0; a < 3; a++) {
        lv_obj_t *bm = theme_button(scr, minus[a], NULL, 0);
        lv_obj_set_size(bm, 76, 38);
        lv_obj_align(bm, LV_ALIGN_TOP_LEFT, 8, 66 + a * 44);
        lv_obj_add_event_cb(bm, on_jog, LV_EVENT_CLICKED, (void *)(intptr_t)(a * 2));

        lv_obj_t *card = theme_card(scr);
        lv_obj_set_size(card, 136, 38);
        lv_obj_align(card, LV_ALIGN_TOP_LEFT, 92, 66 + a * 44);
        lbl_pos[a] = theme_label(card, "?", THEME_FONT_M, THEME_COL_TEXT);
        lv_obj_center(lbl_pos[a]);

        lv_obj_t *bp = theme_button(scr, plus[a], NULL, 0);
        lv_obj_set_size(bp, 76, 38);
        lv_obj_align(bp, LV_ALIGN_TOP_LEFT, 236, 66 + a * 44);
        lv_obj_add_event_cb(bp, on_jog, LV_EVENT_CLICKED, (void *)(intptr_t)(a * 2 + 1));
    }

    /* 归位行（Y 起始 204，与 Z 行底 198 留 6px 间隙，不遮挡） */
    static const struct { const char *icon, *t; int axis; } homes[] = {
        {LV_SYMBOL_HOME, "XY", 0}, {LV_SYMBOL_HOME, "Z", 2}, {LV_SYMBOL_HOME, "全部", -1},
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = theme_button(scr, homes[i].icon, homes[i].t, i == 2);
        lv_obj_set_size(b, 96, 30);
        lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 8 + i * 104, -6);
        lv_obj_add_event_cb(b, on_home, LV_EVENT_CLICKED, (void *)(intptr_t)homes[i].axis);
    }
    /* 注：mock 的 Home XY 简化为归 X（演示用） */

    return scr;
}

panel_def_t panel_move_def = {
    .name = "move", .title = "移动",
    .create = create,
    .on_show = update_pos,
    .on_tick = update_pos,
};
