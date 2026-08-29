#include <Windows.h>

#include <cstdarg>
#include <cstdio>

#include "imgui.h"
#include "imgui_internal.h"

#include "ui/skeet.h"

namespace skeet {
namespace {

using namespace metric;

// The caption of a groupbox has to be drawn before its child window opens,
// otherwise the child's draw commands can land on top of it. Only the geometry
// and the auto-height bookkeeping survive until EndGroupBox.
struct GroupState {
    ImVec2        min{};
    ImVec2        max{};
    ImGuiID       id       = 0;
    ImGuiStorage* storage  = nullptr;
    bool          autoSize = false;
};

constexpr int kMaxGroupDepth = 8;
GroupState g_groups[kMaxGroupDepth];
int        g_groupDepth = 0;

// Which keybind widget is listening for a press, keyed by ImGui id so two
// pickers cannot swallow each other's key.
ImGuiID g_capturing = 0;

// Row of tabs currently being laid out, so the underline can span it.
struct TabRow {
    ImVec2 min{};
    float  height = 0.f;
    bool   open   = false;
};
TabRow g_tabRow;

ImU32 Blend(ImU32 a, ImU32 b, float t) {
    const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
    const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(ca.x + (cb.x - ca.x) * t,
                                                 ca.y + (cb.y - ca.y) * t,
                                                 ca.z + (cb.z - ca.z) * t,
                                                 ca.w + (cb.w - ca.w) * t));
}

void DrawTextAt(ImDrawList* d, const ImVec2& pos, ImU32 color, const char* text) {
    d->AddText(ImVec2(ImFloor(pos.x), ImFloor(pos.y)), color, text);
}

// Every control occupies the full inner width of its groupbox.
float RowWidth() {
    const float w = ImGui::GetContentRegionAvail().x;
    return w < 24.f ? 24.f : w;
}

// Shared frame for combo / keybind / button boxes.
void DrawBox(ImDrawList* d, const ImRect& bb, bool hovered, bool active) {
    d->AddRectFilled(bb.Min, bb.Max, col::kControlBg);
    ImU32 border = col::kBorder;
    if (active)
        border = col::kAccent;
    else if (hovered)
        border = Blend(col::kBorder, col::kAccent, 0.55f);
    d->AddRect(bb.Min, bb.Max, border);
}

void DrawArrow(ImDrawList* d, const ImRect& bb, ImU32 color) {
    const float cx = bb.Max.x - 7.f;
    const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
    d->AddTriangleFilled(ImVec2(cx - 3.f, cy - 1.5f), ImVec2(cx + 3.f, cy - 1.5f),
                         ImVec2(cx, cy + 2.5f), color);
}

// Popup chrome shared by combo, multi-combo and the colour picker.
bool BeginStyledPopup(const char* id, const ImRect& anchor, float width) {
    ImGui::SetNextWindowPos(ImVec2(anchor.Min.x, anchor.Max.y + 1.f));
    if (width > 0.f)
        ImGui::SetNextWindowSize(ImVec2(width, 0.f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.f, 2.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(col::kWindowBg));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(col::kBorder));

    const bool open = ImGui::BeginPopup(id, ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_AlwaysAutoResize);
    if (!open) {
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
    }
    return open;
}

void EndStyledPopup() {
    ImGui::EndPopup();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

// One selectable row inside a popup, drawn by hand so it matches the rest.
bool PopupRow(const char* text, bool selected, bool showTick) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID id = win->GetID(text);

    const float h = ImGui::GetTextLineHeight() + 3.f;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + h));

    ImGui::ItemSize(ImVec2(bb.GetWidth(), h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* d = win->DrawList;
    if (hovered)
        d->AddRectFilled(bb.Min, bb.Max, IM_COL32(0x1E, 0x1E, 0x1E, 0xFF));

    float textX = bb.Min.x + 4.f;
    if (showTick) {
        const float s = 7.f;
        const ImVec2 bmin(bb.Min.x + 4.f, (bb.Min.y + bb.Max.y) * 0.5f - s * 0.5f);
        const ImVec2 bmax(bmin.x + s, bmin.y + s);
        d->AddRectFilled(bmin, bmax, selected ? col::kAccent : col::kControlBg);
        d->AddRect(bmin, bmax, selected ? col::kAccent : col::kBorder);
        textX = bmax.x + 5.f;
    }

    const ImU32 color = selected ? col::kAccent : (hovered ? col::kText : col::kTextDim);
    DrawTextAt(d, ImVec2(textX, bb.Min.y + 1.f), color, text);
    return pressed;
}

} // namespace

