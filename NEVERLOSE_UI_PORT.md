# Neverlose UI port

This project now builds the customized Dear ImGui 1.87 fork supplied in
`neverlose_v4.zip` instead of trying to approximate that UI on stock ImGui.

## Source mapping

| Supplied Neverlose implementation | Port destination / usage |
| --- | --- |
| `ImGui/imgui.cpp::RenderMainWindowDecorations` | built directly from `deps/imgui_nl/imgui.cpp` |
| `ImGui/imgui.cpp::MainBegin` | built directly; used by `src/ui/menu.cpp` |
| `ImGui/imgui.cpp::MenuChildEx/MenuChild` | built directly; `theme::BeginGroupBox` is only a layout adapter |
| `ImGui/imgui_widgets.cpp::TabButton` | built directly; sidebar uses it |
| `Checkbox` | built directly |
| `SliderScalar` | built directly through SliderFloat/SliderInt |
| `BeginCombo` | built directly |
| `Selectable` | built directly |
| `ButtonEx` | built directly |
| `RadioButton` | built directly |
| `ScrollbarEx` | built directly |
| `cheats/menu.cpp` palette | source blue-theme values copied verbatim into the per-frame color interpolation |
| `cheats/menu.cpp` geometry | 800x620 window, 180 sidebar, 50 header, 275 child width, 16 column gap, x=18 and body y=48 |
| `hooks/hooks.cpp` font slots | exact six slot order/sizes retained |

## Project-specific adaptation

The user project's existing Rage/Legit/Visuals/Misc/Skins/Configs logic was not
replaced. Those tab implementations call a thin `theme` adapter, but the visible
checkboxes/sliders/combos/buttons/children come from the supplied Neverlose
fork. Existing sub-tabs are placed in the 50px top header so the original
Neverlose child geometry remains intact.

## Missing assets

The source archive contained no `.ttf` or `.otf` files. When the original Museo
Sans Cyrillic and icon font are absent, Windows Segoe UI/Arial fallbacks are
loaded at the exact source pixel sizes. Icon glyphs are hidden until the actual
`neverlose-v2-icons.ttf` is present, avoiding literal A/B/C letters.

The preview in `docs/neverlose_ui_port_preview_fallback_font.png` was rendered
headlessly from the actual customized ImGui draw lists with fallback fonts. It
is a geometry/style sanity check, not a claim that the missing font assets are
pixel-identical.

## Blur

The source's blur code is DX9-specific. The project renders through D3D11, so
`src/ui/nl_blur.cpp` currently preserves the original call boundary as a no-op
compatibility shim. The menu's original translucent/background layers still
render; only the extra shader blur pass is absent.

## Build

`build.bat` and `CMakeLists.txt` now point to `deps/imgui_nl` and compile
`src/ui/nl_blur.cpp`. `build_test.bat` still creates a standalone D3D11 test
host; its test window is enlarged to fit the full 800x620 menu.
