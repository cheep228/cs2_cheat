@echo off
setlocal
set ROOT=%~dp0
cd /d "%ROOT%"

rem Same toolset lookup as build.bat.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="
set "VSPATHFILE=%TEMP%\cs2_overlay_vspath.txt"
rem No if-block here on purpose: the installer path contains "(x86)", and its
rem closing paren would end the block early.
if not exist "%VSWHERE%" goto :no_vswhere
"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSPATHFILE%" 2>nul
set /p VSPATH=<"%VSPATHFILE%"
del /q "%VSPATHFILE%" 2>nul
:no_vswhere
if defined VSPATH set "VCVARS=%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" ( echo [!] vcvars64 not found - install the C++ workload & exit /b 1 )

call "%VCVARS%" >nul
if errorlevel 1 ( echo [!] vcvars64 failed & exit /b 1 )

if not exist "build\test" mkdir "build\test"

echo [*] client.dll stub
cl /nologo /O2 /MT /LD /Fobuild\test\ /Fdbuild\test\ test\client_stub.c /link /OUT:build\test\client.dll >nul
if errorlevel 1 ( echo [!] stub failed & exit /b 1 )

echo [*] test host
cl /nologo /O2 /MT /EHsc /Fobuild\test\ /Fdbuild\test\ test\host.cpp /link /SUBSYSTEM:WINDOWS /OUT:build\test\host.exe d3d11.lib user32.lib >nul
if errorlevel 1 ( echo [!] host failed & exit /b 1 )

copy /y build\cs2_overlay.dll build\test\cs2_overlay.dll >nul
if errorlevel 1 ( echo [!] build cs2_overlay.dll first with build.bat & exit /b 1 )

echo [+] build\test\host.exe