// ---------------------------------------------------------------- style ---

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();

    // Skeet is square. Nothing in this menu gets a rounded corner.
    s.WindowRounding    = 0.f;
    s.ChildRounding     = 0.f;
    s.FrameRounding     = 0.f;
    s.PopupRounding     = 0.f;
    s.GrabRounding      = 0.f;
    s.TabRounding       = 0.f;
    s.ScrollbarRounding = 0.f;

    // The window and child borders are drawn by hand, so ImGui draws none.
    s.WindowBorderSize = 0.f;
    s.ChildBorderSize  = 0.f;
    s.PopupBorderSize  = 1.f;
    s.FrameBorderSize  = 1.f;

    s.WindowPadding    = ImVec2(8.f, 8.f);
    s.FramePadding     = ImVec2(4.f, 2.f);
    s.ItemSpacing      = ImVec2(4.f, kRowGap);
    s.ItemInnerSpacing = ImVec2(4.f, 2.f);
    s.IndentSpacing    = 10.f;
    s.ScrollbarSize    = 7.f;
    s.GrabMinSize      = 6.f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]  = ImGui::ColorConvertU32ToFloat4(col::kWindowBg);
    c[ImGuiCol_ChildBg]   = ImGui::ColorConvertU32ToFloat4(col::kChildBg);
    c[ImGuiCol_PopupBg]   = ImGui::ColorConvertU32ToFloat4(col::kWindowBg);
    c[ImGuiCol_Border]    = ImGui::ColorConvertU32ToFloat4(col::kBorder);
    c[ImGuiCol_Text]      = ImGui::ColorConvertU32ToFloat4(col::kText);
    c[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(col::kTextDim);
    c[ImGuiCol_FrameBg]   = ImGui::ColorConvertU32ToFloat4(col::kControlBg);
    c[ImGuiCol_ScrollbarBg]   = ImGui::ColorConvertU32ToFloat4(col::kControlBg);
    c[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(col::kBorder);
    c[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(col::kAccent);
    c[ImGuiCol_ScrollbarGrabActive]  = ImGui::ColorConvertU32ToFloat4(col::kAccent);
}

void WindowChrome(const char* title) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetWindowPos();
    const ImVec2 s = ImGui::GetWindowSize();
    const ImVec2 br(p.x + s.x, p.y + s.y);

    // Three nested rectangles: hairline outside, soft inset, then the content
    // frame. This layering is what reads as "Skeet" more than any single colour.
    // The fill is left to ImGui's own WindowBg so the opacity slider still works.
    d->AddRect(p, br, col::kBorder);
    d->AddRect(ImVec2(p.x + 2.f, p.y + 2.f), ImVec2(br.x - 2.f, br.y - 2.f),
               col::kBorderSoft);

    // Title strip with an accent rule under it.
    const float strip = 17.f;
    const ImVec2 t0(p.x + 3.f, p.y + 3.f);
    const ImVec2 t1(br.x - 3.f, p.y + 3.f + strip);
    d->AddRectFilled(t0, t1, IM_COL32(0x0D, 0x0D, 0x0D, 0xFF));
    d->AddRect(t0, t1, col::kBorderSoft);
    DrawTextAt(d, ImVec2(t0.x + 6.f, t0.y + 2.f), col::kAccent, title);

    const ImVec2 ts = ImGui::CalcTextSize(title);
    d->AddLine(ImVec2(t0.x + 6.f, t1.y - 1.f), ImVec2(t0.x + 6.f + ts.x, t1.y - 1.f),
               col::kAccent);
}

// --------------------------------------------------------------- layout ---

float ColumnWidth(int columns) {
    if (columns < 1)
        columns = 1;
    const float total = ImGui::GetContentRegionAvail().x;
    const float gaps = ImGui::GetStyle().ItemSpacing.x * (columns - 1);
    return (total - gaps) / static_cast<float>(columns);
}

void BeginGroupBox(const char* label, const ImVec2& size) {
    IM_ASSERT(g_groupDepth < kMaxGroupDepth);

    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImGui::PushID(label);

    const ImGuiID id = win->GetID("##groupbox");
    ImGuiStorage* storage = ImGui::GetStateStorage();

    ImVec2 sz = size;
    if (sz.x <= 0.f)
        sz.x = ImGui::GetContentRegionAvail().x + sz.x;

    const bool autoSize = sz.y <= 0.f;
    if (autoSize)
        sz.y = storage->GetFloat(id, 48.f);

    const ImVec2 pos = win->DC.CursorPos;

    GroupState& g = g_groups[g_groupDepth++];
    g.min = pos;
    g.max = ImVec2(pos.x + sz.x, pos.y + sz.y);
    g.id = id;
    g.storage = storage;
    g.autoSize = autoSize;

    // Frame first, caption second, content last: the caption must exist before
    // the child window opens so nothing can be drawn over it.
    ImDrawList* d = win->DrawList;
    d->AddRectFilled(g.min, g.max, col::kChildBg);
    d->AddRect(g.min, g.max, col::kBorder);

    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float lx = g.min.x + kGroupPadX;
    d->AddRectFilled(ImVec2(lx - 3.f, g.min.y - 1.f),
                     ImVec2(lx + ts.x + 3.f, g.min.y + 1.f), col::kChildBg);
    DrawTextAt(d, ImVec2(lx, g.min.y - ts.y * 0.5f), col::kTextDim, label);

    // The child is inset so every control inside can simply take the full
    // content width and still keep the padding.
    ImGui::SetCursorScreenPos(ImVec2(pos.x + kGroupPadX, pos.y + kGroupPadY));
    ImGui::BeginChild("##content",
                      ImVec2(sz.x - kGroupPadX * 2.f, sz.y - kGroupPadY - 5.f),
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
}

void EndGroupBox() {
    IM_ASSERT(g_groupDepth > 0);

    // Height the content actually needed this frame, before the child closes.
    const float used = ImGui::GetCursorPosY();
    ImGui::EndChild();

    GroupState& g = g_groups[--g_groupDepth];
    if (g.autoSize && g.storage)
        g.storage->SetFloat(g.id, used + kGroupPadY + 4.f);

    // Put the cursor back so the groupbox occupies exactly its own rectangle.
    ImGui::SetCursorScreenPos(g.min);
    ImGui::ItemSize(ImVec2(g.max.x - g.min.x, g.max.y - g.min.y), 0.f);
    ImGui::PopID();
}

void Separator() {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    ImGui::ItemSize(ImVec2(w, 3.f), 0.f);
    win->DrawList->AddLine(ImVec2(pos.x, pos.y + 1.f), ImVec2(pos.x + w, pos.y + 1.f),
                           col::kBorderSoft);
}

namespace {

void TextV(ImU32 color, const char* fmt, va_list args) {
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);

    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImVec2 pos = win->DC.CursorPos;
    const ImVec2 ts = ImGui::CalcTextSize(buf);

    ImGui::ItemSize(ImVec2(RowWidth(), ts.y), 0.f);
    if (win->SkipItems)
        return;
    DrawTextAt(win->DrawList, pos, color, buf);
}

} // namespace

void Text(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextV(col::kText, fmt, args);
    va_end(args);
}

void TextDim(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextV(col::kTextDim, fmt, args);
    va_end(args);
}

void TextColored(ImU32 color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextV(color, fmt, args);
    va_end(args);
}

// ----------------------------------------------------------------- tabs ---

void BeginTabRow(float height) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    g_tabRow.min = win->DC.CursorPos;
    g_tabRow.height = height;
    g_tabRow.open = true;
}

