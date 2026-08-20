# Klipper Remote Display — 架构设计文档

> 项目代号：**Klipper-Remote-ESP32-Displays**
> 目标：基于 ESP32 CYD（Cheap Yellow Display）系列开发板，为 Klipper 3D 打印机提供远程触屏显示与控制能力，参考 KlipperScreen 的界面与交互设计。
> 状态：v0.1 初稿（设计阶段）

---

## 1. 项目概述与目标

### 1.1 目标

- 在 ESP32 CYD 系列开发板上运行一块"远程 KlipperScreen"：显示打印状态、温度、进度，支持点动、预热、挤出、宏、文件管理等核心操作。
- 通过 **Moonraker 的 WebSocket JSON-RPC API**（端口 7125）与打印机通信，不直连 klippy。
- **多后端**：同一套 UI/业务代码可在不同型号 CYD（不同屏幕尺寸/驱动/触摸芯片）以及 **Linux 模拟器（SDL2）** 上构建运行，便于开发与调试。
- **高级感**：引入非线性（缓动）动画——页面转场、数值补间、进度环、按压反馈等，超越 KlipperScreen 原作（原作基本没有过渡动画）。

### 1.2 非目标（v1 不做）

- 摄像头画面（MJPEG 对 ESP32 解码负担大，后期可选 JPEG 硬解）。
- Spoolman、update_manager 等扩展组件。
- OctoPrint 兼容 API。
- WiFi 配网面板（ESP32 自身的配网通过 ESP-IDF provisioning 或硬编码，不与 KlipperScreen 的 NetworkManager 面板对应）。

---

## 2. 需求与约束

### 2.1 硬件约束（ESP32 经典款）

| 资源 | 预算 | 说明 |
|---|---|---|
| RAM | ~320KB SRAM（部分型号带 8MB PSRAM） | LVGL 帧缓冲、JSON 解析、状态模型都吃内存 |
| Flash | 4MB~16MB | 固件 + 图片资源（LittleFS/FATFS 分区存图标字体） |
| CPU | 240MHz 双核 | UI 任务与网络任务分核运行 |
| 屏幕 | 240×320 / 320×480 / 480×320 等 | 不同 CYD 型号差异大，布局必须自适应 |
| 网络 | WiFi 2.4GHz | Moonraker 与打印机在局域网内，通常属于 `trusted_clients` 免鉴权 |

### 2.2 CYD 系列差异点

| 型号 | MCU | 屏幕 | 分辨率 | 触摸 | 备注 |
|---|---|---|---|---|---|
| ESP32-2432S024C/N/R | ESP32 | 2.4"/2.8" | 240×320 | XPT2046（电阻） | 最常见 CYD |
| ESP32-2432S028R | ESP32 | 2.8" | 240×320 | XPT2046 | 经典款 |
| ESP32-2432S032C | ESP32 | 3.2" | 240×320 | XPT2046 | |
| ESP32-3248S035R/C | ESP32 | 3.5" | 320×480 | XPT2046/GT911 | |
| ESP32-4827S043C | ESP32-S3 | 4.3" | 480×272 | GT911（电容） | 带 PSRAM |
| ESP32-8048S043C | ESP32-S3 | 4.3" | 800×480 | GT911 | 带 PSRAM |

> 差异收敛在 **BSP 层**（显示驱动、触摸驱动、背光、旋转方向），上层 UI 不感知。

### 2.3 软件约束

- 主开发框架：**ESP-IDF**（v5.x），组件化。
- UI 库：**LVGL v9**（官方 esp_lvgl_port 或直接移植；LVGL 也支持 SDL2 后端，天然适配 Linux 模拟器）。
- 网络：`esp_websocket_client`（ESP-IDF 内置）做 WebSocket；HTTP 仅用于拉缩略图等少量 REST 请求。
- JSON：**cJSON**（内存可控，ESP-IDF 内置）。
- Linux 后端：纯 CMake + SDL2 + 系统 libwebsockets（或自写轻量 ws 客户端），用于开发期快速迭代 UI。

---

## 3. 总体架构

### 3.1 分层视图

