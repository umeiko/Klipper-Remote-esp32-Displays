/*
 * bsp_conf 实现：ESP32（LittleFS，挂点 /littlefs，分区 storage）
 */
#include "../bsp_conf.h"

#include <stdio.h>
#include <string.h>

int bsp_conf_read(const char *name, char *buf, size_t len)
{
    char path[64];
    snprintf(path, sizeof(path), "/littlefs/%s", name);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, len - 1, f);
    fclose(f);
    buf[n] = 0;
    return (int)n;
}

int bsp_conf_write(const char *name, const char *buf)
{
    char path[64];
    snprintf(path, sizeof(path), "/littlefs/%s", name);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(buf, f);
    fclose(f);
    return 0;
}
