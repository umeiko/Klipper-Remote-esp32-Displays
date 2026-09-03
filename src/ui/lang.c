/*
 * UI 双语翻译表（zh → en）。key 为源码里的中文字面量，运行时精确匹配。
 * 新增 UI 中文串后在此追加一行即可；表未覆盖的串原样显示。
 */
#include "lang.h"
#include "app_settings.h"
#include <string.h>

static ui_lang_t cur = UI_LANG_ZH;

/* clang-format off */
static const struct { const char *zh; const char *en; } dict[] = {
    /* 面板标题 / 主菜单 */
    {"打印状态",        "Job Status"},
    {"温度控制",        "Temperature"},
    {"移动",            "Move"},
    {"挤出",            "Extrude"},
    {"打印文件",        "Print Files"},
    {"设置",            "Settings"},
    {"无线网络",        "WiFi"},
    {"Moonraker 连接",  "Moonraker"},
    {"文件详情",        "File Detail"},
    {"文件",            "Files"},
    {"温度",            "Temperature"},
    {"主菜单",          "Main Menu"},
    /* 打印机状态 */
    {"空闲",            "Standby"},
    {"打印中",          "Printing"},
    {"已暂停",          "Paused"},
    {"打印完成",        "Complete"},
    {"打印出错",        "Print Error"},
    {"已取消",          "Cancelled"},
    {"未连接",          "Offline"},
    {"Klipper 异常",    "Klipper Error"},
    /* 按钮 */
    {"暂停",            "Pause"},
    {"继续",            "Resume"},
    {"取消",            "Cancel"},
    {"急停",            "E-Stop"},
    {"确定",            "OK"},
    {"确认",            "Confirm"},
    {"删除",            "Delete"},
    {"确认删除?",       "Confirm?"},
    {"重启",            "Restart"},
    {"重启下位机",      "Restart MCU"},
    {"打印",            "Print"},
    {"重新扫描",        "Rescan"},
    {"保存并连接",      "Save & Connect"},
    {"全部冷却",        "Cooldown All"},
    {"冷却",            "Cooldown"},
    {"全部轴归位",      "Home All"},
    {"轴归位",          "Home"},
    {"全部",            "All"},
    {"装料",            "Load"},
    {"退料",            "Unload"},
    {"回抽",            "Retract"},
    /* 提示 / toast */
    {"开始打印",        "Print started"},
    {"已取消打印",      "Print cancelled"},
    {"已删除",          "Deleted"},
    {"已急停（M112）",  "E-Stop sent (M112)"},
    {"已发送重启指令",  "Restart command sent"},
    {"正在打印中，无法开始新任务", "Busy printing, cannot start"},
    {"没有可重启的文件", "No file to restart"},
    {"喷嘴温度过低，无法挤出",     "Nozzle too cold to extrude"},
    {"连接失败，请检查密码",       "Connection failed, check password"},
    {"已保存，正在连接",           "Saved, connecting"},
    {"保存失败",                  "Save failed"},
    {"请先填写主机地址",           "Enter host address first"},
    {"获取失败，请检查连接",       "Fetch failed, check connection"},
    /* 状态行 */
    {"加载中…",         "Loading…"},
    {"暂无 GCode 文件", "No GCode files"},
    {"未连接 Moonraker","Moonraker offline"},
    {"扫描中…",         "Scanning…"},
    {"未发现网络",      "No networks found"},
    {"扫描失败，点列表上方重试", "Scan failed, tap above to retry"},
    {"连接中…",         "Connecting…"},
    {"离线（自动重连中）", "Offline (reconnecting)"},
    {"未配置",          "Not configured"},
    {"未设置",          "Not set"},
    {"已连接",          "Connected"},
    {"已连接 %dms",     "Connected %dms"},
    {"已连接 %s",       "Connected %s"},
    {"正在连接 %s",     "Connecting %s"},
    {"连接到 %s",       "Connect to %s"},
    {"连接超时",        "Connection timeout"},
    {"挤出中…",         "Extruding…"},
    {"回抽中…",         "Retracting…"},
    {"喷嘴加热中",      "Nozzle heating"},
    {"热床加热中",      "Bed heating"},
    {"喷嘴已关闭",      "Nozzle off"},
    {"热床已关闭",      "Bed off"},
    /* 表单 / 设置项 */
    {"Moonraker 主机（IP 或域名）", "Host (IP or name)"},
    {"端口",            "Port"},
    {"API Key（可留空）", "API Key (optional)"},
    {"密码",            "Password"},
    {"加密",            "Secured"},
    {"开放",            "Open"},
    {"喷嘴目标温度",    "Nozzle target"},
    {"热床目标温度",    "Bed target"},
    {"主题",            "Theme"},
    {"背光",            "Backlight"},
    {"版本",            "Version"},
    {"语言",            "Language"},
    {"状态",            "Status"},
    /* 确认对话框 / 格式化串 */
    {"确认急停？\n打印机将立即停止所有运动和加热",
     "E-Stop?\nAll motion and heaters stop immediately"},
    {"确认重启下位机？\n（FIRMWARE_RESTART）",
     "Restart MCU?\n(FIRMWARE_RESTART)"},
    {"已用 %s\n剩余 %s", "Elapsed %s\nLeft %s"},
    {"喷嘴 %d°C（挤出需 ≥ %d°C）", "Nozzle %d°C (min %d°C)"},
};
/* clang-format on */

void ui_lang_set(ui_lang_t l) { cur = l; }
ui_lang_t ui_lang_get(void) { return cur; }

const char *ui_tr(const char *zh)
{
    if (cur == UI_LANG_ZH || !zh) return zh;
    for (unsigned i = 0; i < sizeof(dict) / sizeof(dict[0]); i++)
        if (strcmp(dict[i].zh, zh) == 0) return dict[i].en;
    return zh;
}

void ui_lang_load(void)
{
    char lang[8] = "zh";
    settings_load_language(lang, sizeof(lang));
    cur = (strcmp(lang, "en") == 0) ? UI_LANG_EN : UI_LANG_ZH;
}
