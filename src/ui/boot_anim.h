#pragma once
/*
 * 「Umeko」霓虹描边开机动画（参考 .reff/boot_animation，TFT_eSPI 版移植为平台无关核心）。
 * 分镜：轮廓多段亮头同步描边 + 镜头 160%→88% 后拉 → 洗入黄→红竖向渐变（带镜面高光带）
 *       → 定格 → 整体渐暗 → 清屏。Logo 数据见 assets/boot_logo_path.h（脚本生成）。
 *
 * 平台只需提供两个回调：推一块 RGB565 像素到屏幕、毫秒延时。
 * 动画为阻塞式，约 2.5 秒；内部缓冲用 malloc 临时申请，播完即释放。
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 把 w*h 的 RGB565 像素块推到屏幕 (x,y)（像素序由平台按屏要求处理） */
typedef void (*boot_anim_push_t)(int x, int y, int w, int h, const uint16_t *px);
typedef void (*boot_anim_delay_t)(uint32_t ms);

void boot_anim_play(boot_anim_push_t push, boot_anim_delay_t dly);

#ifdef __cplusplus
}
#endif