```
┌─────────────────────────────────────────────────────────────┐
│                        App 层                                │
│  Panels: main / job_status / move / temperature / extrude / │
│          files / macros / console / settings / ...          │
├─────────────────────────────────────────────────────────────┤
│                       UI 框架层 (ui/)                        │
│  PanelManager · Widgets · Theme · Animation · Toast/Dialog  │
│                    （全部基于 LVGL，不感知业务）               │
├─────────────────────────────────────────────────────────────┤
│                       Core 层 (core/)                        │
│  PrinterModel（状态模型+状态机） · EventBus · JobQueue        │
│  MoonrakerClient（JSON-RPC/WS 连接管理、订阅、重连）           │
├─────────────────────────────────────────────────────────────┤
│                    Platform 层 (platform/)                   │
│  net(WS/HTTP) · time · log · storage · task/queue 抽象       │
├─────────────────────────────────────────────────────────────┤
│                      BSP 层 (bsp/)                           │
│  display · touch · backlight · 电源键 · 蜂鸣器               │
│  ┌──────────────┬──────────────┬──────────────┐             │
│  │ bsp_2432s028r│ bsp_3248s035 │  bsp_linux   │  ……         │
│  └──────────────┴──────────────┴──────────────┘             │
└─────────────────────────────────────────────────────────────┘
```

**依赖方向严格单向**：App → UI → Core → Platform → BSP。下层永远不 #include 上层。

### 3.2 目录结构（已落地）

原则：**所有源码收在 `src/` 下；共享代码与架构代码彻底分离；架构专属代码按"功能 → 架构"两级归位**，一眼即可定位某段代码属于哪个后端。

```
├── src/                          # 全部源码（根目录不放代码）
│   ├── ui/                       # 界面（面板、widgets、主题、动画）—— 所有后端共享
│   ├── core/                     # PrinterModel、EventBus（规划中）
│   ├── net/                      # MoonrakerClient（规划中，依赖 platform 抽象）
│   ├── platform/                 # 平台抽象：plat_*.h 接口契约 + <arch>/ 实现
│   ├── bsp/                      # 板级支持：功能 → 架构两级
│   │   ├── bsp.h                 #   BSP 接口契约（纯头文件）
│   │   ├── esp32/                #   ESP32 家族各板：bsp_cyd_2432s028r.c、bsp_cyd_4827s043c.c …
│   │   ├── desktop/              #   桌面 SDL2 BSP（bsp_sdl2.c）
│   │   ├── pico/                 #   （未来）Pico SDK
│   │   └── stm32/                #   （未来）STM32
│   └── ports/                    # 每个后端一个可构建工程（只放构建胶水 + 入口 main，地位平等）
│       ├── esp32/                #   ESP-IDF 工程：CMakeLists(project) + sdkconfig.defaults + entry/app_main.c
│       ├── desktop/              #   CMake 工程：main.c + lv_conf.h（SDL2，Win/Linux 同构）
│       ├── pico/  stm32/ …       #   （未来）
├── third_party/lvgl/             # LVGL v9.3（git clone，不入库）
├── assets/                       # 图标（转 LVGL C 数组或存 LittleFS）、字体、主题配置
├── docs/                         # 设计文档
└── tools/                        # 构建脚本与便携工具链（非源码）
```

- **新增后端** = `src/bsp/<架构>/` 加 BSP 实现 + `src/platform/<架构>/` 加平台实现 + `src/ports/<架构>/` 加工程壳。
- **同一架构新增板型** = 只在 `src/bsp/<架构>/` 里加一个文件（如 ESP32-S3 的 5 寸 CYD），由 Kconfig 板型选项选择编译。
- `src/` 下的 `CMakeLists.txt`（idf_component_register）只被 ESP-IDF 消费，对其他构建系统是惰性文件。

---

## 4. 多后端与 BSP 设计

### 4.1 BSP 接口（头文件契约）

每块板/后端实现同一组接口（`src/bsp/bsp.h`）：

```c
void          bsp_init(void);            // 初始化显示+触摸+LVGL 节拍/任务
lv_display_t *bsp_get_display(void);
void          bsp_lvgl_lock(void);       // LVGL 互斥锁（单线程后端为空操作）
void          bsp_lvgl_unlock(void);
// 规划：bsp_backlight_set(percent)、bsp_backlight_off()
```