void EndTabRow() {
    if (!g_tabRow.open)
        return;
    g_tabRow.open = false;

    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const float w = ImGui::GetContentRegionAvail().x;
    const float y = g_tabRow.min.y + g_tabRow.height;
    win->DrawList->AddLine(ImVec2(g_tabRow.min.x, y), ImVec2(g_tabRow.min.x + w, y),
                           col::kBorder);

    ImGui::SetCursorScreenPos(ImVec2(g_tabRow.min.x, y + 4.f));
}

namespace {

// Main tabs and sub-tabs differ only in metrics and in how the active one is
// marked, so they share one implementation.
bool TabImpl(const char* label, int index, int* current, float height, bool underline) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID id = win->GetID(label);

    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float padX = underline ? 11.f : 8.f;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + ts.x + padX * 2.f, pos.y + height));

    ImGui::ItemSize(ImVec2(bb.GetWidth(), height), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *current = index;

    const bool active = (*current == index);

    ImDrawList* d = win->DrawList;
    ImU32 color = col::kTextDim;
    if (active)
        color = col::kAccent;
    else if (hovered)
        color = col::kText;

    DrawTextAt(d, ImVec2(bb.Min.x + padX, bb.Min.y + (height - ts.y) * 0.5f),
               color, label);

    if (active && underline)
        d->AddLine(ImVec2(bb.Min.x + 2.f, bb.Max.y - 1.f),
                   ImVec2(bb.Max.x - 2.f, bb.Max.y - 1.f), col::kAccent);
    else if (active)
        d->AddLine(ImVec2(bb.Min.x + 4.f, bb.Max.y - 2.f),
                   ImVec2(bb.Max.x - 4.f, bb.Max.y - 2.f), col::kAccent);

    ImGui::SameLine(0.f, 0.f);
    return pressed;
}

} // namespace

