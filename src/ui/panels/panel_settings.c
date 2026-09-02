/*
 * 设置：网络入口 + 主题/背光/关于
 */
#include "../theme.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"

static lv_obj_t *make_row(lv_obj_t *parent, const char *key, const char *val, int y)
{
    lv_obj_t *row = theme_card(parent);
    lv_obj_set_size(row, 304, 38);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, y);

    lv_obj_t *k = theme_label(row, key, THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *v = theme_label(row, val, THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, -4, 0);
    return row;
}

static void open_wifi(lv_event_t *e)
{
    LV_UNUSED(e);
    panel_mgr_open("wifi");
}

static void open_moonraker(lv_event_t *e)
{
    LV_UNUSED(e);
    panel_mgr_open("moonraker");
}

static lv_obj_t *make_link_row(lv_obj_t *scr, const char *key, int y, lv_event_cb_t cb)
{
    lv_obj_t *row = make_row(scr, key, "", y);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *arrow = theme_label(row, LV_SYMBOL_RIGHT, THEME_FONT_ICON, THEME_COL_ACCENT);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -4, 0);
    return row;
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    make_link_row(scr, "无线网络", THEME_TITLEBAR_H + 6, open_wifi);
    make_link_row(scr, "Moonraker 连接", THEME_TITLEBAR_H + 50, open_moonraker);

    make_row(scr, "主题", "Dark", THEME_TITLEBAR_H + 94);
    make_row(scr, "背光", "100%", THEME_TITLEBAR_H + 138);
    make_row(scr, "版本", "0.1.0-dev", THEME_TITLEBAR_H + 182);

    return scr;
}

panel_def_t panel_settings_def = {
    .name = "settings", .title = "设置",
    .create = create,
    .on_show = NULL,
    .on_tick = NULL,
};
