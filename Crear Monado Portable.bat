@echo off
rem Monado: crea una carpeta PORTATIL con todo adentro (exe, launcher, DLLs, juego, shaders, saves).
rem Se puede mover o copiar a otro disco / otra PC y se inicia con "Play Monado.bat" sin instalar nada.
rem Uso: doble clic, o:  "Crear Monado Portable.bat" "D:\Juegos\Monado"   (opcional: --no-saves, --no-game)
setlocal EnableExtensions
cd /d "%~dp0"
title Monado - carpeta portable
echo.
echo  Monado - crear carpeta portable
echo  ===============================

rem ---- 1. compilar si todavia no existe el build
if not exist "build\xcde.exe" (
    echo.
    echo  Todavia no hay build\xcde.exe: primero se compila ^(Build Monado.bat^).
    call "Build Monado.bat"
)
if not exist "build\xcde.exe" goto :fail
if not exist "build\xcde_launcher.exe" goto :fail

rem ---- 2. recompilar el launcher (rapido, incremental) para que use las DLL de su propia carpeta
set "BASH=%ProgramFiles%\Git\bin\bash.exe"
if not exist "%BASH%" set "BASH=%LocalAppData%\Programs\Git\bin\bash.exe"
if exist "%BASH%" if exist "C:\msys64\ucrt64\lib\libSDL3.dll.a" (
    echo  Actualizando el launcher...
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
    "%BASH%" -c "tools/launcher/build.sh" >nul 2>nul
)

rem ---- 3. carpeta de destino
set "DEST=%~1"
set "EXTRA="
if /i "%~1"=="--no-saves" set "DEST="
if /i "%~1"=="--no-game" set "DEST="
if not defined DEST (
    echo.
    echo  Elige donde crear la carpeta portable ^(se crea una subcarpeta "Monado" ahi^).
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.FolderBrowserDialog; $d.Description='Donde crear la carpeta portable de Monado'; if($d.ShowDialog() -eq 'OK'){$d.SelectedPath}"`) do set "DEST=%%F\Monado"
)
if not defined DEST goto :fail
if /i "%~1"=="--no-saves" set "EXTRA=--no-saves"
if /i "%~1"=="--no-game" set "EXTRA=--no-game"
if /i "%~2"=="--no-saves" set "EXTRA=%EXTRA% --no-saves"
if /i "%~2"=="--no-game" set "EXTRA=%EXTRA% --no-game"

rem ---- 4. empaquetar
set "PY=.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"
echo.
echo  Destino: %DEST%
"%PY%" -X utf8 tools\make_portable.py "%DEST%" %EXTRA%
if errorlevel 1 goto :fail
echo.
choice /m " Abrir la carpeta ahora"
if not errorlevel 2 start "" explorer "%DEST%"
endlocal
exit /b 0

:fail
echo.
echo  No se pudo crear la carpeta portable ^(revisa los mensajes de arriba^).
pause
endlocal
exit /b 1