WiFi 也是 BSP 级抽象（`src/bsp/bsp_wifi.h`，全非阻塞轮询模型，UI 节拍里 poll）：

```c
void             bsp_wifi_scan_start(void);              // 启动扫描
int              bsp_wifi_scan_poll(bsp_wifi_ap_t *, int max);  // RUNNING/FAILED/条数
void             bsp_wifi_connect(const char *ssid, const char *pwd);
bsp_wifi_state_t bsp_wifi_status(void);                  // IDLE/CONNECTING/CONNECTED/FAILED
bool             bsp_wifi_connected(void);               // 真实连接状态（标题栏图标用；桌面端后台 5s 轮询缓存）
```

已实现：`esp32/bsp_wifi_esp32.c`（esp_wifi 事件驱动）、`desktop/bsp_wifi_windows.c`（netsh wlan，netsh 输出可能 UTF-8 也可能 GBK，逐行自动探测转码）、`desktop/bsp_wifi_linux.c`（nmcli）。

- **选择机制**：构建时通过 `CONFIG_BOARD_XXX`（Kconfig）或 `sdkconfig.defaults.<board>` 选中一个 BSP 实现目录参与编译；CMake 里 `if(CONFIG_BOARD_...) set(BSP_DIR ...)`。
- **显示驱动**：ILI9341/ST7789/ST7796 用 `esp_lcd`；Linux 后端用 LVGL SDL2 driver，窗口尺寸模拟目标分辨率。
- **触摸**：XPT2046 走 SPI 共享总线；GT911 走 I²C。均注册为 LVGL indev。
- **屏幕旋转**：240×320 竖屏原生，UI 以横屏 320×240 设计 → 在 BSP 里统一 `lv_disp_set_rotation()`，UI 层永远按横屏逻辑分辨率布局。

### 4.2 Platform 抽象（ESP32 / Linux 双实现）

UI 和 Core 代码不允许直接调用 `esp_*` / `xTask*` API，统一走：

| 接口 | ESP32 实现 | Linux 实现 |
|---|---|---|
| `plat_task_create` / `plat_queue_*` / `plat_mutex_*` | FreeRTOS | pthread + 简单阻塞队列 |
| `plat_time_ms` | `esp_timer_get_time()` | `clock_gettime` |
| `plat_log` | `ESP_LOGx` | `printf` |
| `plat_ws_client_*` | `esp_websocket_client` | libwebsockets |
| `plat_http_get(url, cb)`（仅缩略图等） | `esp_http_client` | libcurl |
| `plat_storage_*`（设置持久化） | NVS | 本地 INI 文件 |

> 这一层是"Linux 后端能跑起来"的关键：所有业务代码 100% 可编译到 Linux，UI 开发不必每次烧录。

---

## 5. 核心模块设计

### 5.1 MoonrakerClient（components/moonraker）

职责对标 KlipperScreen 的 `KlippyWebsocket` + `MoonrakerApi`，合并为一个模块：

- **协议**：WebSocket JSON-RPC 2.0，`ws://<host>:7125/websocket`（可带 `?token=` / `X-Api-Key`）。
- **请求-响应匹配**：内部维护 `id → callback` 路由表（环形表，容量 ~16），带超时的请求自动以错误回调释放。
- **通知分发**：无 `id` 的帧按 `method` 名（`notify_status_update` 等）抛到 EventBus。
- **连接状态机**（对 KlipperScreen 握手流程的精简，详见 API 文档第 2 节）：

```
DISCONNECTED ──connect──▶ CONNECTING ──on_open──▶ IDENTIFYING
     ▲                                                    │ server.connection.identify
     │                                                    ▼
     │                                              QUERY_SERVER_INFO (server.info)
     │                                                    │ klippy_state==ready?
     │                          非 ready：2~5s 退避重查      ▼
     │                                              QUERY_OBJECTS_LIST
     │                                                    ▼
     │                                              SUBSCRIBE (printer.objects.subscribe)
     │                                                    │ 响应即全量状态
     │                                                    ▼
     │                                              READY（正常收 notify_status_update）
     │                                                    │
     └──────── on_close / notify_klippy_disconnected ◀────┘（带指数退避重连）
```

