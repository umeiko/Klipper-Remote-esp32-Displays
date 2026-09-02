/*
 * Moonraker 连接设置：主机/端口/API Key 编辑 + 连接状态 + 保存并连接。
 * 配置持久化到 moonraker.conf（app_settings → bsp_conf）。
 * 文本输入弹层复用 panel_wifi 密码弹层的 textarea + keyboard 模式。
 */
#include "../theme.h"
#include "../ui_anim.h"
#include "../panel_mgr.h"
#include "../widgets/keypad.h"
#include "app_settings.h"
#include "moonraker_client.h"
#include "printer.h"
#include <string.h>
#include <stdio.h>

static moonraker_conf_t cfg;        /* 工作副本，保存时才落盘 */
static lv_obj_t *lbl_host;
static lv_obj_t *lbl_port;
static lv_obj_t *lbl_key;
static lv_obj_t *lbl_status;

/* ---------- 文本输入弹层 ---------- */
static lv_obj_t *txt_overlay;
static lv_obj_t *ta;
static char  *edit_target;          /* 指向 cfg.host 或 cfg.api_key */
static size_t edit_cap;
static lv_obj_t **edit_label;       /* 完成后刷新的行标签 */
static int   edit_masked;

static void refresh_row(lv_obj_t *lbl, const char *val, int masked)
{
    if (!masked) {
        lv_label_set_text(lbl, val[0] ? val : "未设置");
        return;
    }
    /* API Key 掩码显示 */
    char m[24] = "未设置";
    if (val[0]) {
        size_t n = strlen(val);
        memset(m, '*', n > 12 ? 12 : n);
        m[n > 12 ? 12 : n] = 0;
    }
    lv_label_set_text(lbl, m);
}

static void txt_overlay_close(void)
{
    if (txt_overlay) { lv_obj_delete(txt_overlay); txt_overlay = NULL; }
}

static void on_txt_ready(lv_event_t *e)
{
    LV_UNUSED(e);
    const char *v = lv_textarea_get_text(ta);
    strncpy(edit_target, v, edit_cap - 1);
    edit_target[edit_cap - 1] = 0;
    refresh_row(*edit_label, edit_target, edit_masked);
    txt_overlay_close();
}

static void on_txt_cancel(lv_event_t *e)
{
    LV_UNUSED(e);
    txt_overlay_close();
}

static void open_text_dialog(const char *title, char *target, size_t cap,
                             lv_obj_t **row_label, int masked)
{
    edit_target = target;
    edit_cap = cap;
    edit_label = row_label;
    edit_masked = masked;

    txt_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(txt_overlay);
    lv_obj_set_size(txt_overlay, 320, 240);
    lv_obj_set_style_bg_color(txt_overlay, theme_col(THEME_COL_BG), 0);
    lv_obj_set_style_bg_opa(txt_overlay, LV_OPA_COVER, 0);

    lv_obj_t *lbl = theme_label(txt_overlay, title, THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);

    ta = lv_textarea_create(txt_overlay);
    lv_obj_set_style_text_font(ta, THEME_FONT_S, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, target);
    lv_obj_set_width(ta, 300);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 34);

    lv_obj_t *kb = lv_keyboard_create(txt_overlay);
    lv_obj_set_size(kb, 320, 130);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_event_cb(kb, on_txt_ready, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, on_txt_cancel, LV_EVENT_CANCEL, NULL);
}

/* ---------- 行点击 ---------- */
static void on_host_click(lv_event_t *e)
{
    LV_UNUSED(e);
    open_text_dialog("Moonraker 主机（IP 或域名）", cfg.host, sizeof(cfg.host), &lbl_host, 0);
}

static void on_key_click(lv_event_t *e)
{
    LV_UNUSED(e);
    open_text_dialog("API Key（可留空）", cfg.api_key, sizeof(cfg.api_key), &lbl_key, 1);
}

static void on_port_done(float value, int ok, void *ud)
{
    LV_UNUSED(ud);
    if (!ok) return;
    if (value < 1 || value > 65535) return;
    cfg.port = (uint16_t)value;
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned)cfg.port);
    lv_label_set_text(lbl_port, buf);
}

