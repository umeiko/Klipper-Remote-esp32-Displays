// SVG 图标 → LVGL A8 C 数组（白底透明通道，UI 侧用 image_recolor 着色）
// 用法：node tools/icongen/gen_icons.mjs
// 新增图标：把 SVG 放进 src/ui/assets/svg/，在下面 ICONS 里加一行，重跑本脚本。
import { Resvg } from '@resvg/resvg-js';
import { readFileSync, writeFileSync, mkdirSync } from 'fs';
import { execFileSync } from 'child_process';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const SVG_DIR = join(ROOT, 'src/ui/assets/svg');
const OUT_C = join(ROOT, 'src/ui/assets');
const OUT_PNG = join(ROOT, 'tmp/icongen');
const LVGLIMG = join(ROOT, 'third_party/lvgl/scripts/LVGLImage.py');
// LVGLImage.py 需要 pypng，装在 tools/icongen/.venv 里
const PY = join(ROOT, 'tools/icongen/.venv/Scripts/python.exe');

// [svg 名, 输出尺寸(px), C 变量名后缀]
const ICONS = [
  ['heater',         28, 'heater'],        // 主菜单"温度"、温度面板
  ['extruder',       16, 'nozzle_16'],     // 标题栏喷嘴图标
  ['extruder',       32, 'nozzle_32'],     // 温度面板喷嘴大卡
  ['bed',            16, 'bed_16'],        // 标题栏热床图标
  ['bed',            32, 'bed_32'],        // 温度面板热床大卡
  ['move',           28, 'move'],
  ['extrude',        28, 'extrude'],
  ['files',          28, 'files'],
  ['printer',        28, 'printer'],
  ['settings',       28, 'settings'],
  ['wifi_excellent', 18, 'wifi_4'],
  ['wifi_good',      18, 'wifi_3'],
  ['wifi_fair',      18, 'wifi_2'],
  ['wifi_weak',      18, 'wifi_1'],
  ['link_off',       16, 'link_off'],      // 主菜单状态卡：Moonraker 断连
  ['link',           16, 'link'],          // 主菜单状态卡：已连接
  ['alert_circle',   16, 'alert_circle'],  // 主菜单状态卡：Klipper 异常
];

mkdirSync(OUT_PNG, { recursive: true });

for (const [svg, size, name] of ICONS) {
  const r = new Resvg(readFileSync(join(SVG_DIR, `${svg}.svg`)), {
    fitTo: { mode: 'width', value: size },
    // 透明背景；图标本身是白色，A8 只取 alpha 通道
  });
  const png = join(OUT_PNG, `img_${name}.png`);
  writeFileSync(png, r.render().asPng());

  // 注意：LVGLImage.py 的 -o 是输出目录，文件名取输入 PNG 名
  execFileSync(PY, [LVGLIMG, '--ofmt', 'C', '--cf', 'A8', '-o', OUT_C, png],
               { cwd: ROOT, stdio: 'inherit' });

  // 生成的 include 条件块在 ESP-IDF(Kconfig 配置 LVGL) 下会落到 "lvgl/lvgl.h"，
  // 而 IDF 组件没有 lvgl/ 子目录 → 编译失败。统一改成直接 #include "lvgl.h"。
  const cFile = join(OUT_C, `img_${name}.c`);
  const src = readFileSync(cFile, 'utf8').replace(
    /#if defined\(LV_LVGL_H_INCLUDE_SIMPLE\)[\s\S]*?#endif/,
    '#include "lvgl.h"');
  writeFileSync(cFile, src);
  console.log(`img_${name}.c  (${size}px)`);
}
console.log('done');
