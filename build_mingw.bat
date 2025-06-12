@echo off
rem Native Windows build with MinGW.
rem
rem Requires, on PATH:
rem   - a MinGW compiler (e.g. MSYS2 ucrt64 or winlibs): g++, ld, windres
rem   - ninja
rem   - Internet on first configure (every dependency is fetched and built
rem     from source via CPM; SDL2/protobuf/absl take a few minutes once).

setlocal

where g++ >nul 2>nul || (
  echo error: g++ not found on PATH. Use an MSYS2 ucrt64 or winlibs shell.
  exit /b 1
)

where ninja >nul 2>nul || (
  echo error: ninja not found on PATH.
  exit /b 1
)

cmake --preset windows-mingw
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset windows-mingw
if errorlevel 1 exit /b %errorlevel%

powershell -NoProfile -Command "Compress-Archive -Force -Path '%~dp0build\windows-mingw\z2edit.exe' -DestinationPath '%~dp0z2edit-windows.zip'"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Build finished. Run .\build\windows-mingw\z2edit.exe