bool Tab(const char* label, int index, int* current) {
    return TabImpl(label, index, current, kTabRowH, true);
}

bool SubTab(const char* label, int index, int* current) {
    return TabImpl(label, index, current, kSubTabRowH, false);
}

// -------------------------------------------------------------- widgets ---

bool Checkbox(const char* label, bool* value) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float h = ts.y > kCheckBox ? ts.y : kCheckBox;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + RowWidth(), pos.y + h));

    ImGui::ItemSize(ImVec2(bb.GetWidth(), h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) {
        *value = !*value;
        ImGui::MarkItemEdited(id);
    }

    const ImVec2 bmin(pos.x, pos.y + (h - kCheckBox) * 0.5f);
    const ImVec2 bmax(bmin.x + kCheckBox, bmin.y + kCheckBox);

    ImDrawList* d = win->DrawList;
    if (*value) {
        d->AddRectFilled(bmin, bmax, hovered ? col::kAccentHot : col::kAccent);
        d->AddRect(bmin, bmax, col::kAccent);
    } else {
        d->AddRectFilled(bmin, bmax, col::kControlBg);
        d->AddRect(bmin, bmax, hovered ? Blend(col::kBorder, col::kAccent, 0.6f)
                                       : col::kBorder);
    }

    const ImU32 textColor = ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled
                                ? col::kTextDim
                                : (*value ? col::kText : col::kTextDim);
    DrawTextAt(d, ImVec2(bmax.x + 5.f, pos.y + (h - ts.y) * 0.5f), textColor, label);
    return pressed;
}

namespace {

// One slider implementation for both scalar types; the caller supplies the
// data type so ImGui's own SliderBehavior does the clamping and the keyboard
// and gamepad handling.
bool SliderImpl(const char* label, ImGuiDataType type, void* value,
                const void* min, const void* max, const char* format) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const float textH = ImGui::GetTextLineHeight();
    const float h = textH + 2.f + kSliderBar;
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    const ImRect bar(ImVec2(pos.x, pos.y + textH + 2.f),
                     ImVec2(pos.x + w, pos.y + textH + 2.f + kSliderBar));

    ImGui::ItemSize(ImVec2(w, h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    ImRect grab;
    const bool changed = ImGui::SliderBehavior(bar, id, type, value, min, max,
                                               format, ImGuiSliderFlags_None, &grab);
    if (changed)
        ImGui::MarkItemEdited(id);

    const bool hovered = ImGui::IsItemHovered();

    // Filled fraction, taken from the value rather than from the grab box so a
    // zero-width grab still paints the correct amount.
    float t = 0.f;
    if (type == ImGuiDataType_Float) {
        const float v = *static_cast<float*>(value);
        const float lo = *static_cast<const float*>(min);
        const float hi = *static_cast<const float*>(max);
        t = hi > lo ? (v - lo) / (hi - lo) : 0.f;
    } else {
        const int v = *static_cast<int*>(value);
        const int lo = *static_cast<const int*>(min);
        const int hi = *static_cast<const int*>(max);
        t = hi > lo ? static_cast<float>(v - lo) / static_cast<float>(hi - lo) : 0.f;
    }
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);

    char valueText[64];
    ImGui::DataTypeFormatString(valueText, IM_ARRAYSIZE(valueText), type, value, format);

    ImDrawList* d = win->DrawList;
    DrawTextAt(d, pos, col::kTextDim, label);

    const ImVec2 vs = ImGui::CalcTextSize(valueText);
    DrawTextAt(d, ImVec2(pos.x + w - vs.x, pos.y), col::kText, valueText);

    d->AddRectFilled(bar.Min, bar.Max, col::kControlBg);
    if (t > 0.f) {
        const float fill = bar.Min.x + (bar.Max.x - bar.Min.x) * t;
        d->AddRectFilled(ImVec2(bar.Min.x + 1.f, bar.Min.y + 1.f),
                         ImVec2(fill, bar.Max.y - 1.f),
                         hovered ? col::kAccentHot : col::kAccent);
    }
    d->AddRect(bar.Min, bar.Max, hovered ? Blend(col::kBorder, col::kAccent, 0.6f)
                                         : col::kBorder);
    return changed;
}

} // namespace

