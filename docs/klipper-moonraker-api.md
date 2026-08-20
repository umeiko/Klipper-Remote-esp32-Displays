# Klipper / Moonraker API 参考文档

> 面向 ESP32 资源受限客户端的 Moonraker Web API 参考。
> 信息来源：[Moonraker 官方文档](https://moonraker.readthedocs.io/en/latest/external_api/introduction/)（[GitHub docs](https://github.com/Arksine/moonraker/tree/master/docs)）、[Klipper Status Reference](https://www.klipper3d.org/Status_Reference.html)、[Klipper API Server](https://www.klipper3d.org/API_Server.html)。

---

## 1. 网络服务拓扑

```
┌─────────┐  Unix Socket   ┌───────────┐   HTTP/WS (7125)   ┌────────────┐
│ klippy  │ ◀────────────▶ │ Moonraker │ ◀────────────────▶ │ 本 ESP32    │
│ (Klipper│  (Klipper API  │ (Tornado) │                    │ 客户端      │
│  进程)  │    Server)     │           │                    │            │
└─────────┘                └───────────┘                    └────────────┘
```

- ESP32 **只与 Moonraker 通信**，不直连 klippy。
- 默认端口：**7125**（HTTP/WebSocket），SSL 端口 7130。
- 端点总览：

| 端点 | 协议 | 用途 |
|---|---|---|
| `ws://host:7125/websocket` | WebSocket | **主通道**，JSON-RPC 2.0 长连接 |
| `ws://host:7125/klippysocket` | WebSocket | 桥接 Klipper API Server（一般用不到） |
| `POST /server/jsonrpc` | HTTP | JSON-RPC over HTTP（纯轮询方案可用） |
| `/printer/...` | HTTP REST | 打印机控制与对象查询（多数由 Klipper 注册、Moonraker 透传） |
| `/server/...` | HTTP REST | 服务器状态、文件、历史、数据库 |
| `/access/...` | HTTP REST | 鉴权 |
| `/machine/...` | HTTP REST | 系统管理、电源设备、更新 |
| `/api/...` | HTTP REST | OctoPrint 兼容层（不需要） |

- HTTP 成功响应统一包一层 `{"result": ...}`。
- Query string 类型提示：`?seconds:int=120&enabled:bool=true`（`:int`、`:float`、`:bool`、`:json`）。

### 1.1 鉴权

- **局域网通常免鉴权**：`moonraker.conf` 的 `trusted_clients`（CIDR）内的客户端无需任何凭据。ESP32 与打印机同网段时通常直接可信。
- 非可信客户端：
  - HTTP：`Authorization: Bearer <JWT>` 或 `X-Api-Key: <key>` 头。
  - `POST /access/login`：`{"username","password","source":"moonraker"}` → `{"token"(JWT, 1h), "refresh_token"(~90d)}`。
  - `POST /access/refresh_jwt`：刷新 JWT。
  - `GET /access/api_key` / `POST /access/api_key`：查询/重置全局 API key。
  - `GET /access/oneshot_token`：一次性 token（5 秒有效），拼到 query：`ws://host:7125/websocket?token=<base32>`（无法设 header 的场景，如 WS 握手、文件下载）。
  - WS 也可连接后用 `server.connection.identify` 传 `access_token`/`api_key`。
- `GET /access/info`：查 `login_required` 与当前连接是否 `trusted`。

---

## 2. WebSocket JSON-RPC 2.0

### 2.1 报文格式

请求：
```json
{"jsonrpc":"2.0","method":"printer.info","params":{"arg_one":1},"id":354}
```
成功响应：
```json
{"jsonrpc":"2.0","result":{...},"id":354}
```
错误响应：
```json
{"jsonrpc":"2.0","error":{"code":36000,"message":"..."},"id":354}
```
通知（服务器推送，无 `id`）：
```json
{"jsonrpc":"2.0","method":"notify_status_update","params":[{...}, 578243.578]}
```
> 注意：Moonraker 通知的 `params` 始终是**位置参数数组**（常为单元素包一个对象）。

### 2.2 推荐启动连接流程

1. 循环重试连接 `ws://host:7125/websocket`。
2. 连上后调用 `server.info`，检查 `klippy_state`：
   - `ready` → 继续；
   - `startup` → 2 秒后重查；
   - `error`/`shutdown` → 显示 `printer.info.state_message` 提示用户；
   - `disconnected` → klippy 未运行或未启用 API Server。
3. `ready` 后：调 `printer.objects.list` → 构建并发送 `printer.objects.subscribe`（响应即全量状态）→ 拉文件列表等。
4. 收到 `notify_klippy_disconnected` 后回到步骤 2 重新等待。

可选：连接后调一次 `server.connection.identify`（只能调一次）：
```json
{"jsonrpc":"2.0","method":"server.connection.identify","params":{
  "client_name":"KlipperRemoteESP32","version":"0.1.0","type":"display",
  "url":"https://..."},"id":1}
```
`type` 对显示屏类应用填 `"display"`。返回 `{"connection_id": ...}`。

---

## 3. 打印机对象查询与订阅

### 3.1 列出对象

```
GET /printer/objects/list          # JSON-RPC: printer.objects.list
→ {"result": {"objects": ["webhooks","toolhead","extruder","heater_bed",...]}}
```
klippy ready 后先调它，再决定订阅哪些对象/发现动态设备。

### 3.2 查询（一次性）

```json
{"jsonrpc":"2.0","method":"printer.objects.query","id":4654,
 "params":{"objects":{"gcode_move":null,"toolhead":["position"],
                      "extruder":["temperature","target"]}}}
```
- 值为 `null` = 全部字段；字符串数组 = 仅指定字段（**ESP32 应充分利用以减流量**）。
- HTTP 简写：`GET /printer/objects/query?gcode_move&toolhead&extruder=target,temperature`
- 响应：`{"result":{"eventtime":578243.578,"status":{...}}}`
- 请求不存在的对象/字段**不报错**，只是省略。

### 3.3 订阅（仅 WebSocket/Unix Socket）

```json
{"jsonrpc":"2.0","method":"printer.objects.subscribe","id":5434,
 "params":{"objects":{"webhooks":["state","state_message"],
   "print_stats":null,"virtual_sdcard":["progress","is_active"],
   "display_status":["progress","message"],
   "extruder":["temperature","target"],"heater_bed":["temperature","target"],
   "fan":["speed"]}}}
```
- 订阅响应**立即包含当前全量状态**（用于初始化本地模型，无需再 query）。
- 之后变化经 `notify_status_update` 推送，**只含增量字段**。
- 新订阅请求**整体覆盖**旧订阅；`"objects":{}` 取消订阅。
- 温度类字段推送频率约 1Hz 量级。

### 3.4 核心对象字段速查

| 对象 | 关键字段 | 含义 |
|---|---|---|
| `webhooks` | `state`, `state_message` | klippy 状态：ready/startup/error/shutdown |
| `toolhead` | `position` [X,Y,Z,E]、`homed_axes`、`max_velocity`、`max_accel`、`estimated_print_time`、`extruder` | 指令坐标与运动限制 |
| `gcode_move` | `gcode_position`、`speed_factor`、`extrude_factor`、`speed`、`absolute_coordinates`、`homing_origin` | G-code 层位置与倍率 |
| `extruder` / `extruder1`… | `temperature`、`target`、`power`(0–1)、`can_extrude`、`pressure_advance` | |
| `heater_bed` | `temperature`、`target`、`power`(0–1) | |
| `fan` | `speed`(0–1)、`rpm`（可为 null） | 部件冷却风扇 |
| `heater_fan <name>` | `speed`、`rpm` | |
| `print_stats` | `state`(standby/printing/paused/complete/error/cancelled)、`filename`、`total_duration`、`print_duration`、`filament_used`(mm)、`message`、`info.total_layer/current_layer` | **打印状态主数据源** |
| `virtual_sdcard` | `progress`(0–1)、`is_active`、`file_position`、`file_size`、`file_path` | 文件进度 |
| `display_status` | `progress`（M73，5s 超时回落）、`message`（M117） | |
| `idle_timeout` | `state`(Printing/Ready/Idle)、`printing_time` | 不能用于判断是否在打印文件 |
| `motion_report` | `live_position`、`live_velocity`、`live_extruder_velocity` | 实时估算位置 |
| `pause_resume` | `is_paused` | |
| `stepper_enable` | `steppers: {name: bool}` | 电机使能 |
| `temperature_sensor <name>` | `temperature`、`measured_min_temp`、`measured_max_temp` | |
| `temperature_fan <name>` | `temperature`、`target`、`speed` | |
| `filament_switch_sensor <name>` | `filament_detected`、`enabled` | |
| `output_pin <name>` | `value`（数字 0/1，PWM 0.0–1.0） | |
| `mcu` | `mcu_version`、`mcu_build_versions`、`mcu_constants` | |
| `z_tilt` / `quad_gantry_level` | `applied`(bool) | |
| `gcode_macro <name>` | `variable_*` | 宏变量 |

**⚠ 嵌入式慎用的大对象**：`configfile`（可能非常大，官方明确警告嵌入式内存风险）、`bed_mesh`（网格矩阵）、`exclude_object`。本项目 v1 不订阅。

坐标一律为 4 元素数组 `[X, Y, Z, E]`。

---

## 4. G-code

### 4.1 执行脚本

```
POST /printer/gcode/script       # JSON-RPC: printer.gcode.script
{"jsonrpc":"2.0","method":"printer.gcode.script","params":{"script":"G28"},"id":...}
```
- 多行用 `\n` 分隔；返回 `"ok"`。
- **温度/风扇等控制没有专用 API，全部拼 gcode**：
  - 喷嘴：`M104 S{temp}`（多挤出头 `M104 T{n} S{temp}`）
  - 热床：`M140 S{temp}`
  - 通用加热器：`SET_HEATER_TEMPERATURE heater="{name}" target={temp}`
  - 温度风扇：`SET_TEMPERATURE_FAN_TARGET temperature_fan="{name}" target={temp}`
  - 风扇：`M106 S{0-255}`；速度倍率：`M220 S{pct}`；流量：`M221 S{pct}`
- **警告**：经此端点发 `M112` 会排队而非立即停机；急停必须用 `/printer/emergency_stop`。

### 4.2 命令帮助

```
GET /printer/gcode/help          # JSON-RPC: printer.gcode.help
→ {"result": {"RESTART":"Reload config...","SET_HEATER_TEMPERATURE":"...",...}}
```
可用于动态判断打印机支持哪些扩展命令（如 PROBE_CALIBRATE、Z_ENDSTOP_CALIBRATE）。

---

## 5. 打印控制与重启

| HTTP | JSON-RPC method | 说明 |
|---|---|---|
| `POST /printer/print/start?filename=x.gcode` | `printer.print.start` `{"filename":...}` | 路径相对 gcodes root |
| `POST /printer/print/pause` | `printer.print.pause` | |
| `POST /printer/print/resume` | `printer.print.resume` | |
| `POST /printer/print/cancel` | `printer.print.cancel` | |
| `POST /printer/emergency_stop` | `printer.emergency_stop` | 立即停机（M112 立即版） |
| `POST /printer/restart` | `printer.restart` | klippy 软重启 |
| `POST /printer/firmware_restart` | `printer.firmware_restart` | 完全重启 klippy + MCU |
| `POST /server/restart` | `server.restart` | 重启 Moonraker |
| `POST /machine/shutdown` / `POST /machine/reboot` | `machine.shutdown` / `machine.reboot` | 主机关机/重启 |

`/printer/*` 控制类成功均返回字符串 `"ok"`。

---

## 6. 文件管理（/server/files）

Root 概念：默认 `gcodes`、`config`（可写）、`logs`/`config_examples`/`docs`（只读）。路径形如 `/server/files/gcodes/subdir/a.gcode`。

### 6.1 列表与元数据

```
GET /server/files/list?root=gcodes           # server.files.list
→ {"result":[{"path":"a.gcode","modified":...,"size":...,"permissions":"rw"},...]}

GET /server/files/metadata?filename=a.gcode  # server.files.metadata
→ {"result":{"size":...,"slicer":"PrusaSlicer","estimated_time":3600,
   "object_height":20.0,"layer_height":0.2,"filament_total":5200.0,
   "filament_weight_total":15.4,"first_layer_extr_temp":215,
   "first_layer_bed_temp":60,
   "thumbnails":[{"width":32,"height":32,"size":1234,"relative_path":".thumbs/a-32x32.png"},...],
   "job_id":"...","print_start_time":...}}
```
- 元数据字段可用性取决于切片器；解析失败会报错，不支持的切片器只返回 size/modified。
- **每次打印只需取一次 metadata**（ETA 计算等）。

### 6.2 缩略图

- 取 `thumbnails[]` 中**最小尺寸**；下载 URL：
  `GET /server/files/gcodes/{gcode所在目录}/{relative_path}`
  例：`GET /server/files/gcodes/.thumbs/a-32x32.png`
- 另有 `GET /server/files/thumbnails?filename=...`，返回的 `thumbnail_path` 相对 gcodes root。

### 6.3 目录与其他操作

| 操作 | HTTP | JSON-RPC |
|---|---|---|
| 目录信息 | `GET /server/files/directory?path=gcodes/sub&extended=true` | `server.files.get_directory` |
| 创建目录 | `POST /server/files/directory` `{"path":"gcodes/d"}` | `server.files.post_directory` |
| 删除目录 | `DELETE /server/files/directory?path=...&force=false` | `server.files.delete_directory` |
| 移动/重命名 | `POST /server/files/move` `{"source","dest"}` | `server.files.move_file` |
| 复制 | `POST /server/files/copy` `{"source","dest"}` | `server.files.copy_file` |
| 上传（仅 HTTP） | `POST /server/files/upload`（multipart，字段 `file`，可选 `root/path/checksum/print`） | — |
| 下载（仅 HTTP） | `GET /server/files/{root}/{filename}` | — |
| 删除文件 | `DELETE /server/files/{root}/{filename}` | `server.files.delete_file` `{"path":"{root}/{filename}"}` |
| 日志快捷下载 | `GET /server/files/klippy.log` / `moonraker.log` | — |

上传上限 `max_upload_size`（默认约 210MB）；`print=true` 上传后自动打印。

---

## 7. 打印历史与服务器通知

### 7.1 历史

```
GET /server/history/list?limit=50&start=0&order=desc    # server.history.list
→ {"result":{"count":N,"jobs":[{"job_id":"...","filename":"...","exists":true,
   "status":"completed","start_time":...,"end_time":...,"print_duration":...,
   "filament_used":...,"metadata":{...}},...]}}
```
- status 枚举：`in_progress/completed/cancelled/error/klippy_shutdown/klippy_disconnect/interrupted`
- 其他：`GET /server/history/totals`、`GET /server/history/job?uid=xxx`、`DELETE /server/history/job?uid=xxx`、`POST /server/history/reset_totals`。

### 7.2 通知（notify_*）

全部经 WebSocket 推送，`params` 为位置数组：

| method | params | 说明 |
|---|---|---|
| `notify_status_update` | `[{增量对象}, eventtime]` | **订阅更新（核心）** |
| `notify_gcode_response` | `["message"]` | 所有 gcode 输出广播（M117/M118/`!!`错误）。流量可能很大，ESP32 不需要 console 时收到即丢弃 |
| `notify_klippy_ready` / `notify_klippy_shutdown` / `notify_klippy_disconnected` | 无 | 收到 disconnected 需重走启动流程 |
| `notify_history_changed` | `[{action:added/finished, job:{...}}]` | |
| `notify_filelist_changed` | `[{action, item, source_item?}]` | action: create_file/delete_file/move_file… |
| `notify_proc_stat_update` | `[{moonraker_stats, cpu_temp, network, ...}]` | 周期性主机资源统计 |
| `notify_cpu_throttled` | `[{bits}]` | 主机欠压/降频 |
| `notify_update_response` / `notify_update_refreshed` | | 更新管理 |
| `notify_job_queue_changed` | | 打印队列 |
| `notify_active_spool_set` / `notify_spoolman_status_changed` | | Spoolman |
| `notify_announcement_update/dismissed/wake` | | 公告 |

---

## 8. 服务器状态 / 健康检查

### 8.1 `server.info`（ESP32 首要健康检查端点）

```
GET /server/info           # JSON-RPC: server.info
→ {"result":{
   "klippy_connected":true,
   "klippy_state":"ready",            // disconnected/startup/ready/error/shutdown
   "components":["database","file_manager","klippy_apis","history",...],
   "failed_components":[],
   "registered_directories":["config","gcodes"],
   "warnings":[],
   "websocket_count":2,
   "moonraker_version":"v0.7.1-...",
   "api_version":[1,4,0],"api_version_string":"1.4.0"}}
```

### 8.2 `printer.info`

```
GET /printer/info          # JSON-RPC: printer.info
→ {"result":{"state":"ready","state_message":"","hostname":"...",
   "software_version":"v0.12.0-...","config_file":"...","cpu_info":"..."}}
```
klippy error/shutdown 时用 `state_message` 显示原因。

### 8.3 klippy 未连接时的行为

- `/printer/*` 端点需要 klippy 已连接，多数还要求 `ready`；否则返回错误。
- `query` 返回空 `status` 对象也是 klippy 异常的信号。
- `server.info`、`/access/*`、多数 `/machine/*` 不依赖 klippy。

### 8.4 其他

- `GET /server/temperature_store?include_monitors=false`：每秒采样的温度/目标/功率 FIFO 历史（默认最多 1200 点），**适合初始化温度曲线**。
- `GET /server/gcode_store?count=100`：gcode 响应缓存（console 面板初始化用）。
- `GET /server/config`：完整 Moonraker 配置（大，慎用）。

---

## 9. 其他有用接口

### 9.1 电源设备（需 `[power <name>]` 配置）

```
GET  /machine/device_power/devices                 # machine.device_power.devices
→ {"devices":[{"device":"printer","status":"on","locked_while_printing":true,"type":"tplink_smartplug"}]}
GET  /machine/device_power/device?device=printer   # machine.device_power.get_device
POST /machine/device_power/device                  # machine.device_power.post_device
     {"device":"printer","action":"on"|"off"|"toggle"}
```
类型支持 gpio/tplink/tasmota/shelly/homeassistant/mqtt/http 等。

### 9.2 更新管理

`GET /machine/update/status`、`POST /machine/update/refresh`、`POST /machine/update/upgrade?name=xxx`、`POST /machine/update/full` 等。

### 9.3 系统管理

`GET /machine/system_info`、`GET /machine/proc_stats`（cpu_temp、throttled_state）、`POST /machine/services/restart|stop|start`、`GET /machine/peripherals/usb|serial|video|canbus`。

### 9.4 打印队列

`GET /server/job_queue/status`、`POST /server/job_queue/job?filenames=a.gcode,b.gcode`、`DELETE /server/job_queue/job?job_ids=...`、`POST /server/job_queue/pause|start`。

---

## 10. ESP32 客户端实践建议（流量/内存控制）

1. **纯轮询备选方案**（官方 tutorial 以 ESP32 为例）：不维持 WS，1~2 秒一次
   `GET /printer/objects/query?webhooks&virtual_sdcard&print_stats`。
   ETA 计算：`eta = metadata.estimated_time − progress × estimated_time`，或 `print_duration/progress − print_duration`（防除零）。本项目默认用 WebSocket，但保留此方案作为弱网降级。
2. **订阅一次合并**：一次 `subscribe` 请求包含全部所需对象与字段（覆盖式语义）；只订阅需要的字段。
3. **绝不订阅** `configfile`、`bed_mesh` 等大对象；配置信息按需一次性 query 指定字段。
4. **订阅响应即全量**，无需额外 query 初始化。
5. **缩略图只拉最小尺寸**；metadata 每次打印只取一次。
6. `notify_gcode_response` 打印期流量大：不做 console 时快速丢弃。
7. klippy 断连后停止对象订阅，改低频（数秒）轮询 `server.info` 直到 `klippy_state==ready`，再重建订阅。
8. WS 断线带**指数退避重连**（1s → 2s → 4s → 封顶 30s）。
9. 单帧 JSON 设上限（如 8KB），超限断开重连保护内存。

---

## 附录：v1 最小 API 调用清单

| 时机 | 调用 |
|---|---|
| 连接后 | `server.connection.identify`（type=display） |
| 握手 | `server.info` → `printer.info` → `printer.objects.list` |
| 初始化 | `printer.objects.subscribe`（最小字段集，响应即全量） |
| 温度曲线初始化 | `server.temperature_store` |
| 温度/风扇/点动 | `printer.gcode.script`（拼 gcode） |
| 打印控制 | `printer.print.start/pause/resume/cancel` |
| 急停/重启 | `printer.emergency_stop` / `printer.restart` / `printer.firmware_restart` |
| 文件列表 | `server.files.list` + `server.files.metadata`（懒加载） |
| 缩略图 | `GET /server/files/gcodes/...`（HTTP） |
| 运行中 | 收 `notify_status_update` 增量合并 |
