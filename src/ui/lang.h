#pragma once
/*
 * UI 双语（中/英）：中文串作为 key，运行时按当前语言查表替换。
 * 翻译钩子内置在 theme_label/theme_button/titlebar_set/ui_toast/confirm_open
 * 等助手里；直接调 lv_label_set_text 的个别位置用 TR() 包一层。
 * 偏好存 klipperscreen.conf（language=zh|en），ui_app_create 启动时加载。
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UI_LANG_ZH = 0, UI_LANG_EN } ui_lang_t;

void ui_lang_set(ui_lang_t l);
ui_lang_t ui_lang_get(void);

/* 查表翻译；无匹配（数字/文件名/纯 ASCII 等）原样返回 */
const char *ui_tr(const char *zh);
#define TR(zh) ui_tr(zh)

/* 从 klipperscreen.conf 读语言并应用（须在构建任何 UI 前调用） */
void ui_lang_load(void);

#ifdef __cplusplus
}
#endif
