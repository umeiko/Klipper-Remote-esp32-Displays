#pragma once
/*
 * Klipper 控制面薄封装（对照 KlipperScreen ks_includes/MoonrakerApi.py + KlippyGcodes.py）。
 * 全部 fire-and-forget；desktop 端由 printer_mock 直接模拟，不经过这里。
 */
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool klipper_gcode_script(const char *script);

bool klipper_print_start(const char *filename);   /* server.files 路径，如 "3dbenchy.gcode" */
bool klipper_print_pause(void);
bool klipper_print_resume(void);
bool klipper_print_cancel(void);

bool klipper_emergency_stop(void);      /* printer.emergency_stop (M112 语义) */
bool klipper_restart(void);             /* printer.restart（host restart） */
bool klipper_firmware_restart(void);    /* printer.firmware_restart */

/* 预留：文件列表（panel_files 真实化时接回调），本期未使用 */
typedef void (*klipper_files_cb)(const char *json_result, void *ud);
bool klipper_files_list(klipper_files_cb cb, void *ud);

#ifdef __cplusplus
}
#endif
