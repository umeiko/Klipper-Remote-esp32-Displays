@echo off
rem Klipper Remote ESP32 Displays - Windows flash script
rem Usage: flash.bat [COMx]
setlocal
cd /d "%~dp0"

set PORT=%~1
if "%PORT%"=="" set /p PORT=Enter COM port (e.g. COM6): 

if not exist esptool.exe (
    echo esptool.exe not found next to this script!
    pause
    exit /b 1
)

echo Flashing to %PORT% ...
esptool.exe --chip esp32 -b 460800 --before default_reset --after hard_reset ^
  write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m ^
  0x1000 bootloader.bin 0x8000 partition-table.bin 0x10000 klipper_remote_display.bin

echo.
echo Done. Press RESET or replug USB. First boot takes ~3s (boot animation).
pause
