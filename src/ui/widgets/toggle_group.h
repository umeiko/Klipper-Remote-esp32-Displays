#pragma once
/* 单选切换按钮组（距离档/挤出量等） */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*toggle_group_cb_t)(int index, void *user_data);

lv_obj_t *toggle_group_create(lv_obj_t *parent, const char **items, int count,
                              int selected, toggle_group_cb_t cb, void *user_data);

int toggle_group_get(lv_obj_t *group);

#ifdef __cplusplus
}
#endif