- **订阅字段最小集**（v1）：
  `webhooks(state,state_message)`、`print_stats(state,filename,print_duration,total_duration,filament_used,info)`、`virtual_sdcard(progress,is_active,file_position,file_size)`、`display_status(progress,message)`、`gcode_move(speed_factor,extrude_factor,gcode_position,homing_origin,speed)`、`toolhead(position,homed_axes,estimated_print_time,extruder)`、`extruder(temperature,target,power)`、`heater_bed(temperature,target,power)`、`fan(speed)`、`idle_timeout(state)`、`pause_resume`。
  动态对象（extruder1+、temperature_sensor、temperature_fan、heater_fan、filament_sensor）在 `QUERY_OBJECTS_LIST` 后按列表拼接进订阅请求。
  **不订阅** `configfile`、`bed_mesh` 等大对象（内存警告，见 API 文档）。
- **控制面 API**：全部为薄封装：`gcode_script()`（温度、风扇、点动全部拼 gcode）、`print_start/pause/resume/cancel()`、`emergency_stop()`、`restart()`、`firmware_restart()`、`files.list/metadata()`、`device_power.*()`。

### 5.2 PrinterModel（components/printer_model）

对标 KlipperScreen 的 `printer.py`：

- **`data` 字典**（哈希表，键 = "object.field" 或嵌套 struct）：保存最近一次全量 + 增量合并后的打印机状态。`process_update()` 做增量 merge。
- **状态机** `evaluate_state()`：`webhooks.state==ready` 时叠加 `print_stats.state`（printing/paused/…），否则直接用 webhooks.state。状态变化 → EventBus 广播 `PRINTER_STATE_CHANGED`，App 层据此切面板（如 printing → job_status）。
- **设备枚举**：从 `printer.objects.list` + 订阅响应推导工具头、热床、风扇、传感器清单（`_` 开头的设备名隐藏，沿用 KlipperScreen 约定）。
- **温度历史环形缓冲**：每秒把当前温度 push 进 ring buffer（每设备 ~120 点 × int16，六七个设备 ≈ 2KB），驱动温度曲线 widget。
- **线程安全**：所有写只发生在 Core 线程（见 5.3），UI 读取通过快照/`lv_async_call` 拿到主线程拷贝——**不允许 UI 线程直接读 Core 线程正在写的结构体**。

### 5.3 EventBus 与线程模型

ESP32 上是双核 FreeRTOS，模型对齐 KlipperScreen 的"网络线程 → GLib.idle_add → UI 主循环"：

```
┌──────────────┐   JSON 帧    ┌─────────────────┐  事件入队   ┌──────────────┐
│  net_task    │ ───────────▶ │   core_task      │ ──────────▶ │  lvgl_task    │
│ (WS 收发,    │              │ (JSON 解析、     │             │ (面板渲染、   │
│  核 0)       │ ◀─────────── │  模型合并、      │ ◀────────── │  动画, 核 1)  │
└──────────────┘  用户指令    │  状态机、EventBus)│  UI 命令    └──────────────┘
                              └─────────────────┘
```

- **net_task**：裸收发，整帧文本通过 queue 递给 core_task，不做解析（降低 WS 回调内耗时）。
- **core_task**：唯一解析 JSON、唯一写 PrinterModel 的地方；分发事件。
- **lvgl_task**：持有 LVGL 锁，消费 `LV_EVENT_*` 类事件刷新 UI；用户触摸产生的指令（gcode 等）打包成"请求"丢回 core_task 发送。
- 所有跨线程传递走深拷贝 + 定长内存池，避免堆碎片。

### 5.4 PanelManager（components/ui_core）

对标 KlipperScreen 的 `show_panel/attach_panel/_cur_panels` 机制：

- **面板注册表**：每个面板是一个 `.c` 文件，暴露工厂函数与元信息（id、标题、图标）。集中注册表 `panel_registry.c` 维护。
- **面板池 + 导航栈**：面板实例创建后缓存复用（ESP32 内存紧，冷面板可配置为"销毁模式"）；导航栈支持 back。
- **生命周期钩子**：
  ```c
  typedef struct {
      lv_obj_t *(*create)(panel_ctx_t *ctx);   // 首次创建（或重建）
      void (*activate)(panel_ctx_t *ctx);      // 每次显示
      void (*deactivate)(panel_ctx_t *ctx);
      void (*on_event)(panel_ctx_t *ctx, const event_t *ev); // 状态更新
      void (*destroy)(panel_ctx_t *ctx);
  } panel_ops_t;
  ```
