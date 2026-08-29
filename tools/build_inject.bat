@echo off
setlocal
cd /d "%~dp0.."

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="
set "VSPATHFILE=%TEMP%\cs2_overlay_vspath.txt"
if not exist "%VSWHERE%" goto :no_vswhere
"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSPATHFILE%" 2>nul
set /p VSPATH=<"%VSPATHFILE%"
del /q "%VSPATHFILE%" 2>nul
:no_vswhere
if defined VSPATH set "VCVARS=%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" ( echo [!] vcvars64 not found & exit /b 1 )

call "%VCVARS%" >nul
if errorlevel 1 ( echo [!] vcvars64 failed & exit /b 1 )

if not exist "build" mkdir "build"
echo [*] compiling inject.exe
cl /nologo /O2 /MT /EHsc /W3 /Fobuild\ /Fdbuild\ tools\inject.cpp /link /OUT:build\inject.exe advapi32.lib
if errorlevel 1 ( echo [!] inject build failed & exit /b 1 )
echo [+] build\inject.exe
