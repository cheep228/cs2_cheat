#include <Windows.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#include "imgui.h"
#include "imgui_internal.h"

#include "ui/theme.h"

namespace theme {
namespace {

struct GroupState {
    ImGuiID heightId = 0;
    ImGuiStorage* parentStorage = nullptr;
    bool autoSize = false;
};

GroupState g_groups[16];
int g_groupDepth = 0;
ImGuiID g_capturingKey = 0;

ImVec4 U32ToVec4(ImU32 c) {
    return ImGui::ColorConvertU32ToFloat4(c);
}

float Scale() {
    return ImGui::GetIO().GUIScale > 0.f ? ImGui::GetIO().GUIScale : 1.f;
}

void TextVColor(const ImVec4& color, const char* fmt, va_list args) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
}

} // namespace

void ApplyStyle() {
    // Deliberately no ImGuiStyle redesign here. The supplied Neverlose source
    // creates a context and keeps this fork's default style; its widgets draw
    // from Colors::* directly. Fonts and custom IO fields are initialized by
    // menu::ApplyStyle().
}


float ColumnWidth(int columns) {
    const float sc = Scale();
    if (columns <= 1)
        return (metric::kChildW * 2.f + metric::kColumnGap) * sc;
    return metric::kChildW * sc;
}

void BeginGroupBox(const char* label, const ImVec2& size) {
    IM_ASSERT(g_groupDepth < IM_ARRAYSIZE(g_groups));

    ImGuiWindow* parent = ImGui::GetCurrentWindow();
    const float sc = Scale();
    const ImGuiID heightId = parent->GetID((std::string("##nl_height_") + label).c_str());
    const bool autoSize = size.y <= 0.f;

    float bodyH = size.y;
    if (autoSize) {
        bodyH = ImGui::GetStateStorage()->GetFloat(heightId, 115.f * sc);
        bodyH = ImClamp(bodyH, 47.f * sc, 470.f * sc);
    }

    float width = size.x;
    if (width <= 0.f)
        width = ColumnWidth(2);

    GroupState& st = g_groups[g_groupDepth++];
    st.heightId = heightId;
    st.parentStorage = ImGui::GetStateStorage();
    st.autoSize = autoSize;

    // MenuChild's original renderer paints its caption 30px above the child
    // window. Grouping a 30px spacer with the child makes SameLine() align the
    // complete visual panel rather than the child body only.
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(1.f, metric::kChildHeaderH * sc));
    ImGui::MenuChild(label, ImVec2(width, bodyH));
}

void EndGroupBox() {
    IM_ASSERT(g_groupDepth > 0);
    GroupState st = g_groups[--g_groupDepth];

    if (st.autoSize) {
        // CursorPosY is relative to the child and already contains the fork's
        // +15px content offset. Leave a little breathing room at the bottom.
        const float used = ImGui::GetCursorPosY() + 8.f * Scale();
        if (st.parentStorage)
            st.parentStorage->SetFloat(st.heightId,
                ImClamp(used, 47.f * Scale(), 470.f * Scale()));
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

void Separator() {
    ImGui::Separator();
}

void Text(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}

void TextDim(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextVColor(Colors::Text, fmt, args);
    va_end(args);
}

void TextColored(ImU32 color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextVColor(U32ToVec4(color), fmt, args);
    va_end(args);
}

bool Tab(const char* label, int index, int* current) {
    const bool pressed = ImGui::TabButton("", label, *current == index);
    if (pressed)
        *current = index;
    return pressed;
}

bool SubTab(const char* label, int index, int* current) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const bool active = *current == index;
    ImGui::PushID(index);

    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 size(textSize.x + 18.f * Scale(), 26.f * Scale());
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::ButtonEx(label, size, ImGuiButtonFlags_TextOnly);
    if (pressed)
        *current = index;

    if (active) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(start.x + 5.f * Scale(), start.y + size.y - 2.f * Scale()),
                          ImVec2(start.x + size.x - 5.f * Scale(), start.y + size.y),
                          ImColor(Colors::Accent), 1.f);
    }

    ImGui::PopID();
    return pressed;
}

void BeginTabRow(float) {
    ImGui::BeginGroup();
}

void EndTabRow() {
    ImGui::EndGroup();
}

bool Checkbox(const char* label, bool* value) {
    return ImGui::Checkbox(label, value);
}

bool SliderFloat(const char* label, float* value, float min, float max, const char* format) {
    return ImGui::SliderFloat(label, value, min, max, format);
}

