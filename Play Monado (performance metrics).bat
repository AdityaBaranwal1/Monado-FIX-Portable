@echo off
rem T260: native frame metrics. This enables profiling; compare runs using this same mode.
cd /d "%~dp0"
if not exist build mkdir build
set "XCDE_METRICS=build\native_metrics.csv"
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if not exist build\xcde_launcher.exe (
  echo Falta compilar Monado. Ejecuta Build Monado.bat.
  pause
  exit /b 1
)
start "" "build\xcde_launcher.exe"