- **打开即全量回放**：activate 时把 PrinterModel 当前快照推给面板一次，保证新开面板立即有数据（KlipperScreen 同款行为）。
- **只有栈顶面板接收实时事件**，标题栏/Base 框架单独订阅温度等少量字段。

### 5.5 通用组件库（widgets，对照 KlipperScreen widgets/）

| 组件 | 对应 KlipperScreen | 说明 |
|---|---|---|
| `ui_keypad` | keypad.py | 数字键盘弹层（温度/距离输入） |
| `ui_dialog` | KlippyGtk.Dialog | 模态对话框，按钮列表 + 回调 |
| `ui_confirm` | `_confirm_send_action` | "确认 → 发 RPC"一行封装 |
| `ui_toast` | show_popup_message | 分级 toast（info/warn/error），自动消失 |
| `ui_tempgraph` | heatergraph.py | 温度曲线（LVGL chart + 环形缓冲） |
| `ui_progress_ring` | job_status 进度环 | LVGL arc 自绘环形进度 |
| `ui_toggle_group` | 距离档按钮组 | 单选 toggle 组（0.1/1/10/50） |
| `ui_titlebar` | base_panel | 常驻标题栏：连接状态、当前温度、时钟 |
| `ui_filelist` | gcodes 面板列表 | 文件列表项（缩略图+名称+大小） |

### 5.6 主题系统

- **单一基准尺寸派生全局布局**（借鉴 KlipperScreen 的 font_size 派生方案）：`theme.scale = f(分辨率)`，字号、图标尺寸、栏高、间距全部按 scale 缩放 → 一套布局适配 240×320 到 800×480。
- 主题 = 颜色表（结构体常量）+ 字号表 + 图标集路径。v1 内置 dark / light 两套，运行时切换 = 重设 LVGL style + 标记面板重建。
- 图标：KlipperScreen 的 SVG 图标集经 `tools/` 脚本转为 LVGL 单色 C 数组（或 LVGL 支持的 imgfont），图标名约定沿用 KlipperScreen（`extruder.svg` → `ICON_EXTRUDER`），降低对照成本。
- 中文字体：LVGL 字体子集化工具生成常用汉字子集（UI 文案约 500~800 字），避免整字库爆 Flash。

### 5.7 动画系统（本项目"高级感"核心）

LVGL 自带 `lv_anim` + 缓动路径（`lv_anim_path_ease_out / ease_in_out / overshoot / bounce`），在其上封装 `ui_anim` 模块：

| 场景 | 效果 | 实现 |
|---|---|---|
| 面板转场 | 滑动 + 淡入，不同方向表达层级（进入右滑、返回左滑） | `lv_scr_load_anim(..., LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false)` |
| 温度数值变化 | 旧值 → 新值 200ms 补间（ease-out），避免跳变 | `lv_anim` 作用于 label 的数值绑定 |
| 进度环 | progress 变化平滑追赶，300ms ease-in-out | arc 角度动画 |
| 打印开始/结束 | 进度环 0→当前值"回弹"（overshoot）入场 | `lv_anim_path_overshoot` |
| 按钮按压 | 缩放 0.95 回弹 + 涟漪色 | style transition + transform_zoom 动画 |
| Toast | 顶部滑入 + 停顿 + 滑出 | y 坐标动画链 + delay |
| 弹窗 | 背景淡入 + 内容 scale 0.9→1 | 双层动画 |
| 打印中状态徽标 | 呼吸/脉冲 | opacity 往返动画 |

约束：
- 动画时长统一 150~350ms，缓动曲线集中在 `ui_anim_easing.h` 定义，禁止散落魔法数字。
- 动画全部跑在 lvgl_task；低内存/低端板（240×320 电阻屏）可通过 `CONFIG_UI_ANIMATION_LEVEL=reduced` 降级为仅转场+数值补间。
- 电阻屏触摸精度差，按压反馈动画同时承担"确认触摸生效"的可感知性职责。

