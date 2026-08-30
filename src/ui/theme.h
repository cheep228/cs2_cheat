#pragma once
#include <cstdint>

#include "imgui.h"

// Adapter for the project's existing tab files. The visible widgets are the
// implementations from the supplied Neverlose ImGui fork (deps/imgui_nl):
// Checkbox, SliderScalar, BeginCombo, Selectable, ButtonEx, RadioButton,
// ScrollbarEx, MenuChild and TabButton are not recreated here.
namespace theme {

namespace col {
constexpr ImU32 kAccent = IM_COL32(0, 165, 243, 255);
constexpr ImU32 kBad    = IM_COL32(224, 90, 90, 255);
constexpr ImU32 kGood   = IM_COL32(90, 200, 120, 255);
}

namespace metric {
constexpr float kChildW = 275.f;
constexpr float kColumnGap = 16.f;
constexpr float kChildHeaderH = 30.f;
}

void ApplyStyle();

float ColumnWidth(int columns);
void BeginGroupBox(const char* label, const ImVec2& size);
void EndGroupBox();

void Separator();
void Text(const char* fmt, ...);
void TextDim(const char* fmt, ...);
void TextColored(ImU32 color, const char* fmt, ...);

bool Tab(const char* label, int index, int* current);
bool SubTab(const char* label, int index, int* current);
void BeginTabRow(float height);
void EndTabRow();

bool Checkbox(const char* label, bool* value);
bool SliderFloat(const char* label, float* value, float min, float max,
                 const char* format = "%.2f");
bool SliderInt(const char* label, int* value, int min, int max,
               const char* format = "%d");
bool Combo(const char* label, int* value, const char* const items[], int count);
bool MultiCombo(const char* label, bool* values, const char* const items[], int count);
bool Keybind(const char* label, int* vk);
bool ColorPicker(const char* label, ImVec4* color);
bool Button(const char* label);
const char* KeyName(int vk);

} // namespace theme
