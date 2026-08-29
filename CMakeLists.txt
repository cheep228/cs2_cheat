cmake_minimum_required(VERSION 3.20)
project(cs2_overlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "CS2 is x64 only. Configure with -A x64.")
endif()

include(FetchContent)

# ---- MinHook -----------------------------------------------------------
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master)
FetchContent_MakeAvailable(minhook)

# ---- Dear ImGui --------------------------------------------------------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler)

# ---- payload -----------------------------------------------------------
add_library(cs2_overlay SHARED
    src/dllmain.cpp
    src/core/hooks.cpp
    src/core/log.cpp
    src/core/settings.cpp
    src/sdk/entity.cpp
    src/sdk/gameinfo.cpp
    src/sdk/schema.cpp
    src/sdk/sigscan.cpp
    src/sdk/resolve.cpp
    src/sdk/entitylist.cpp
    src/sdk/bones.cpp
    src/sdk/weapon.cpp
    src/features/esp.cpp
    src/features/skeleton.cpp
    src/features/chams.cpp
    src/features/aimbot.cpp
    src/features/autowall.cpp
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
    src/ui/skeet.cpp
    src/ui/menu.cpp
    src/ui/tab_rage.cpp
    src/ui/tab_legit.cpp
    src/ui/tab_visuals.cpp
    src/ui/tab_misc.cpp
    src/ui/tab_skins.cpp
    src/ui/tab_config.cpp)

target_include_directories(cs2_overlay PRIVATE src)
target_compile_definitions(cs2_overlay PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(cs2_overlay PRIVATE imgui minhook d3d11 dxgi)
set_target_properties(cs2_overlay PROPERTIES OUTPUT_NAME "cs2_overlay")
