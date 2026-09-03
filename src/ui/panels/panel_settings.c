/*
 * 设置：网络 / Moonraker / 语言入口 + 主题/背光/关于
 */
#include "../theme.h"
#include "../lang.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"
#include "app_settings.h"

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

/* 语言行：点击在中/英间切换，存 klipperscreen.conf 并重建 UI */
static void toggle_lang(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_lang_t l = ui_lang_get() == UI_LANG_ZH ? UI_LANG_EN : UI_LANG_ZH;
    ui_lang_set(l);
    settings_save_language(l == UI_LANG_EN ? "en" : "zh");
    panel_mgr_reload();
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

    make_link_row(scr, "无线网络", THEME_TITLEBAR_H + 4, open_wifi);
    make_link_row(scr, "Moonraker 连接", THEME_TITLEBAR_H + 43, open_moonraker);

    /* 语言：行内显示当前语言，点击即切换 */
    lv_obj_t *row = make_row(scr, "语言",
                             ui_lang_get() == UI_LANG_ZH ? "中文" : "English",
                             THEME_TITLEBAR_H + 82);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, toggle_lang, LV_EVENT_CLICKED, NULL);

    make_row(scr, "主题", "Dark", THEME_TITLEBAR_H + 121);
    make_row(scr, "背光", "100%", THEME_TITLEBAR_H + 160);
    make_row(scr, "版本", "0.1.0-dev", THEME_TITLEBAR_H + 199);

    return scr;
}

panel_def_t panel_settings_def = {
    .name = "settings", .title = "设置",
    .create = create,
    .on_show = NULL,
    .on_tick = NULL,
};