### 5.8 文件与缩略图

- 文件列表：`server.files.list`（gcodes root）+ 逐文件 `server.files.metadata`（懒加载，进入 files 面板才拉）。
- 缩略图：取 metadata `thumbnails[]` 中**最小尺寸**（通常 32×32/48×48），HTTP GET `/server/files/gcodes/<dir>/<relative_path>`。
- 240×320 机型：缩略图仅用于 job_status 显示；大分辨率机型才在文件列表显示缩略图。
- PNG 解码用 LVGL 自带 png decoder（libpng 对 ESP32 偏重，可用 `lvgl/src/libs/lodepng`）。

---

## 6. 面板规划（对照 KlipperScreen，按优先级分期）

### v1（MVP）

| 面板 | 对应 KlipperScreen | 核心功能 |
|---|---|---|
| splash / 连接页 | splash_screen.py | 连接进度、klippy 未就绪时的 restart/firmware_restart |
| main_menu | main_menu.py | 设备温度列表 + 温度曲线 + 导航入口 |
| job_status | job_status.py（简化） | 进度环、文件名、剩余时间、暂停/恢复/取消、微调入口 |
| temperature | temperature.py | 预热预设、目标温度调节（keypad） |
| move | move.py | 点动 + 距离档 + 归位 |
| extrude | extrude.py | 挤出入/退料 |
| files | gcodes.py（简化） | 文件列表、打印确认、删除 |
| settings | settings.py（简化） | 背光、主题、打印机地址、关于 |

### v2

fine_tune（打印中微调：速度/流量/zoffset 滑条）、gcode_macros、console（gcode 终端 + notify_gcode_response）、fan、led、zcalibrate、power（device_power 插座）、history。

### v3（候选）

bed_mesh（数据量大，需谨慎）、exclude_object、多打印机切换（printer_select）、锁屏/屏保。

---

## 7. 端到端数据流（以"打印进度刷新"为例）

```
Moonraker ──WS──▶ net_task ──帧──▶ core_task: JSON 解析
                                        │ notify_status_update
                                        ▼
                              PrinterModel.process_update（增量合并）
                                        │ 状态未变：不广播 STATE_CHANGED
                                        ▼
                              EventBus: EV_STATUS_UPDATE（携带增量字段集）
                                        ▼
                              lvgl_task: job_status.on_event()
                                        ├─ progress 变化 → 进度环补间动画
                                        ├─ ETA 重算 → label 更新
                                        └─ 温度字段 → titlebar + tempgraph ring buffer
```

"用户按下预热 PLA"反向链路：

```
lvgl_task: 按钮回调 → ui_confirm("预热到 210/60?") → 确认
        → core_task: gcode_script("M104 S210\nM140 S60") → WS 发送
        → 按钮转 busy spinner → 收到 "ok" 响应 → 恢复 + toast 提示
```

---

## 8. 构建与配置

- **构建系统**：ESP-IDF 标准 CMake；Linux 后端提供顶层 `CMakeLists.host.txt`（或 `idf.py` 之外的 `cmake -B build-host`），通过 `PLATFORM=linux` 切换 platform/ 实现与 bsp_linux。
- **板级选择**：`idf.py -DSDKCONFIG_DEFAULTS=sdkconfig.defaults.2432s028r ...` 或 `export BOARD=2432s028r` 由顶层 CMake 拼接。
- **Kconfig 关键项**：`CONFIG_BOARD_*`、`CONFIG_MOONRAKER_HOST/PORT`（默认值，运行时可改）、`CONFIG_UI_ANIMATION_LEVEL`、`CONFIG_UI_LANGUAGE`。
- **配置持久化**：打印机地址、API key、主题、背光等存 NVS（Linux 端存 INI），设置面板读写。
- **资源**：图标/字体编译期转 C 数组放 rodata；大资源（中文字库）放 LittleFS 分区，OTA 时不覆盖。

---

## 9. 内存与性能预算（ESP32 经典款，240×320）