bool SliderFloat(const char* label, float* value, float min, float max,
                 const char* format) {
    return SliderImpl(label, ImGuiDataType_Float, value, &min, &max, format);
}

bool SliderInt(const char* label, int* value, int min, int max, const char* format) {
    return SliderImpl(label, ImGuiDataType_S32, value, &min, &max, format);
}

namespace {

// Label line plus the control box, shared by combo, multi-combo and keybind.
ImRect LabelledBox(const char* label, float boxHeight, bool* hovered, bool* pressed,
                   ImGuiID id) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const float textH = ImGui::GetTextLineHeight();
    const float w = RowWidth();
    const ImVec2 pos = win->DC.CursorPos;

    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + textH + 2.f + boxHeight));
    const ImRect box(ImVec2(pos.x, pos.y + textH + 2.f),
                     ImVec2(pos.x + w, pos.y + textH + 2.f + boxHeight));

    ImGui::ItemSize(ImVec2(w, bb.GetHeight()), 0.f);
    if (!ImGui::ItemAdd(bb, id)) {
        *hovered = false;
        *pressed = false;
        return box;
    }

    bool held = false;
    *pressed = ImGui::ButtonBehavior(box, id, hovered, &held);
    DrawTextAt(win->DrawList, pos, col::kTextDim, label);
    return box;
}

} // namespace

bool Combo(const char* label, int* value, const char* const items[], int count) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    ImGui::PushID(label);
    const ImGuiID id = win->GetID("##combo");

    bool hovered = false, pressed = false;
    const ImRect box = LabelledBox(label, kControlH, &hovered, &pressed, id);

    const bool popupOpen = ImGui::IsPopupOpen("##combopopup");
    DrawBox(win->DrawList, box, hovered, popupOpen);

    const char* preview = (*value >= 0 && *value < count) ? items[*value] : "-";
    DrawTextAt(win->DrawList, ImVec2(box.Min.x + 5.f, box.Min.y + 1.f),
               col::kText, preview);
    DrawArrow(win->DrawList, box, popupOpen ? col::kAccent : col::kTextDim);

    if (pressed)
        ImGui::OpenPopup("##combopopup");

    bool changed = false;
    if (BeginStyledPopup("##combopopup", box, box.GetWidth())) {
        for (int i = 0; i < count; ++i) {
            if (PopupRow(items[i], i == *value, false)) {
                *value = i;
                changed = true;
                ImGui::CloseCurrentPopup();
            }
        }
        EndStyledPopup();
    }

    ImGui::PopID();
    return changed;
}

bool MultiCombo(const char* label, bool* values, const char* const items[], int count) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    ImGui::PushID(label);
    const ImGuiID id = win->GetID("##multicombo");

    bool hovered = false, pressed = false;
    const ImRect box = LabelledBox(label, kControlH, &hovered, &pressed, id);

    const bool popupOpen = ImGui::IsPopupOpen("##multipopup");
    DrawBox(win->DrawList, box, hovered, popupOpen);

    // Preview: joined names, trimmed to whatever fits before the arrow.
    char preview[128] = "none";
    int enabled = 0;
    for (int i = 0; i < count; ++i)
        enabled += values[i] ? 1 : 0;

    if (enabled == count && count > 0) {
        snprintf(preview, sizeof(preview), "all");
    } else if (enabled > 0) {
        preview[0] = '\0';
        for (int i = 0, written = 0; i < count; ++i) {
            if (!values[i])
                continue;
            const char* sep = written++ ? ", " : "";
            const size_t len = strlen(preview);
            snprintf(preview + len, sizeof(preview) - len, "%s%s", sep, items[i]);
        }
    }

    const float textLimit = box.GetWidth() - 18.f;
    ImVec2 previewSize = ImGui::CalcTextSize(preview);
    if (previewSize.x > textLimit) {
        // Cut and mark it, rather than letting the arrow be overdrawn.
        for (int len = static_cast<int>(strlen(preview)); len > 3; --len) {
            preview[len - 1] = '\0';
            char probe[132];
            snprintf(probe, sizeof(probe), "%s..", preview);
            if (ImGui::CalcTextSize(probe).x <= textLimit) {
                snprintf(preview + strlen(preview), sizeof(preview) - strlen(preview), "..");
                break;
            }
        }
    }

    DrawTextAt(win->DrawList, ImVec2(box.Min.x + 5.f, box.Min.y + 1.f),
               enabled ? col::kText : col::kTextDim, preview);
    DrawArrow(win->DrawList, box, popupOpen ? col::kAccent : col::kTextDim);

    if (pressed)
        ImGui::OpenPopup("##multipopup");

    bool changed = false;
    if (BeginStyledPopup("##multipopup", box, box.GetWidth())) {
        for (int i = 0; i < count; ++i) {
            if (PopupRow(items[i], values[i], true)) {
                values[i] = !values[i];
                changed = true;   // stays open: picking several is the point
            }
        }
        EndStyledPopup();
    }

    ImGui::PopID();
    return changed;
}

