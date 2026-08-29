#pragma once
#include <cstdint>

#include "imgui.h"

// Neverlose-style visual layer.
//
// The palette and metrics are ported from the neverlose CS:GO menu; the code is
// not. Every widget below is drawn here through ImDrawList, and ImGui keeps only
// the parts that are genuinely hard to redo correctly - item ids, hover and
// active state, ButtonBehavior, SliderBehavior.
//
// The look, in one sentence: near-black background, rounded frames, a blue
// accent, a left sidebar for the main tabs, and everything eased rather than
// snapped. That last part is what actually reads as "neverlose" - a static
// screenshot of this palette looks wrong until the toggles slide.
//
// Tab content lives in ui/tab_*.cpp - this file must stay free of it.
namespace theme {

// --------------------------------------------------------------- palette --
namespace col {

constexpr ImU32 kWindowBg    = IM_COL32(0x08, 0x08, 0x08, 0xFF);  // Background
constexpr ImU32 kSidebar     = IM_COL32(0x08, 0x08, 0x08, 0xFF);  // LeftBar
constexpr ImU32 kChildBg     = IM_COL32(0x0B, 0x0B, 0x0D, 0xFF);  // Child
constexpr ImU32 kControlBg   = IM_COL32(0x03, 0x06, 0x0F, 0xFF);  // Frame
constexpr ImU32 kFrameHover  = IM_COL32(0x0D, 0x0D, 0x0D, 0xFF);  // FrameHovered
constexpr ImU32 kFrameActive = IM_COL32(0x04, 0x19, 0x2E, 0xFF);  // FrameActive
constexpr ImU32 kFrameOpen   = IM_COL32(0x20, 0x1F, 0x21, 0xFF);  // FrameOpened

constexpr ImU32 kBorder      = IM_COL32(0x1A, 0x1A, 0x1A, 0xFF);  // Borders
constexpr ImU32 kBorderHot   = IM_COL32(0x24, 0x22, 0x26, 0xFF);  // BordersActive
constexpr ImU32 kTabFrame    = IM_COL32(0x3E, 0x3F, 0x3A, 0xFF);  // TabFrame

constexpr ImU32 kText        = IM_COL32(0xA2, 0xB0, 0xB9, 0xFF);  // Text
constexpr ImU32 kTextActive  = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);  // TextActive
constexpr ImU32 kChildText   = IM_COL32(0xDD, 0xDC, 0xE1, 0xFF);  // ChildText
constexpr ImU32 kTextDim     = IM_COL32(0x4C, 0x5A, 0x65, 0xFF);  // AccentDisabled
constexpr ImU32 kLogo        = IM_COL32(0xFD, 0xFE, 0xFF, 0xFF);  // Logo

constexpr ImU32 kAccent      = IM_COL32(0x00, 0xA5, 0xF3, 0xFF);  // Accent
constexpr ImU32 kAccentDim   = IM_COL32(0x4C, 0x5A, 0x65, 0xFF);  // AccentDisabled

constexpr ImU32 kBad         = IM_COL32(0xE0, 0x5A, 0x5A, 0xFF);
constexpr ImU32 kGood        = IM_COL32(0x5A, 0xC8, 0x78, 0xFF);

} // namespace col

// --------------------------------------------------------------- metrics --
namespace metric {

constexpr float kSidebarW   = 200.f;   // left tab column
constexpr float kTabW       = 180.f;   // tab pill, inset 15 px each side
constexpr float kTabH       = 30.f;
constexpr float kToggleW    = 26.f;    // pill switch
constexpr float kToggleH    = 13.f;
constexpr float kKnobR      = 5.5f;
constexpr float kSliderBar  = 6.f;
constexpr float kControlH   = 22.f;
constexpr float kRowGap     = 5.f;
constexpr float kGroupPadX  = 12.f;
constexpr float kGroupPadY  = 26.f;    // room for the group header
constexpr float kRounding   = 4.f;
constexpr float kTabRowH    = kTabH;
constexpr float kSubTabRowH = 26.f;

// Everything eases at DeltaTime * this. Neverlose uses 8 for widgets and 12
// for the theme colours; keeping the widget number here matches the feel.
constexpr float kAnimSpeed = 8.f;

} // namespace metric

void ApplyStyle();

// Window background, border and the sidebar column, plus the product name.
void WindowChrome(const char* title);

// --------------------------------------------------------------- layout ---

float ColumnWidth(int columns);

// Rounded panel with its caption inside the top-left. `size.y` of 0 measures
// the content, which costs one frame of settling.
void BeginGroupBox(const char* label, const ImVec2& size);
void EndGroupBox();

void Separator();
void Text(const char* fmt, ...);
void TextDim(const char* fmt, ...);
void TextColored(ImU32 color, const char* fmt, ...);

// ----------------------------------------------------------------- tabs ---

// Sidebar pill, 180x30 inset by 15. Returns true on the frame it is picked.
bool Tab(const char* label, int index, int* current);

// Horizontal sub-tab with an animated accent underline.
bool SubTab(const char* label, int index, int* current);

void BeginTabRow(float height);
void EndTabRow();

// -------------------------------------------------------------- widgets ---

// A sliding pill switch, not a tick box - this is the single most recognisable
// part of the neverlose look.
bool Checkbox(const char* label, bool* value);

bool SliderFloat(const char* label, float* value, float min, float max,
                 const char* format = "%.2f");
bool SliderInt(const char* label, int* value, int min, int max,
               const char* format = "%d");
bool Combo(const char* label, int* value, const char* const items[], int count);
bool MultiCombo(const char* label, bool* values, const char* const items[], int count);

// Click, then press any key. Right-click clears to none.
bool Keybind(const char* label, int* vk);

bool ColorPicker(const char* label, ImVec4* color);
bool Button(const char* label);

const char* KeyName(int vk);

} // namespace theme
