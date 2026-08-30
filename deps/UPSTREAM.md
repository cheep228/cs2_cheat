# Vendored dependencies

The active menu build is intentionally pinned to the customized ImGui tree that
came with the supplied Neverlose source. Do not replace it with FetchContent or
a clean upstream checkout: the UI modifications live inside ImGui itself.

## deps/imgui_nl — ACTIVE Neverlose ImGui fork

- Version string: `1.87 WIP` (`IMGUI_VERSION_NUM` 18604)
- Origin: `ImGui/` directory from the user-supplied `neverlose_v4.zip`
- Build path: used by both `CMakeLists.txt` and `build.bat`
- Backends: the matching Win32 + D3D11 backends in the same directory

The following original custom implementations are therefore built directly,
not recreated by `theme.cpp` wrappers:

- `ImGui::MainBegin` / `RenderMainWindowDecorations`
- `ImGui::MenuChild` / `MenuChildEx`
- `ImGui::TabButton`
- `ImGui::Checkbox`
- `ImGui::SliderScalar`
- `ImGui::BeginCombo`
- `ImGui::Selectable`
- `ImGui::ButtonEx` and the icon+label `Button` overload
- `ImGui::RadioButton`
- `ImGui::ScrollbarEx`
- the custom `ImGuiIO::GUIScale` and `AnimationSpeed` fields
- the global `Colors::*` palette used by those widgets

Local differences from the supplied tree are deliberately tiny:

1. `TabButton` wraps `Colors::TabFrame` in `ImColor(...)` in one ternary to
   avoid an ambiguous conversion on non-MSVC compilers. The resulting color is
   identical.
2. Window logo text is branded `4FIR` instead of `NEVERLOSE`.
3. Subscription text is branded `lifetime` instead of the hard-coded old date.
4. The original DX9 blur implementation is replaced at the call boundary by a
   D3D11 compatibility shim (`src/ui/nl_blur.cpp`). The supplied DX9 shader
   binaries cannot be used by a D3D11 context.

## deps/imgui — INACTIVE reference/previous attempt

This is the previously vendored ImGui 1.93 tree. It is intentionally left in
place so prior work/history is not destroyed, but it is no longer compiled by
the payload. The exact 1.87 customized fork above avoids reimplementing the
Neverlose widget bodies across the 1.87 -> 1.93 API gap.

## Fonts

The supplied source references these files but did not contain any TTF/OTF
assets:

- `museosanscyrl-500.ttf` — 16 px and 11 px
- `museosanscyrl-700.ttf` — 16 px
- `museosanscyrl-900.ttf` — 30 px
- `neverlose-v2-icons.ttf` — 18 px and 12 px

See `assets/fonts/README.txt`. The loader preserves the exact six font slots and
sizes, and uses Windows font fallbacks when those assets are absent.

## deps/minhook

Vendored MinHook remains unchanged by the UI port and is built locally.
