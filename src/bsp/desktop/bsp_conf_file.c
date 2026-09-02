/*
 * bsp_conf 实现：desktop（工作目录下的同名文件，仅供桌面端调试设置 UI）
 */
#include "../bsp_conf.h"

#include <stdio.h>

int bsp_conf_read(const char *name, char *buf, size_t len)
{
    FILE *f = fopen(name, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, len - 1, f);
    fclose(f);
    buf[n] = 0;
    return (int)n;
}

int bsp_conf_write(const char *name, const char *buf)
{
    FILE *f = fopen(name, "w");
    if (!f) return -1;
    fputs(buf, f);
    fclose(f);
    return 0;
}
