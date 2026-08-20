# Klipper Remote ESP32 Displays

基于 ESP32 CYD 系列开发板的 Klipper 远程触屏显示器（对标 KlipperScreen）。多后端架构：所有界面/业务代码共享，每个后端（ESP32 各板型、desktop SDL2、未来的 Pico/STM32…）地位平等。

- 架构设计：[docs/architecture.md](docs/architecture.md)
- Klipper/Moonraker API 参考：[docs/klipper-moonraker-api.md](docs/klipper-moonraker-api.md)
- LVGL v9 API 防错笔记：[docs/lvgl-v9-api-notes.md](docs/lvgl-v9-api-notes.md)

## 技术栈

ESP-IDF v5.5.5 · LVGL v9.3 · 多后端（ESP32 各 CYD 板型 / desktop SDL2：Windows+Linux / 未来 Pico SDK、STM32…）

## 目录

```
src/                      # 全部源码
  ui/                     #   界面（所有后端共享）
  bsp/                    #   板级支持：功能→架构两级
    bsp.h                 #     BSP 接口契约
    esp32/                #     ESP32 家族各板（bsp_cyd_2432s028r.c 等）
    desktop/              #     桌面 SDL2 BSP
  ports/                  #   每个后端一个可构建工程（构建胶水 + 入口）
    esp32/                #     ESP-IDF 工程（idf.py 在此目录运行）
    desktop/              #     CMake 工程（SDL2，Win/Linux）
tools/                  # 构建脚本与便携工具链（非源码）
third_party/lvgl/       # LVGL v9.3（git clone，不入库）
docs/                   # 设计文档
```

新增后端 = `src/bsp/<架构>/` 加一个 BSP 实现 + `src/ports/<架构>/` 加一个工程壳；同一架构的新板型只需在 `src/bsp/<架构>/` 里加文件。

## 环境准备（Windows，全程不依赖 GitHub）

1. **ESP-IDF**：离线安装器安装 v5.5.5（本机位于 `C:\esp\v5.5.5\esp-idf`，工具链 `C:\Espressif\tools`）。
   注意：Git Bash 的 MSYS2 运行时会向子进程注入 `MSYSTEM`，导致 `idf.py` 静默空转——
   必须通过 `tools/idf.ps1` 包装器调用（内部用 EIM profile 激活并移除该变量）。
2. **便携 MSYS2**（desktop 后端的 MinGW 编译环境）：
   ```bash
   curl -L -o tools/dl/msys2-base.tar.xz https://mirrors.tuna.tsinghua.edu.cn/msys2/distrib/x86_64/msys2-base-x86_64-20260611.tar.xz
   mkdir -p tools/msys64 && tar -xf tools/dl/msys2-base.tar.xz -C tools/msys64 --strip-components=1
   tools/msys64/usr/bin/bash.exe -lc "echo ok"   # 首次运行完成初始化
   echo 'Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/msys/$arch'  > tools/msys64/etc/pacman.d/mirrorlist.msys
   echo 'Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/mingw/$repo' > tools/msys64/etc/pacman.d/mirrorlist.mingw
   tools/msys64/usr/bin/bash.exe -lc "pacman -Sy --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2"
   ```
3. **LVGL**：`git clone --depth 1 -b release/v9.3 https://gitee.com/mirrors/lvgl.git third_party/lvgl`

## 构建

```bash
# desktop 后端（产物 src/ports/desktop/build/klipper_remote_desktop.exe）
bash tools/build-desktop.sh
./src/ports/desktop/build/klipper_remote_desktop.exe            # 交互窗口
./src/ports/desktop/build/klipper_remote_desktop.exe 3000 x.bmp # 3 秒后截图退出

# ESP32 后端（CYD 2432S028R，工程目录 src/ports/esp32）
cd src/ports/esp32
powershell -NoProfile -ExecutionPolicy Bypass -File ../../../tools/idf.ps1 build
powershell -NoProfile -ExecutionPolicy Bypass -File ../../../tools/idf.ps1 -p COMx flash monitor
```

## 中文字体（改了 UI 文案后必跑）

界面里所有字符串字面量的非 ASCII 字符会被自动提取，生成 CJK 子集字体：

```bash
python tools/fontgen/gen_fonts.py        # 默认用 C:/Windows/Fonts/simhei.ttf，可 --font 换
```

新增中文后不重跑就会出现方框（□）。生成后需重新编译对应后端。

## WiFi 配置

设置 → 无线网络：扫描 → 选 AP → 输密码 → 连接（转圈）→ toast 反馈。
三端实现共用 `src/bsp/bsp_wifi.h` 轮询接口：

- **ESP32**：`src/bsp/esp32/bsp_wifi_esp32.c`（esp_wifi 事件驱动）
- **Windows**：`src/bsp/desktop/bsp_wifi_windows.c`（netsh wlan，自动适配中英文系统输出）
- **Linux**：`src/bsp/desktop/bsp_wifi_linux.c`（nmcli，需 NetworkManager）
