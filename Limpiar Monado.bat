@echo off
rem Monado: borra lo que es de ESTA PC (cache de shaders/pipelines, reportes de crash, logs).
rem Corre esto antes de mover o copiar la carpeta a otra PC. NO toca save\, xcde.ini ni el juego.
cd /d "%~dp0"
echo.
echo  Se borrara: cache\  crash\  y los .log de build\
echo  (Las partidas en save\ y los ajustes NO se tocan. En esta PC tendras que volver a pulsar "Precompile shaders".)
choice /m " Continuar"
if errorlevel 2 exit /b 0
if exist "cache" rmdir /s /q "cache"
if exist "crash" rmdir /s /q "crash"
del /q "build\*.log" 2>nul
echo.
echo  Listo.
pause
