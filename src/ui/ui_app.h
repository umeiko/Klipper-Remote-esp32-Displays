#pragma once
/* 应用入口：装配标题栏 + 面板管理器 + 数据层 */
#ifdef __cplusplus
extern "C" {
#endif

void ui_app_create(void);
void ui_app_open(const char *panel);   /* 截图/调试用：直接打开某面板 */

#ifdef __cplusplus
}
#endif
