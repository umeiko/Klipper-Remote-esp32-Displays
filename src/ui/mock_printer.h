#pragma once
/*
 * Mock 打印机数据层：里程碑 1 之前代替 PrinterModel/MoonrakerClient。
 * 接口设计对齐真实数据层，后续整体替换实现，面板代码不动。
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOCK_STATE_STANDBY = 0,
    MOCK_STATE_PRINTING,
    MOCK_STATE_PAUSED,
    MOCK_STATE_COMPLETE,
} mock_state_t;

/* 初始化并启动 1s 数据节拍（内部创建 LVGL timer，驱动 panel_mgr_tick） */
void mock_printer_init(void);

/* ---- 读 ---- */
mock_state_t mock_state(void);
float mock_temp_ext(void);      /* 当前喷嘴温度 */
float mock_temp_bed(void);
float mock_target_ext(void);    /* 目标温度 */
float mock_target_bed(void);
float mock_pos(int axis);       /* 0=X 1=Y 2=Z */
int  mock_homed(int axis);
int  mock_progress_permille(void);   /* 0~1000 千分比 */
const char *mock_filename(void);
uint32_t mock_print_elapsed_s(void);
uint32_t mock_print_eta_s(void);
float mock_flow_pct(void);      /* 打印流量 %（演示用） */

/* ---- 写（全部模拟真实 RPC 的效果） ---- */
void mock_set_target_ext(float t);
void mock_set_target_bed(float t);
void mock_jog(int axis, float dist);       /* 相对点动 */
void mock_home(int axis);                  /* -1 = 全部归位 */
void mock_extrude(float mm);               /* 正=挤出 负=回抽 */
void mock_print_start(const char *filename);
void mock_print_pause(void);
void mock_print_resume(void);
void mock_print_cancel(void);
void mock_emergency_stop(void);      /* 对应 M112：立即停止，加热目标清零 */
void mock_firmware_restart(void);    /* 对应 FIRMWARE_RESTART：重启下位机 */

#ifdef __cplusplus
}
#endif
