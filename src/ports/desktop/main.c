/*
 * desktop 后端入口：bsp_init + 共享 UI + 主循环，与 ESP32 后端的 app_main.c 对称。
 * 用法:
 *   klipper_remote_desktop[.exe]                  交互窗口（2x 缩放）
 *   klipper_remote_desktop[.exe] <毫秒> <out.bmp> 运行指定毫秒后截图保存并退出
 */
#include "bsp.h"
#include "ui_app.h"
#include "mock_printer.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 把 RGB565 快照存成 24bit BMP（合成 layer_top：标题栏/toast 常驻顶层） */
static int save_bmp(const char *path)
{
    lv_obj_t *scr = lv_screen_active();
    lv_draw_buf_t *snap = lv_snapshot_take(scr, LV_COLOR_FORMAT_RGB565);
    if (!snap) {
        fprintf(stderr, "snapshot failed\n");
        return 1;
    }
    /* 顶层（标题栏/toast），带 alpha 通道 */
    lv_draw_buf_t *top = lv_snapshot_take(lv_layer_top(), LV_COLOR_FORMAT_ARGB8888);

    uint32_t w = snap->header.w, h = snap->header.h;
    uint32_t stride = snap->header.stride;

    uint32_t row_bytes = (w * 3 + 3) & ~3u;
    uint32_t img_size = row_bytes * h;
    uint32_t file_size = 54 + img_size;

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "open %s failed\n", path);
        lv_draw_buf_destroy(snap);
        if (top) lv_draw_buf_destroy(top);
        return 1;
    }

    uint8_t header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    memcpy(header + 2, &file_size, 4);
    uint32_t off = 54; memcpy(header + 10, &off, 4);
    uint32_t ih = 40; memcpy(header + 14, &ih, 4);
    memcpy(header + 18, &w, 4);
    memcpy(header + 22, &h, 4);
    uint16_t planes = 1; memcpy(header + 26, &planes, 2);
    uint16_t bpp = 24; memcpy(header + 28, &bpp, 2);
    memcpy(header + 34, &img_size, 4);
    fwrite(header, 1, 54, f);

    uint8_t *row = malloc(row_bytes);
    uint32_t top_stride = top ? top->header.stride : 0;
    for (int32_t y = (int32_t)h - 1; y >= 0; y--) {
        const uint8_t *src = snap->data + (uint32_t)y * stride;
        const uint8_t *tsrc = top ? top->data + (uint32_t)y * top_stride : NULL;
        for (uint32_t x = 0; x < w; x++) {
            uint16_t px = src[x * 2] | (src[x * 2 + 1] << 8);
            uint8_t r = (px >> 11) & 0x1F, g = (px >> 5) & 0x3F, b = px & 0x1F;
            uint8_t R = (r << 3) | (r >> 2), G = (g << 2) | (g >> 4), B = (b << 3) | (b >> 2);
            if (tsrc) {   /* 合成顶层（ARGB8888 预乘？否，普通 alpha 混合） */
                const uint8_t *tp = tsrc + x * 4;
                uint8_t a = tp[3];
                if (a) {
                    B = (tp[0] * a + B * (255 - a)) / 255;
                    G = (tp[1] * a + G * (255 - a)) / 255;
                    R = (tp[2] * a + R * (255 - a)) / 255;
                }
            }
            row[x * 3 + 0] = B;
            row[x * 3 + 1] = G;
            row[x * 3 + 2] = R;
        }
        fwrite(row, 1, row_bytes, f);
    }
    free(row);
    fclose(f);
    lv_draw_buf_destroy(snap);
    if (top) lv_draw_buf_destroy(top);
    printf("saved %s (%ux%u)\n", path, w, h);
    return 0;
}

int main(int argc, char **argv)
{
    bsp_init();
    ui_app_create();

    /* 截图模式: <毫秒> <out.bmp> [面板名] */
    int shot_at = 0;
    const char *shot_path = NULL;
    if (argc >= 3) {
        shot_at = atoi(argv[1]);
        shot_path = argv[2];
        if (argc >= 4)
            ui_app_open(argv[3]);   /* 直接打开指定面板再截图 */
        if (argc >= 5 && strcmp(argv[4], "printing") == 0) {
            mock_print_start("3dbenchy.gcode");   /* 演示打印中状态 */
            ui_app_open("job_status");
        }
    }

    uint32_t start = lv_tick_get();
    while (1) {
        lv_timer_handler();
        if (shot_path && (int)(lv_tick_elaps(start)) >= shot_at)
            return save_bmp(shot_path);
        SDL_Delay(5);
    }
}
