/*
 * 设置：网络 / Moonraker / 语言 / 背光 / 自动息屏 + 主题/版本
 * 7 行超出 240 屏高，整页可上下滚动。
 */
#include "../theme.h"
#include "../lang.h"
#include "../panel_mgr.h"
#include "app_settings.h"
#include "bsp.h"
#include <stdio.h>

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

static void open_brightness(lv_event_t *e)
{
    LV_UNUSED(e);
    panel_mgr_open("brightness");
}

/* 语言：下拉选择；切换后存 klipperscreen.conf，背光 1s 渐暗到黑再重启
 * （热重建 UI 在事件回调里删屏幕会踩 LVGL 对象树，不稳定，故直接重启） */
static void on_lang_select(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    ui_lang_t want = lv_dropdown_get_selected(dd) == 1 ? UI_LANG_EN : UI_LANG_ZH;
    if (want == ui_lang_get()) return;
    settings_save_language(want == UI_LANG_EN ? "en" : "zh");
    lv_refr_now(NULL);      /* 先把选中态画出来 */
    bsp_fade_out(1000);     /* 当前亮度 1s 渐暗到纯黑 */
    bsp_restart();
}

/* 息屏选项（秒）；0 = 永不 */
static const uint32_t so_values[] = { 15, 30, 60, 300, 900, 1800, 3600, 0 };
static const char    *so_labels[] = { "15秒", "30秒", "1分钟", "5分钟", "15分钟", "30分钟", "1小时", "永不" };
#define SO_COUNT (sizeof(so_values) / sizeof(so_values[0]))

static void on_screen_off_select(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dd);
    if (sel >= SO_COUNT) return;
    settings_save_screen_off((int)so_values[sel]);
    bsp_set_screen_timeout(so_values[sel]);   /* 立即生效，无需重启 */
}

/* 带下拉的设置行（语言/自动息屏共用样式） */
static lv_obj_t *make_dropdown_row(lv_obj_t *scr, const char *key, const char *options,
                                   int y, int sel, lv_event_cb_t cb)
{
    lv_obj_t *row = theme_card(scr);
    lv_obj_set_size(row, 304, 38);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);   /* 行内下拉拖动不卷动整页 */

    lv_obj_t *k = theme_label(row, key, THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *dd = lv_dropdown_create(row);
    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_symbol(dd, NULL);   /* CJK 字库无 LV_SYMBOL_DOWN 字形，省得显示方框 */
    lv_obj_set_size(dd, 100, 30);
    lv_obj_align(dd, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_text_font(dd, THEME_FONT_S, 0);
    lv_obj_set_style_text_color(dd, theme_col(THEME_COL_TEXT), 0);
    lv_obj_set_style_bg_color(dd, theme_col(THEME_COL_SURFACE2), 0);
    lv_obj_set_style_border_width(dd, 0, 0);
    /* 下拉列表：深色底，选中项更深一档；限高 150（列表从下拉框向上展开，
       再高顶端会顶出屏幕上沿，顶部选项够不着） */
    lv_obj_t *list = lv_dropdown_get_list(dd);
    lv_obj_set_height(list, 150);
    lv_obj_set_style_max_height(list, 150, 0);
    lv_obj_set_style_text_font(list, THEME_FONT_S, 0);
    lv_obj_set_style_text_color(list, theme_col(THEME_COL_TEXT), 0);
    lv_obj_set_style_bg_color(list, theme_col(THEME_COL_SURFACE), 0);
    lv_obj_set_style_bg_color(list, theme_col(THEME_COL_SURFACE2), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED);
    lv_dropdown_set_selected(dd, sel);
    lv_obj_add_event_cb(dd, cb, LV_EVENT_VALUE_CHANGED, NULL);
    return row;
}

static lv_obj_t *make_link_row(lv_obj_t *scr, const char *key, const char *val, int y, lv_event_cb_t cb)
{
    lv_obj_t *row = theme_card(scr);
    lv_obj_set_size(row, 304, 38);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *k = theme_label(row, key, THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *arrow = theme_label(row, LV_SYMBOL_RIGHT, THEME_FONT_ICON, THEME_COL_ACCENT);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -4, 0);

    /* 值标签在箭头左侧 */
    lv_obj_t *v = theme_label(row, val, THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_align_to(v, arrow, LV_ALIGN_OUT_LEFT_MID, -4, 0);
    return row;
}

static lv_obj_t *create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);   /* 7 行超出 240 高，允许上下滚动 */

    make_link_row(scr, "无线网络", "", THEME_TITLEBAR_H + 4, open_wifi);
    make_link_row(scr, "Moonraker 连接", "", THEME_TITLEBAR_H + 43, open_moonraker);

    /* 语言：下拉选择中/英，切换后渐暗重启生效 */
    make_dropdown_row(scr, "语言", "中文\nEnglish", THEME_TITLEBAR_H + 82,
                      ui_lang_get() == UI_LANG_EN ? 1 : 0, on_lang_select);

    /* 背光：行内显示当前亮度，点击进滑杆调节 */
    char br[8];
    snprintf(br, sizeof(br), "%d%%", settings_load_brightness());
    make_link_row(scr, "背光", br, THEME_TITLEBAR_H + 121, open_brightness);

    /* 自动息屏：下拉选择超时（立即生效） */
    static char so_opts[96];   /* 按当前语言拼接选项 */
    int so_len = 0, so_sel = (int)SO_COUNT - 1;
    int cur = settings_load_screen_off();
    for (unsigned i = 0; i < SO_COUNT; i++) {
        so_len += snprintf(so_opts + so_len, sizeof(so_opts) - so_len, "%s%s",
                           i ? "\n" : "", TR(so_labels[i]));
        if ((uint32_t)cur == so_values[i]) so_sel = (int)i;
    }
    make_dropdown_row(scr, "自动息屏", so_opts, THEME_TITLEBAR_H + 160, so_sel,
                      on_screen_off_select);

    make_row(scr, "主题", "Dark", THEME_TITLEBAR_H + 199);
    make_row(scr, "版本", "0.1.0-dev", THEME_TITLEBAR_H + 238);

    return scr;
}

panel_def_t panel_settings_def = {
    .name = "settings", .title = "设置",
    .create = create,
    .on_show = NULL,
    .on_tick = NULL,
};