| 项 | 预算 |
|---|---|
| LVGL draw buffer | 2 × 320×40 × 2B = 50KB（双缓冲行缓冲） |
| LVGL 堆（widgets/styles/anim） | ~60KB |
| PrinterModel + 事件池 | ~16KB |
| JSON 解析工作缓冲（单帧上限 8KB） | 8KB |
| net_task / core_task / lvgl_task 栈 | 4KB + 8KB + 12KB |
| 温度环形缓冲 | ~2KB |
| 缩略图解码缓冲（48×48 RGB565） | ~5KB |

- `notify_gcode_response` 流量可能很大：v1 默认不注册处理（console 面板打开时才临时订阅处理路径），收到即丢弃。
- 订阅更新约 1Hz，对 240MHz 双核毫无压力；瓶颈在刷屏 SPI 带宽（全屏 320×240×16bit ≈ 150KB，动画期间只刷脏区）。
- 带 PSRAM 的 S3 机型可把 draw buffer 放大到整屏 1/4，动画更流畅。

### 9.1 双核渲染与刷屏流水线（动画性能方案）

ESP32 经典款为 Xtensa 双核 240MHz，无 GPU/PPA，动画性能靠以下三板斧榨取：

1. **LVGL v9 多 draw unit 并行渲染**：`LV_USE_OS = LV_OS_FREERTOS` + `LV_DRAW_SW_DRAW_UNIT_CNT = 2`，渲染管线把屏幕按水平条带切分给两个线程并行绘制，CPU 渲染吞吐约 1.6~1.8×（渐变/圆弧/混合等软件逐像素操作受益最大）。
2. **核分配**：lvgl_task 与两个 draw unit 线程 pin 到核 1（APP_CPU）；net_task、core_task、触摸采样 pin 到核 0（PRO_CPU），网络抖动不影响动画帧时钟。FreeRTOS tick 配 1000Hz。
3. **DMA 异步 flush 流水线**：esp_lcd SPI 传输在回调中即刻释放 draw buffer，LVGL 渲染下一块脏区时上一块仍在 DMA 传输，渲染与传输重叠。ILI9341 按 40MHz 稳定时钟设计（全屏一帧约 30ms，全屏转场上限 ~30fps）。

UI 配合原则：动画尽量作用于小脏区（数值补间、进度环、toast、弹窗），全屏转场控制在 250ms 内快速完成。

预期帧率：小脏区动画稳定 30fps+；全屏滑动转场 ~30fps（SPI 极限）；S3 大分辨率机型靠 PSRAM 大缓冲 + 更高 SPI 时钟持平，且 S3 的 SIMD 指令在 v9 中有专门优化路径。

---

## 10. 开发路线图

1. **里程碑 0 — 地基**：platform 抽象 + bsp_linux（SDL2 跑通 LVGL demo）+ bsp_2432s028r（真机点亮）。
2. **里程碑 1 — 通信**：MoonrakerClient + PrinterModel，Linux 模拟器上连通真实打印机，cli 打印状态。
3. **里程碑 2 — UI 骨架**：PanelManager + theme + titlebar + toast/dialog + 动画模块；main_menu 静态版。
4. **里程碑 3 — MVP**：main_menu / job_status / temperature / move / files 五面板可用，真机联调。
5. **里程碑 4 — 完善**：动画打磨、extrude/settings、错误处理与断线重连 UX、v2 面板。
6. **里程碑 5 — 扩展板型**：3248s035、4827s043 等 BSP 适配（验证 BSP 抽象的正确性）。

---

## 附录 A：KlipperScreen 架构对照速查

| 本项目 | KlipperScreen | 说明 |
|---|---|---|
| MoonrakerClient | KlippyWebsocket + MoonrakerApi | 全 WebSocket JSON-RPC |
| PrinterModel | printer.py Printer | data 字典 + evaluate_state 状态机 |
| EventBus | NotificationHandler + GLib.idle_add | notify_* 路由 |
| PanelManager | screen.py panels/_cur_panels | 面板池+导航栈+三钩子+全量回放 |
| widgets | ks_includes/widgets/ | keypad/dialog/graph 等 |
| theme | styles/ + KlippyGtk | 基准尺寸派生布局 |
| ui_anim | （原作几乎没有） | 本项目增量优势 |
| bsp/platform | （无，直接 GTK/系统） | 多后端的关键抽象 |
