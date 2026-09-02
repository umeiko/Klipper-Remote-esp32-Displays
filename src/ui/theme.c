#include "theme.h"
#include <stdio.h>

void theme_fmt_float(char *buf, size_t n, float v, int decimals)
{
    int v10 = (int)(v * 10.0f + (v >= 0 ? 0.5f : -0.5f));   /* 四舍五入到 0.1 */
    if (decimals == 0)
        snprintf(buf, n, "%d", (v10 + (v10 >= 0 ? 5 : -5)) / 10);
    else
        snprintf(buf, n, "%d.%d", v10 / 10, v10 < 0 ? -(v10 % 10) : v10 % 10);
}

lv_color_t theme_col(uint32_t hex)
{
    return lv_color_hex(hex);
}

lv_obj_t *theme_card(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, theme_col(THEME_COL_SURFACE), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, THEME_RADIUS_CARD, 0);
    lv_obj_set_style_pad_all(obj, THEME_PAD, 0);
    return obj;
}

lv_obj_t *theme_button(lv_obj_t *parent, const char *icon, const char *text, int accent)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_style_bg_color(btn, theme_col(accent ? THEME_COL_ACCENT : THEME_COL_SURFACE2), 0);
    lv_obj_set_style_radius(btn, THEME_RADIUS_BTN, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    /* 按压反馈：背景半透明即可。
       不能用 transform_scale/opa —— 它们会强制 LVGL 把控件渲染进中间层缓冲，
       ESP32 堆紧张时该分配失败会让 lvgl 任务死循环（看门狗卡死 UI） */
    lv_obj_set_style_bg_opa(btn, LV_OPA_70, LV_STATE_PRESSED);

    if ((icon && icon[0]) || (text && text[0])) {
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(btn, 4, 0);
    }
    if (icon && icon[0]) {
        lv_obj_t *ic = lv_label_create(btn);
        lv_label_set_text(ic, icon);
        lv_obj_set_style_text_font(ic, THEME_FONT_ICON, 0);
        lv_obj_set_style_text_color(ic, theme_col(THEME_COL_TEXT), 0);
    }
    if (text && text[0]) {
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, text);
        lv_obj_set_style_text_font(lbl, THEME_FONT_S, 0);
        lv_obj_set_style_text_color(lbl, theme_col(THEME_COL_TEXT), 0);
    }
    return btn;
}

lv_obj_t *theme_menu_button(lv_obj_t *parent, const char *icon, const char *text)
{
    lv_obj_t *btn = theme_button(parent, NULL, NULL, 0);
    lv_obj_set_style_bg_color(btn, theme_col(THEME_COL_SURFACE), 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn, 4, 0);

    lv_obj_t *ic = lv_label_create(btn);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_font(ic, THEME_FONT_L, 0);
    lv_obj_set_style_text_color(ic, theme_col(THEME_COL_ACCENT), 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, THEME_FONT_S, 0);
    lv_obj_set_style_text_color(lbl, theme_col(THEME_COL_TEXT), 0);
    return btn;
}

lv_obj_t *theme_label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t col_hex)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, theme_col(col_hex), 0);
    return lbl;
}

lv_obj_t *theme_img(lv_obj_t *parent, const lv_image_dsc_t *src, uint32_t col_hex)
{
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, src);
    /* A8 只有 alpha 通道，靠 recolor 上色 */
    lv_obj_set_style_image_recolor(img, theme_col(col_hex), 0);
    lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
    return img;
}

lv_obj_t *theme_menu_button_img(lv_obj_t *parent, const lv_image_dsc_t *icon, const char *text)
{
    lv_obj_t *btn = theme_button(parent, NULL, NULL, 0);
    lv_obj_set_style_bg_color(btn, theme_col(THEME_COL_SURFACE), 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn, 4, 0);

    theme_img(btn, icon, THEME_COL_ACCENT);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, THEME_FONT_S, 0);
    lv_obj_set_style_text_color(lbl, theme_col(THEME_COL_TEXT), 0);
    return btn;
}
