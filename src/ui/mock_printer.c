#include "mock_printer.h"
#include "panel_mgr.h"
#include "lvgl.h"
#include <stdlib.h>

static struct {
    mock_state_t state;
    float ext, bed, ext_t, bed_t;
    float pos[3];
    int homed[3];
    float e_pos;
    int progress;          /* 千分比 */
    const char *filename;
    uint32_t tick;         /* 秒计数 */
    uint32_t print_start_tick;
    float flow;
} P = {
    .state = MOCK_STATE_STANDBY,
    .ext = 24.5f, .bed = 23.8f, .ext_t = 0, .bed_t = 0,
    .pos = {0, 0, 0}, .homed = {0, 0, 0},
    .progress = 0, .filename = "", .flow = 100,
};

mock_state_t mock_state(void) { return P.state; }
float mock_temp_ext(void)  { return P.ext; }
float mock_temp_bed(void)  { return P.bed; }
float mock_target_ext(void){ return P.ext_t; }
float mock_target_bed(void){ return P.bed_t; }
float mock_pos(int axis)   { return P.pos[axis]; }
int mock_homed(int axis)   { return P.homed[axis]; }
int mock_progress_permille(void) { return P.progress; }
const char *mock_filename(void)  { return P.filename; }
float mock_flow_pct(void)  { return P.flow; }

uint32_t mock_print_elapsed_s(void)
{
    return P.state == MOCK_STATE_PRINTING || P.state == MOCK_STATE_PAUSED
           ? P.tick - P.print_start_tick : 0;
}

uint32_t mock_print_eta_s(void)
{
    if (P.state != MOCK_STATE_PRINTING || P.progress < 5) return 0;
    uint32_t el = mock_print_elapsed_s();
    return el * (1000 - P.progress) / P.progress;
}

void mock_set_target_ext(float t) { P.ext_t = t; }
void mock_set_target_bed(float t) { P.bed_t = t; }

void mock_jog(int axis, float dist)
{
    if (axis < 0 || axis > 2) return;
    P.pos[axis] += dist;
    if (P.pos[axis] < 0) P.pos[axis] = 0;
    if (P.pos[axis] > 235) P.pos[axis] = 235;
}

void mock_home(int axis)
{
    if (axis < 0) { P.homed[0] = P.homed[1] = P.homed[2] = 1; P.pos[0] = P.pos[1] = P.pos[2] = 0; }
    else { P.homed[axis] = 1; P.pos[axis] = 0; }
}

void mock_extrude(float mm) { P.e_pos += mm; }

void mock_print_start(const char *filename)
{
    P.filename = filename;
    P.progress = 0;
    P.print_start_tick = P.tick;
    P.state = MOCK_STATE_PRINTING;
}

void mock_print_pause(void)  { if (P.state == MOCK_STATE_PRINTING) P.state = MOCK_STATE_PAUSED; }
void mock_print_resume(void) { if (P.state == MOCK_STATE_PAUSED)  P.state = MOCK_STATE_PRINTING; }
void mock_print_cancel(void) { P.state = MOCK_STATE_STANDBY; P.progress = 0; P.filename = ""; }

void mock_emergency_stop(void)
{
    /* M112：立刻停一切运动与加热 */
    P.state = MOCK_STATE_STANDBY;
    P.progress = 0;
    P.filename = "";
    P.ext_t = 0;
    P.bed_t = 0;
}

void mock_firmware_restart(void)
{
    /* FIRMWARE_RESTART：下位机重启期间加热目标清零 */
    P.ext_t = 0;
    P.bed_t = 0;
}

/* 每秒：温度向目标漂移 + 打印进度推进 + 广播节拍 */
static void tick_1s(lv_timer_t *tm)
{
    LV_UNUSED(tm);
    P.tick++;

    float rate = (P.ext_t > P.ext) ? 3.0f : 0.4f;   /* 升温快、降温慢 */
    P.ext += (P.ext_t - P.ext) * 0.18f + ((float)(rand() % 10) - 5) * 0.02f;
    P.bed += (P.bed_t - P.bed) * 0.12f + ((float)(rand() % 10) - 5) * 0.015f;
    LV_UNUSED(rate);

    if (P.state == MOCK_STATE_PRINTING && P.progress < 1000) {
        P.progress += 2 + rand() % 3;               /* ~5~8 分钟打完一个 mock 件 */
        if (P.progress >= 1000) { P.progress = 1000; P.state = MOCK_STATE_COMPLETE; }
    }

    panel_mgr_tick();
}

void mock_printer_init(void)
{
    lv_timer_create(tick_1s, 1000, NULL);
}
