@echo off
rem Microsoft Visual Studio build (MSVC).
rem
rem Requires:
rem   - Visual Studio with the "Desktop development with C++" workload
rem     (provides cl.exe, and VS's bundled cmake + ninja).
rem   - Internet on first configure: every third-party dependency is fetched
rem     and built from source via CPM (compiling protobuf/absl takes a while).
rem     Clones are cached under %USERPROFILE%\.cpm and reused.

setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
)
if "%VSROOT%"=="" set "VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" (
  echo error: MSVC toolset not found under %VSROOT%
  echo Install the "Desktop development with C++" VS workload.
  exit /b 1
)

call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

cmake --preset windows-msvc-static -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset windows-msvc-static
if errorlevel 1 exit /b %errorlevel%

powershell -NoProfile -Command "Compress-Archive -Force -Path '%~dp0build\windows-msvc\z2edit.exe' -DestinationPath '%~dp0z2edit-windows.zip'"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Build finished. Run .\build\windows-msvc\z2edit.exe