const char* KeyName(int vk) {
    static char buf[24];

    switch (vk) {
    case 0:            return "-";
    case VK_LBUTTON:   return "MOUSE 1";
    case VK_RBUTTON:   return "MOUSE 2";
    case VK_MBUTTON:   return "MOUSE 3";
    case VK_XBUTTON1:  return "MOUSE 4";
    case VK_XBUTTON2:  return "MOUSE 5";
    case VK_BACK:      return "BACKSPACE";
    case VK_TAB:       return "TAB";
    case VK_RETURN:    return "ENTER";
    case VK_SHIFT:     return "SHIFT";
    case VK_CONTROL:   return "CTRL";
    case VK_MENU:      return "ALT";
    case VK_CAPITAL:   return "CAPS";
    case VK_ESCAPE:    return "ESC";
    case VK_SPACE:     return "SPACE";
    case VK_PRIOR:     return "PAGE UP";
    case VK_NEXT:      return "PAGE DOWN";
    case VK_END:       return "END";
    case VK_HOME:      return "HOME";
    case VK_LEFT:      return "LEFT";
    case VK_UP:        return "UP";
    case VK_RIGHT:     return "RIGHT";
    case VK_DOWN:      return "DOWN";
    case VK_INSERT:    return "INSERT";
    case VK_DELETE:    return "DELETE";
    case VK_LSHIFT:    return "LSHIFT";
    case VK_RSHIFT:    return "RSHIFT";
    case VK_LCONTROL:  return "LCTRL";
    case VK_RCONTROL:  return "RCTRL";
    case VK_LMENU:     return "LALT";
    case VK_RMENU:     return "RALT";
    default:           break;
    }

    if (vk >= VK_F1 && vk <= VK_F24) {
        snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1);
        return buf;
    }
    if (vk >= '0' && vk <= '9') {
        snprintf(buf, sizeof(buf), "%c", static_cast<char>(vk));
        return buf;
    }
    if (vk >= 'A' && vk <= 'Z') {
        snprintf(buf, sizeof(buf), "%c", static_cast<char>(vk));
        return buf;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        snprintf(buf, sizeof(buf), "NUM %d", vk - VK_NUMPAD0);
        return buf;
    }

    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

