@echo off
rem Activate ESP-IDF v5.5.5 env (call from cmd).
rem Usage: cmd /c tools\idf-env.bat ^&^& idf.py build
rem NOTE: MSYSTEM must be cleared, otherwise idf.py silently does nothing under Git Bash.
set MSYSTEM=
set IDF_PATH=C:\esp\v5.5.5\esp-idf
set IDF_TOOLS_PATH=C:\Espressif\tools
set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.5.5\venv
call "%IDF_PATH%\export.bat" >nul