static void on_port_click(lv_event_t *e)
{
    LV_UNUSED(e);
    keypad_open("端口", cfg.port ? cfg.port : 7125, on_port_done, NULL);
}

static void on_save_click(lv_event_t *e)
{
    LV_UNUSED(e);
    if (!cfg.host[0]) {
        ui_toast("请先填写主机地址", THEME_COL_ERROR);
        return;
    }
    cfg.valid = true;
    if (settings_save_moonraker(&cfg)) {
        ui_toast("已保存，正在连接", THEME_COL_OK);
        moonraker_reload();
    } else {
        ui_toast("保存失败", THEME_COL_ERROR);
    }
}

/* ---------- 界面 ---------- */
static lv_obj_t *make_row(lv_obj_t *parent, const char *key, lv_obj_t **val_lbl, int y)
{
    lv_obj_t *row = theme_card(parent);
    lv_obj_set_size(row, 304, 38);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *k = theme_label(row, key, THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 2, 0);

    *val_lbl = theme_label(row, "", THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_set_width(*val_lbl, 200);
    lv_label_set_long_mode(*val_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(*val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(*val_lbl, LV_ALIGN_RIGHT_MID, -4, 0);
    return row;
}

static void update_rows(void)
{
    refresh_row(lbl_host, cfg.host, 0);
    refresh_row(lbl_key, cfg.api_key, 1);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned)(cfg.port ? cfg.port : 7125));
    lv_label_set_text(lbl_port, buf);
}

static void tick(void)
{
    const char *s;
    uint32_t col;
    char buf[40];
    switch (moonraker_state()) {
    case MOONRAKER_READY:
        /* 已连接时附应用层心跳延迟（5s 一跳，0=还没测到） */
        if (printer_rtt_ms() > 0)
            snprintf(buf, sizeof(buf), "已连接 %dms", printer_rtt_ms());
        else
            snprintf(buf, sizeof(buf), "已连接");
        s = buf; col = THEME_COL_OK;
        break;
    case MOONRAKER_CONNECTING: s = "连接中…";   col = THEME_COL_WARN;  break;
    default:                   s = cfg.valid ? "离线（自动重连中）" : "未配置";
                               col = cfg.valid ? THEME_COL_WARN : THEME_COL_TEXT_DIM; break;
    }
    lv_label_set_text(lbl_status, s);
    lv_obj_set_style_text_color(lbl_status, theme_col(col), 0);
}

static lv_obj_t *create(void)
{
    memset(&cfg, 0, sizeof(cfg));
    settings_load_moonraker(&cfg);
    if (!cfg.port) cfg.port = 7125;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_col(THEME_COL_BG), 0);

    int y = THEME_TITLEBAR_H + 6;
    lv_obj_t *r;
    r = make_row(scr, "主机", &lbl_host, y);
    lv_obj_add_event_cb(r, on_host_click, LV_EVENT_CLICKED, NULL);
    r = make_row(scr, "端口", &lbl_port, y + 44);
    lv_obj_add_event_cb(r, on_port_click, LV_EVENT_CLICKED, NULL);
    r = make_row(scr, "API Key", &lbl_key, y + 88);
    lv_obj_add_event_cb(r, on_key_click, LV_EVENT_CLICKED, NULL);

    /* 连接状态行（不可点） */
    lv_obj_t *srow = theme_card(scr);
    lv_obj_set_size(srow, 304, 38);
    lv_obj_align(srow, LV_ALIGN_TOP_MID, 0, y + 132);
    lv_obj_t *k = theme_label(srow, "状态", THEME_FONT_M, THEME_COL_TEXT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 2, 0);
    lbl_status = theme_label(srow, "", THEME_FONT_S, THEME_COL_TEXT_DIM);
    lv_obj_align(lbl_status, LV_ALIGN_RIGHT_MID, -4, 0);

    lv_obj_t *btn = theme_button(scr, LV_SYMBOL_SAVE, "保存并连接", 1);
    lv_obj_set_size(btn, 304, 36);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_add_event_cb(btn, on_save_click, LV_EVENT_CLICKED, NULL);

    update_rows();
    tick();
    return scr;
}

panel_def_t panel_moonraker_def = {
    .name = "moonraker", .title = "Moonraker 连接",
    .create = create,
    .on_show = NULL,
    .on_tick = tick,
};
