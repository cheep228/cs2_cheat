#pragma once
#include <cstdint>

#include "imgui.h"

// Skeet / old-Gamesense visual layer.
//
// Everything here is presentation only: no feature reads or writes game state
// from this file. The widgets keep ImGui's item logic (ids, hover, active,
// keyboard nav, SliderBehavior) because that part is correct and hard to
// reproduce, but every pixel is drawn by hand through ImDrawList so nothing
// carries the default ImGui look.
//
// Tab content lives in ui/tab_*.cpp - this file must stay free of it.
namespace skeet {

// --------------------------------------------------------------- palette --
namespace col {

constexpr ImU32 kWindowBg   = IM_COL32(0x10, 0x10, 0x10, 0xFF);
constexpr ImU32 kChildBg    = IM_COL32(0x15, 0x15, 0x15, 0xFF);
constexpr ImU32 kControlBg  = IM_COL32(0x0B, 0x0B, 0x0B, 0xFF);
constexpr ImU32 kBorder     = IM_COL32(0x33, 0x33, 0x33, 0xFF);
constexpr ImU32 kBorderSoft = IM_COL32(0x20, 0x20, 0x20, 0xFF);
constexpr ImU32 kText       = IM_COL32(0xD6, 0xD6, 0xD6, 0xFF);
constexpr ImU32 kTextDim    = IM_COL32(0x77, 0x77, 0x77, 0xFF);
constexpr ImU32 kAccent     = IM_COL32(0x00, 0xC8, 0xA0, 0xFF);
constexpr ImU32 kAccentHot  = IM_COL32(0x2C, 0xE8, 0xC2, 0xFF);
constexpr ImU32 kBad        = IM_COL32(0xD0, 0x50, 0x50, 0xFF);
constexpr ImU32 kGood       = IM_COL32(0x60, 0xC0, 0x70, 0xFF);

} // namespace col

// --------------------------------------------------------------- metrics --
namespace metric {

constexpr float kCheckBox   = 9.f;    // side of the checkbox square
constexpr float kSliderBar  = 7.f;    // slider track height
constexpr float kControlH   = 15.f;   // combo / keybind box height
constexpr float kRowGap     = 3.f;    // vertical gap between controls
constexpr float kGroupPadX  = 7.f;
constexpr float kGroupPadY  = 8.f;
constexpr float kTabRowH    = 22.f;
constexpr float kSubTabRowH = 17.f;

} // namespace metric

// Square, dense ImGui style. Called once after the context exists.
void ApplyStyle();

// Draws the nested window chrome (outer hairline, inset second border, title
// strip) for the current window. Call right after Begin().
void WindowChrome(const char* title);

// --------------------------------------------------------------- layout ---

// Width of one groupbox when `columns` of them share the content region.
float ColumnWidth(int columns);

// A bordered block whose caption sits inside the top border line. `size.y` of
// 0 means "measure the content", which costs one frame of settling.
void BeginGroupBox(const char* label, const ImVec2& size);
void EndGroupBox();

// Thin full-width rule used inside groupboxes.
void Separator();

// Text helpers that respect the palette.
void Text(const char* fmt, ...);
void TextDim(const char* fmt, ...);
void TextColored(ImU32 color, const char* fmt, ...);

// ----------------------------------------------------------------- tabs ---

// Horizontal main tabs (RAGE / LEGIT / ...). Returns true on the frame the tab
// is picked. `index` identifies this tab inside `current`.
bool Tab(const char* label, int index, int* current);

// Sub-tab row under the main tabs, same contract.
bool SubTab(const char* label, int index, int* current);

// Wrap a row of Tab()/SubTab() calls so the underline spans the full width.
void BeginTabRow(float height);
void EndTabRow();

// -------------------------------------------------------------- widgets ---

bool Checkbox(const char* label, bool* value);
bool SliderFloat(const char* label, float* value, float min, float max,
                 const char* format = "%.2f");
bool SliderInt(const char* label, int* value, int min, int max,
               const char* format = "%d");
bool Combo(const char* label, int* value, const char* const items[], int count);

// Dropdown of independent toggles; `values` holds `count` flags.
bool MultiCombo(const char* label, bool* values, const char* const items[], int count);

// Click, then press any key. Right-click clears to `none` (0).
bool Keybind(const char* label, int* vk);

// Swatch plus a hand-drawn SV / hue / alpha popup.
bool ColorPicker(const char* label, ImVec4* color);

bool Button(const char* label);

// Name of a virtual-key code ("F1", "MOUSE 5", "ALT", ...). Never null.
const char* KeyName(int vk);

} // namespace skeet