bool SliderInt(const char* label, int* value, int min, int max, const char* format) {
    return ImGui::SliderInt(label, value, min, max, format);
}

bool Combo(const char* label, int* value, const char* const items[], int count) {
    return ImGui::Combo(label, value, items, count);
}

bool MultiCombo(const char* label, bool* values, const char* const items[], int count) {
    std::string preview;
    for (int i = 0; i < count; ++i) {
        if (!values[i])
            continue;
        if (!preview.empty())
            preview += ", ";
        preview += items[i];
    }
    if (preview.empty())
        preview = "Select";

    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str())) {
        ImGui::Spacing();
        ImGui::BeginGroup();
        for (int i = 0; i < count; ++i) {
            if (ImGui::Selectable(items[i], &values[i], ImGuiSelectableFlags_DontClosePopups))
                changed = true;
        }
        ImGui::EndGroup();
        ImGui::Spacing();
        ImGui::EndCombo();
    }
    return changed;
}

const char* KeyName(int vk) {
    static char buf[32];
    if (vk == 0) return "none";
    switch (vk) {
    case VK_LBUTTON: return "MOUSE 1";
    case VK_RBUTTON: return "MOUSE 2";
    case VK_MBUTTON: return "MOUSE 3";
    case VK_XBUTTON1: return "MOUSE 4";
    case VK_XBUTTON2: return "MOUSE 5";
    case VK_INSERT: return "INSERT";
    case VK_DELETE: return "DELETE";
    case VK_HOME: return "HOME";
    case VK_END: return "END";
    case VK_PRIOR: return "PAGE UP";
    case VK_NEXT: return "PAGE DOWN";
    case VK_SPACE: return "SPACE";
    case VK_TAB: return "TAB";
    case VK_SHIFT: return "SHIFT";
    case VK_CONTROL: return "CTRL";
    case VK_MENU: return "ALT";
    case VK_LSHIFT: return "LSHIFT";
    case VK_RSHIFT: return "RSHIFT";
    case VK_LCONTROL: return "LCTRL";
    case VK_RCONTROL: return "RCTRL";
    case VK_LMENU: return "LALT";
    case VK_RMENU: return "RALT";
    default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F24) {
        std::snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1);
        return buf;
    }
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        std::snprintf(buf, sizeof(buf), "%c", static_cast<char>(vk));
        return buf;
    }
    std::snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

bool Keybind(const char* label, int* vk) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGui::PushID(label);
    const ImGuiID id = window->GetID("##keybind");
    const bool capturing = g_capturingKey == id;
    const char* value = capturing ? "..." : KeyName(*vk);

    ImGui::TextColored(Colors::Text, "%s", label);
    ImGui::SameLine();

    const float colW = ImGui::GetColumnWidth();
    const float boxW = 92.f * Scale();
    const float x = ImMax(ImGui::GetCursorPosX(), colW - boxW - 10.f * Scale());
    ImGui::SetCursorPosX(x);

    ImGui::PushStyleColor(ImGuiCol_Text, capturing ? Colors::Accent : Colors::TextActive);
    const bool pressed = ImGui::Button(value, ImVec2(boxW, 22.f * Scale()));
    ImGui::PopStyleColor();
    if (pressed)
        g_capturingKey = capturing ? 0 : id;

    bool changed = false;
    if (capturing) {
        for (int key = 1; key <= 0xFE; ++key) {
            if ((GetAsyncKeyState(key) & 0x8000) == 0)
                continue;
            if (key == VK_ESCAPE) {
                g_capturingKey = 0;
                break;
            }
            *vk = key;
            g_capturingKey = 0;
            changed = true;
            break;
        }
    }

    ImGui::PopID();
    return changed;
}

bool ColorPicker(const char* label, ImVec4* color) {
    ImGui::PushID(label);
    ImGui::TextColored(Colors::Text, "%s", label);
    ImGui::SameLine();
    const float colW = ImGui::GetColumnWidth();
    ImGui::SetCursorPosX(ImMax(ImGui::GetCursorPosX(), colW - 55.f * Scale()));
    const bool changed = ImGui::ColorEdit4("##color", &color->x,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
    ImGui::PopID();
    return changed;
}

bool Button(const char* label) {
    return ImGui::Button(label, ImVec2(ImGui::GetColumnWidth() - 20.f * Scale(), 25.f * Scale()));
}

} // namespace theme
