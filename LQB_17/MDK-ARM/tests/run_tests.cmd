@echo off
cd /d "%~dp0.."
"D:\Downloads\mingw64\bin\gcc.exe" -std=c99 -Wall -Wextra -Werror -I..\BSP_Drivers\Inc tests\test_monitor.c ..\BSP_Drivers\Src\monitor.c ..\BSP_Drivers\Src\bsp_keys.c -o tests\test_monitor.exe
if errorlevel 1 exit /b 1
tests\test_monitor.exe