bool Keybind(const char* label, int* vk) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    ImGui::PushID(label);
    const ImGuiID id = win->GetID("##keybind");

    const float textH = ImGui::GetTextLineHeight();
    const float h = textH > kControlH ? textH : kControlH;
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    const bool listening = (g_capturing == id);
    const char* keyText = listening ? "..." : KeyName(*vk);
    const ImVec2 ks = ImGui::CalcTextSize(keyText);
    const float boxW = (ks.x + 12.f < 46.f) ? 46.f : ks.x + 12.f;

    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    const ImRect box(ImVec2(pos.x + w - boxW, pos.y), ImVec2(pos.x + w, pos.y + h));

    ImGui::ItemSize(ImVec2(w, h), 0.f);
    if (!ImGui::ItemAdd(bb, id)) {
        ImGui::PopID();
        return false;
    }

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(box, id, &hovered, &held,
                                               ImGuiButtonFlags_MouseButtonLeft |
                                                   ImGuiButtonFlags_MouseButtonRight);
    bool changed = false;
    if (pressed) {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            *vk = 0;                      // right click clears the bind
            g_capturing = 0;
            changed = true;
        } else {
            g_capturing = listening ? 0 : id;
        }
    }

    ImDrawList* d = win->DrawList;
    DrawTextAt(d, ImVec2(pos.x, pos.y + (h - textH) * 0.5f), col::kTextDim, label);
    DrawBox(d, box, hovered, listening);
    DrawTextAt(d, ImVec2(box.Min.x + (boxW - ks.x) * 0.5f, box.Min.y + (h - ks.y) * 0.5f),
               listening ? col::kAccent : col::kText, keyText);

    // Capture happens against the OS key state: the bind has to be usable for
    // mouse buttons and modifiers that ImGui's own key list does not surface.
    if (listening) {
        for (int candidate = 0x01; candidate <= 0xFE; ++candidate) {
            if (candidate == VK_LBUTTON)
                continue;   // reserved for clicking the widget itself
            if ((GetAsyncKeyState(candidate) & 0x8000) == 0)
                continue;
            if (candidate == VK_ESCAPE) {
                g_capturing = 0;
                break;
            }
            *vk = candidate;
            g_capturing = 0;
            changed = true;
            break;
        }
    }

    ImGui::PopID();
    return changed;
}

