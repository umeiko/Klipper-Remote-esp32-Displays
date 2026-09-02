#pragma once
/*
 * 配置小文件读写（按文件名）：
 *   esp32   → /littlefs/<name>（LittleFS，bsp_init 时已挂载）
 *   desktop → ./<name>（工作目录，仅调试 UI 用）
 * 内容格式由上层（app_settings）决定，这里只管字节流。
 */
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 读整个文件到 buf（ NUL 结尾）。返回读取字节数；文件不存在/失败返回 <0 */
int bsp_conf_read(const char *name, char *buf, size_t len);

/* 整体覆盖写（buf 为 NUL 结尾字符串）。0 成功，<0 失败 */
int bsp_conf_write(const char *name, const char *buf);

#ifdef __cplusplus
}
#endif
