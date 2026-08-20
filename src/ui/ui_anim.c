#include "ui_anim.h"
#include "theme.h"

void ui_anim_to(void *var, lv_anim_exec_xcb_t cb, int32_t from, int32_t to,
                uint32_t dur, lv_anim_path_cb_t path)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, var);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, dur);
    lv_anim_set_path_cb(&a, path);
    lv_anim_set_exec_cb(&a, cb);
    lv_anim_start(&a);
}

void ui_anim_after(uint32_t delay_ms, lv_anim_completed_cb_t cb, void *var)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, var);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_duration(&a, 1);
    lv_anim_set_completed_cb(&a, cb);
    lv_anim_start(&a);
}

/* ---------- Toast ---------- */
static void toast_delete_cb(lv_anim_t *a)
{
    lv_obj_delete((lv_obj_t *)a->var);
}

static void toast_slide_out(lv_anim_t *a)
{
    lv_obj_t *toast = (lv_obj_t *)a->var;
    lv_anim_t t;
    lv_anim_init(&t);
    lv_anim_set_var(&t, toast);
    lv_anim_set_values(&t, lv_obj_get_y(toast), -48);
    lv_anim_set_duration(&t, UI_ANIM_NORMAL);
    lv_anim_set_path_cb(&t, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&t, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_completed_cb(&t, toast_delete_cb);
    lv_anim_start(&t);
}

void ui_toast(const char *text, uint32_t accent_hex)
{
    lv_obj_t *top = lv_layer_top();
    lv_obj_t *toast = theme_card(top);
    lv_obj_set_size(toast, 232, 36);
    lv_obj_set_style_bg_color(toast, theme_col(accent_hex), 0);
    lv_obj_set_style_radius(toast, 18, 0);

    lv_obj_t *lbl = lv_label_create(toast);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, THEME_FONT_S, 0);   /* 不设会落到 montserrat_14，中文变方框 */
    lv_obj_set_style_text_color(lbl, theme_col(0xFFFFFF), 0);
    lv_obj_center(lbl);

    lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, -48);
    /* 停在常驻标题栏下方，避免遮挡标题 */
    ui_anim_to(toast, (lv_anim_exec_xcb_t)lv_obj_set_y, -48, THEME_TITLEBAR_H + 6, 350, lv_anim_path_ease_out);
    ui_anim_after(2200, toast_slide_out, toast);
}

/* ---------- 转场 ---------- */
void ui_screen_push(lv_obj_t *scr)
{
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, UI_ANIM_NORMAL, 0, false);
}

void ui_screen_pop(lv_obj_t *scr)
{
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, UI_ANIM_NORMAL, 0, false);
}