bool ColorPicker(const char* label, ImVec4* color) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    ImGui::PushID(label);
    const ImGuiID id = win->GetID("##color");

    const float textH = ImGui::GetTextLineHeight();
    const float h = textH > 11.f ? textH : 11.f;
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    const float swatchW = 24.f;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    const ImRect swatch(ImVec2(pos.x + w - swatchW, pos.y + (h - 10.f) * 0.5f),
                        ImVec2(pos.x + w, pos.y + (h - 10.f) * 0.5f + 10.f));

    ImGui::ItemSize(ImVec2(w, h), 0.f);
    if (!ImGui::ItemAdd(bb, id)) {
        ImGui::PopID();
        return false;
    }

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(swatch, id, &hovered, &held);

    ImDrawList* d = win->DrawList;
    DrawTextAt(d, ImVec2(pos.x, pos.y + (h - textH) * 0.5f), col::kTextDim, label);

    // Alpha is shown by splitting the swatch: opaque left, real alpha right.
    const ImU32 opaque = ImGui::ColorConvertFloat4ToU32(ImVec4(color->x, color->y, color->z, 1.f));
    const ImU32 actual = ImGui::ColorConvertFloat4ToU32(*color);
    const float mid = (swatch.Min.x + swatch.Max.x) * 0.5f;
    d->AddRectFilled(swatch.Min, ImVec2(mid, swatch.Max.y), opaque);
    d->AddRectFilled(ImVec2(mid, swatch.Min.y), swatch.Max, actual);
    d->AddRect(swatch.Min, swatch.Max, hovered ? col::kAccent : col::kBorder);

    if (pressed)
        ImGui::OpenPopup("##colorpopup");

    bool changed = false;
    if (BeginStyledPopup("##colorpopup", swatch, 0.f)) {
        // Hand-drawn SV square, hue strip and alpha strip. Kept deliberately
        // small: this is a menu accent picker, not an image editor.
        constexpr float kSvW = 118.f, kSvH = 92.f, kStrip = 10.f, kGap = 4.f;

        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(color->x, color->y, color->z, hue, sat, val);

        ImDrawList* pd = ImGui::GetWindowDrawList();

        // --- saturation / value square ---
        const ImVec2 svPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##sv", ImVec2(kSvW, kSvH));
        if (ImGui::IsItemActive()) {
            const ImVec2 m = ImGui::GetIO().MousePos;
            sat = ImClamp((m.x - svPos.x) / kSvW, 0.f, 1.f);
            val = 1.f - ImClamp((m.y - svPos.y) / kSvH, 0.f, 1.f);
            changed = true;
        }

        ImVec4 pure;
        ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, pure.x, pure.y, pure.z);
        const ImU32 pureU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(pure.x, pure.y, pure.z, 1.f));
        const ImVec2 svMax(svPos.x + kSvW, svPos.y + kSvH);

        pd->AddRectFilledMultiColor(svPos, svMax, IM_COL32_WHITE, pureU32, pureU32,
                                    IM_COL32_WHITE);
        pd->AddRectFilledMultiColor(svPos, svMax, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
                                    IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 255));
        pd->AddRect(svPos, svMax, col::kBorder);

        const ImVec2 cursor(svPos.x + sat * kSvW, svPos.y + (1.f - val) * kSvH);
        pd->AddRect(ImVec2(cursor.x - 3.f, cursor.y - 3.f),
                    ImVec2(cursor.x + 3.f, cursor.y + 3.f), IM_COL32_WHITE);

        // --- hue strip ---
        ImGui::Dummy(ImVec2(0.f, kGap));
        const ImVec2 huePos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##hue", ImVec2(kSvW, kStrip));
        if (ImGui::IsItemActive()) {
            hue = ImClamp((ImGui::GetIO().MousePos.x - huePos.x) / kSvW, 0.f, 0.9999f);
            changed = true;
        }
        for (int i = 0; i < 6; ++i) {
            ImVec4 a, b;
            ImGui::ColorConvertHSVtoRGB(i / 6.f, 1.f, 1.f, a.x, a.y, a.z);
            ImGui::ColorConvertHSVtoRGB((i + 1) / 6.f, 1.f, 1.f, b.x, b.y, b.z);
            const ImU32 ca = ImGui::ColorConvertFloat4ToU32(ImVec4(a.x, a.y, a.z, 1.f));
            const ImU32 cb = ImGui::ColorConvertFloat4ToU32(ImVec4(b.x, b.y, b.z, 1.f));
            const float x0 = huePos.x + kSvW * (i / 6.f);
            const float x1 = huePos.x + kSvW * ((i + 1) / 6.f);
            pd->AddRectFilledMultiColor(ImVec2(x0, huePos.y), ImVec2(x1, huePos.y + kStrip),
                                        ca, cb, cb, ca);
        }
        pd->AddRect(huePos, ImVec2(huePos.x + kSvW, huePos.y + kStrip), col::kBorder);
        pd->AddLine(ImVec2(huePos.x + hue * kSvW, huePos.y),
                    ImVec2(huePos.x + hue * kSvW, huePos.y + kStrip), IM_COL32_WHITE);

        // --- alpha strip ---
        ImGui::Dummy(ImVec2(0.f, kGap));
        const ImVec2 alphaPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##alpha", ImVec2(kSvW, kStrip));
        if (ImGui::IsItemActive()) {
            color->w = ImClamp((ImGui::GetIO().MousePos.x - alphaPos.x) / kSvW, 0.f, 1.f);
            changed = true;
        }
        ImVec4 rgb;
        ImGui::ColorConvertHSVtoRGB(hue, sat, val, rgb.x, rgb.y, rgb.z);
        const ImU32 clear = ImGui::ColorConvertFloat4ToU32(ImVec4(rgb.x, rgb.y, rgb.z, 0.f));
        const ImU32 solid = ImGui::ColorConvertFloat4ToU32(ImVec4(rgb.x, rgb.y, rgb.z, 1.f));
        pd->AddRectFilled(alphaPos, ImVec2(alphaPos.x + kSvW, alphaPos.y + kStrip),
                          IM_COL32(40, 40, 40, 255));
        pd->AddRectFilledMultiColor(alphaPos, ImVec2(alphaPos.x + kSvW, alphaPos.y + kStrip),
                                    clear, solid, solid, clear);
        pd->AddRect(alphaPos, ImVec2(alphaPos.x + kSvW, alphaPos.y + kStrip), col::kBorder);
        pd->AddLine(ImVec2(alphaPos.x + color->w * kSvW, alphaPos.y),
                    ImVec2(alphaPos.x + color->w * kSvW, alphaPos.y + kStrip),
                    IM_COL32_WHITE);

        if (changed)
            ImGui::ColorConvertHSVtoRGB(hue, sat, val, color->x, color->y, color->z);

        EndStyledPopup();
    }

    ImGui::PopID();
    return changed;
}

bool Button(const char* label) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float h = kControlH + 2.f;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + RowWidth(), pos.y + h));

    ImGui::ItemSize(ImVec2(bb.GetWidth(), h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* d = win->DrawList;
    DrawBox(d, bb, hovered, held);
    DrawTextAt(d, ImVec2(bb.Min.x + (bb.GetWidth() - ts.x) * 0.5f,
                         bb.Min.y + (h - ts.y) * 0.5f),
               hovered ? col::kAccent : col::kText, label);
    return pressed;
}

} // namespace skeet
