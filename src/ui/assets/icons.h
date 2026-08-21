#pragma once
/*
 * KlipperScreen material-dark 图标的 LVGL A8 资源（tools/icongen/gen_icons.mjs 生成）
 * A8 = 仅 alpha 通道的白色图标，用 theme_img() 的 recolor 着色。
 * SVG 源文件在 svg/ 子目录（来自 KlipperScreen，GPL-3.0）。
 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

LV_IMAGE_DECLARE(img_heater);      /* 28px 火焰：温度 */
LV_IMAGE_DECLARE(img_nozzle_16);   /* 16px 喷嘴：标题栏 */
LV_IMAGE_DECLARE(img_nozzle_32);   /* 32px 喷嘴：温度面板 */
LV_IMAGE_DECLARE(img_bed_16);      /* 16px 热床：标题栏 */
LV_IMAGE_DECLARE(img_bed_32);      /* 32px 热床：温度面板 */
LV_IMAGE_DECLARE(img_move);        /* 28px 四向箭头 */
LV_IMAGE_DECLARE(img_extrude);     /* 28px 挤出 */
LV_IMAGE_DECLARE(img_files);       /* 28px 文件 */
LV_IMAGE_DECLARE(img_printer);     /* 28px 打印机 */
LV_IMAGE_DECLARE(img_settings);    /* 28px 齿轮 */
LV_IMAGE_DECLARE(img_wifi_4);      /* 18px 信号四档：强→弱 */
LV_IMAGE_DECLARE(img_wifi_3);
LV_IMAGE_DECLARE(img_wifi_2);
LV_IMAGE_DECLARE(img_wifi_1);

#ifdef __cplusplus
}
#endif
