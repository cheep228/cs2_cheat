cmake_minimum_required(VERSION 3.20)
project(cs2_overlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "CS2 is x64 only. Configure with -A x64.")
endif()

# ---- MinHook (vendored, deps/minhook) --------------------------------
add_library(minhook STATIC
    deps/minhook/src/buffer.c
    deps/minhook/src/hook.c
    deps/minhook/src/trampoline.c
    deps/minhook/src/hde/hde64.c)
target_include_directories(minhook PUBLIC deps/minhook/include)

# ---- Dear ImGui (exact supplied Neverlose fork, 1.87 WIP) -------------
# This directory is byte-for-byte the customized ImGui core from the provided
# Neverlose source (apart from tiny compiler/branding compatibility edits).
add_library(imgui STATIC
    deps/imgui_nl/imgui.cpp
    deps/imgui_nl/imgui_draw.cpp
    deps/imgui_nl/imgui_tables.cpp
    deps/imgui_nl/imgui_widgets.cpp
    deps/imgui_nl/imgui_impl_win32.cpp
    deps/imgui_nl/imgui_impl_dx11.cpp)
target_include_directories(imgui PUBLIC deps/imgui_nl)
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler)

# ---- payload -----------------------------------------------------------
add_library(cs2_overlay SHARED
    src/dllmain.cpp
    src/core/hooks.cpp
    src/core/log.cpp
    src/core/settings.cpp
    src/core/safe_settings.cpp
    src/core/vmt.cpp
    src/core/game_hooks.cpp
    src/sdk/entity.cpp
    src/sdk/gameinfo.cpp
    src/sdk/schema.cpp
    src/sdk/sigscan.cpp
    src/sdk/modules.cpp
    src/sdk/interfaces.cpp
    src/sdk/usercmd.cpp
    src/sdk/resolve.cpp
    src/sdk/entitylist.cpp
    src/sdk/bones.cpp
    src/sdk/weapon.cpp
    src/features/esp.cpp
    src/features/skeleton.cpp
    src/features/chams.cpp
    src/features/aimbot.cpp
    src/features/autowall.cpp
    src/features/world.cpp
    src/features/movement.cpp
    src/features/triggerbot.cpp
    src/features/recoil.cpp
    src/features/ragebot.cpp
    src/features/rage/weapon_settings.cpp
    src/features/rage/hitbox_scanner.cpp
    src/features/rage/point_scanner.cpp
    src/features/rage/hitchance.cpp
    src/features/rage/damage.cpp
    src/features/rage/target_selector.cpp
    src/features/rage/fire.cpp
    src/core/config_io.cpp
    src/sdk/trace.cpp
    src/sdk/visibility.cpp
    src/ui/theme.cpp
    src/ui/nl_blur.cpp
    src/ui/menu.cpp
    src/ui/tab_legit.cpp
    src/ui/tab_visuals.cpp
    src/ui/tab_misc.cpp
    src/ui/tab_skins.cpp
    src/ui/tab_config.cpp)

target_include_directories(cs2_overlay PRIVATE src)
target_compile_definitions(cs2_overlay PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(cs2_overlay PRIVATE imgui minhook d3d11 dxgi)
set_target_properties(cs2_overlay PROPERTIES OUTPUT_NAME "cs2_overlay")
