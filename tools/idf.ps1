# ESP-IDF v5.5.5 构建环境包装器
# 用法: powershell -NoProfile -ExecutionPolicy Bypass -File tools/idf.ps1 <idf.py 参数...>
#   例: powershell -File tools/idf.ps1 build
# 说明: 环境由 EIM 安装器自带的 profile 激活；Git Bash 的 MSYS2 运行时会向所有子进程
#       强制注入 MSYSTEM（会导致 idf.py 静默空转），必须显式移除。

. 'C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1' | Out-Null
Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue

idf.py @args
exit $LASTEXITCODE
