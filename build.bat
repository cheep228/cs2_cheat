@echo off
setlocal
set ROOT=%~dp0
cd /d "%ROOT%"

rem The toolset is located with vswhere so any edition (Community, Insiders,
rem Build Tools) works; the old hard-coded path stays as the fallback.
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

if not exist "build\obj" mkdir "build\obj"
del /q build\obj\*.obj 2>nul

set COMMON=/nologo /c /O2 /MT /EHa /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DNOMINMAX /Fobuild\obj\

echo [*] compiling MinHook
cl %COMMON% /Ideps\minhook\include /Ideps\minhook\src deps\minhook\src\buffer.c deps\minhook\src\hook.c deps\minhook\src\trampoline.c deps\minhook\src\hde\hde64.c
if errorlevel 1 ( echo [!] MinHook failed & exit /b 1 )

echo [*] compiling ImGui
cl %COMMON% /std:c++20 /Ideps\imgui_nl deps\imgui_nl\imgui.cpp deps\imgui_nl\imgui_draw.cpp deps\imgui_nl\imgui_tables.cpp deps\imgui_nl\imgui_widgets.cpp deps\imgui_nl\imgui_impl_dx11.cpp deps\imgui_nl\imgui_impl_win32.cpp
if errorlevel 1 ( echo [!] ImGui failed & exit /b 1 )

echo [*] compiling payload
cl %COMMON% /std:c++20 /W3 /Isrc /Ideps\imgui_nl /Ideps\minhook\include src\dllmain.cpp src\core\hooks.cpp src\core\log.cpp src\core\settings.cpp src\core\safe_settings.cpp src\core\vmt.cpp src\core\game_hooks.cpp src\core\config_io.cpp src\sdk\gameinfo.cpp src\sdk\schema.cpp src\sdk\sigscan.cpp src\sdk\modules.cpp src\sdk\interfaces.cpp src\sdk\usercmd.cpp src\sdk\resolve.cpp src\sdk\entitylist.cpp src\sdk\entity.cpp src\sdk\bones.cpp src\sdk\weapon.cpp src\sdk\trace.cpp src\sdk\visibility.cpp src\features\esp.cpp src\features\skeleton.cpp src\features\chams.cpp src\features\aimbot.cpp src\features\autowall.cpp src\features\world.cpp src\features\movement.cpp src\features\triggerbot.cpp src\features\recoil.cpp src\features\ragebot.cpp src\features\rage\weapon_settings.cpp src\features\rage\hitbox_scanner.cpp src\features\rage\point_scanner.cpp src\features\rage\hitchance.cpp src\features\rage\damage.cpp src\features\rage\target_selector.cpp src\features\rage\fire.cpp src\ui\theme.cpp src\ui\nl_blur.cpp src\ui\menu.cpp src\ui\tab_legit.cpp src\ui\tab_visuals.cpp src\ui\tab_misc.cpp src\ui\tab_skins.cpp src\ui\tab_config.cpp
if errorlevel 1 ( echo [!] payload failed & exit /b 1 )

echo [*] linking
link /nologo /DLL /OUT:build\cs2_overlay.dll build\obj\*.obj d3d11.lib dxgi.lib d3dcompiler.lib user32.lib gdi32.lib dwmapi.lib
if errorlevel 1 ( echo [!] link failed & exit /b 1 )

echo [+] build\cs2_overlay.dll
