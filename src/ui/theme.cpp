#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <unordered_map>

#include "imgui.h"
#include "imgui_internal.h"

#include "ui/theme.h"

namespace theme {
namespace {

using namespace metric;

// --------------------------------------------------------------- easing ---
//
// Neverlose eases every state change instead of snapping it, so each widget
// needs somewhere to keep its in-flight value between frames. Keyed on the
// ImGui id plus a slot number, because one widget animates several things.

struct AnimKey {
    ImGuiID id;
    int     slot;
    bool operator==(const AnimKey& o) const { return id == o.id && slot == o.slot; }
};

struct AnimHash {
    size_t operator()(const AnimKey& k) const {
        return (static_cast<size_t>(k.id) * 31u) ^ static_cast<size_t>(k.slot);
    }
};

std::unordered_map<AnimKey, float, AnimHash>  g_animFloat;
std::unordered_map<AnimKey, ImVec4, AnimHash> g_animColor;

float Ease(ImGuiID id, int slot, float target, float speed = kAnimSpeed) {
    const float dt = ImGui::GetIO().DeltaTime;
    auto it = g_animFloat.find({ id, slot });
    if (it == g_animFloat.end()) {
        g_animFloat[{ id, slot }] = target;   // first frame lands on the value
        return target;
    }
    it->second = ImLerp(it->second, target, ImSaturate(dt * speed));
    return it->second;
}

ImU32 EaseColor(ImGuiID id, int slot, ImU32 target, float speed = kAnimSpeed) {
    const ImVec4 want = ImGui::ColorConvertU32ToFloat4(target);
    const float dt = ImGui::GetIO().DeltaTime;

    auto it = g_animColor.find({ id, slot });
    if (it == g_animColor.end()) {
        g_animColor[{ id, slot }] = want;
        return target;
    }
    it->second = ImLerp(it->second, want, ImSaturate(dt * speed));
    return ImGui::ColorConvertFloat4ToU32(it->second);
}

// --------------------------------------------------------------- helpers --

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

ImGuiID g_capturing = 0;

struct TabRow {
    ImVec2 min{};
    float  height = 0.f;
    bool   open   = false;
};
TabRow g_tabRow;

void DrawTextAt(ImDrawList* d, const ImVec2& pos, ImU32 color, const char* text) {
    d->AddText(ImVec2(ImFloor(pos.x), ImFloor(pos.y)), color, text);
}

float RowWidth() {
    const float w = ImGui::GetContentRegionAvail().x;
    return w < 24.f ? 24.f : w;
}

void DrawArrow(ImDrawList* d, const ImRect& bb, ImU32 color, float openAmount) {
    const float cx = bb.Max.x - 10.f;
    const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
    // Flips as the popup opens instead of just sitting there.
    const float dir = 1.f - openAmount * 2.f;
    d->AddTriangleFilled(ImVec2(cx - 3.5f, cy - 2.f * dir), ImVec2(cx + 3.5f, cy - 2.f * dir),
                         ImVec2(cx, cy + 2.5f * dir), color);
}

bool BeginStyledPopup(const char* id, const ImRect& anchor, float width) {
    ImGui::SetNextWindowPos(ImVec2(anchor.Min.x, anchor.Max.y + 3.f));
    if (width > 0.f)
        ImGui::SetNextWindowSize(ImVec2(width, 0.f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, kRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(col::kChildBg));
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

bool PopupRow(const char* text, bool selected, bool showTick) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID id = win->GetID(text);

    const float h = ImGui::GetTextLineHeight() + 6.f;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + h));

    ImGui::ItemSize(ImVec2(bb.GetWidth(), h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* d = win->DrawList;
    const ImU32 bg = EaseColor(id, 0, hovered ? col::kFrameActive : col::kControlBg);
    d->AddRectFilled(bb.Min, bb.Max, bg, kRounding);

    float textX = bb.Min.x + 8.f;
    if (showTick) {
        const float s = 8.f;
        const ImVec2 bmin(bb.Min.x + 7.f, (bb.Min.y + bb.Max.y) * 0.5f - s * 0.5f);
        const ImVec2 bmax(bmin.x + s, bmin.y + s);
        const ImU32 fill = EaseColor(id, 1, selected ? col::kAccent : col::kControlBg);
        d->AddRectFilled(bmin, bmax, fill, 2.f);
        d->AddRect(bmin, bmax, selected ? col::kAccent : col::kBorder, 2.f);
        textX = bmax.x + 8.f;
    }

    const ImU32 color = EaseColor(id, 2, selected ? col::kTextActive : col::kText);
    DrawTextAt(d, ImVec2(textX, bb.Min.y + 3.f), color, text);
    return pressed;
}

} // namespace

// ---------------------------------------------------------------- style ---

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 6.f;
    s.ChildRounding     = kRounding;
    s.FrameRounding     = kRounding;
    s.PopupRounding     = kRounding;
    s.GrabRounding      = kRounding;
    s.TabRounding       = kRounding;
    s.ScrollbarRounding = 6.f;

    s.WindowBorderSize = 0.f;   // drawn by hand
    s.ChildBorderSize  = 0.f;
    s.PopupBorderSize  = 1.f;
    s.FrameBorderSize  = 0.f;

    s.WindowPadding    = ImVec2(0.f, 0.f);   // the sidebar owns the left edge
    s.FramePadding     = ImVec2(6.f, 3.f);
    s.ItemSpacing      = ImVec2(6.f, kRowGap);
    s.ItemInnerSpacing = ImVec2(5.f, 3.f);
    s.IndentSpacing    = 12.f;
    s.ScrollbarSize    = 6.f;
    s.GrabMinSize      = 8.f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]      = ImGui::ColorConvertU32ToFloat4(col::kWindowBg);
    c[ImGuiCol_ChildBg]       = ImGui::ColorConvertU32ToFloat4(col::kChildBg);
    c[ImGuiCol_PopupBg]       = ImGui::ColorConvertU32ToFloat4(col::kChildBg);
    c[ImGuiCol_Border]        = ImGui::ColorConvertU32ToFloat4(col::kBorder);
    c[ImGuiCol_Text]          = ImGui::ColorConvertU32ToFloat4(col::kChildText);
    c[ImGuiCol_TextDisabled]  = ImGui::ColorConvertU32ToFloat4(col::kTextDim);
    c[ImGuiCol_FrameBg]       = ImGui::ColorConvertU32ToFloat4(col::kControlBg);
    c[ImGuiCol_ScrollbarBg]   = ImGui::ColorConvertU32ToFloat4(col::kWindowBg);
    c[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(col::kBorderHot);
    c[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(col::kAccent);
    c[ImGuiCol_ScrollbarGrabActive]  = ImGui::ColorConvertU32ToFloat4(col::kAccent);
}

void WindowChrome(const char* title) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetWindowPos();
    const ImVec2 s = ImGui::GetWindowSize();
    const ImVec2 br(p.x + s.x, p.y + s.y);

    // One rounded body, one hairline border, and the sidebar column separated
    // by a single line - neverlose has no nested frames at all.
    d->AddRect(p, br, col::kBorder, 6.f);

    const float sidebar = p.x + kSidebarW;
    d->AddLine(ImVec2(sidebar, p.y + 1.f), ImVec2(sidebar, br.y - 1.f), col::kBorder);

    // Product name at the top of the sidebar, accent dot beside it.
    const ImVec2 ts = ImGui::CalcTextSize(title);
    const ImVec2 at(p.x + 26.f, p.y + 22.f);
    DrawTextAt(d, at, col::kLogo, title);
    d->AddCircleFilled(ImVec2(at.x - 10.f, at.y + ts.y * 0.5f), 3.f, col::kAccent, 12);

    d->AddLine(ImVec2(p.x + 15.f, p.y + 52.f), ImVec2(sidebar - 15.f, p.y + 52.f),
               col::kBorder);
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
        sz.y = storage->GetFloat(id, 60.f);

    const ImVec2 pos = win->DC.CursorPos;

    GroupState& g = g_groups[g_groupDepth++];
    g.min = pos;
    g.max = ImVec2(pos.x + sz.x, pos.y + sz.y);
    g.id = id;
    g.storage = storage;
    g.autoSize = autoSize;

    // Body, hairline, then the header - drawn before the child window opens so
    // nothing inside can land on top of the caption.
    ImDrawList* d = win->DrawList;
    d->AddRectFilled(g.min, g.max, col::kChildBg, kRounding);
    d->AddRect(g.min, g.max, col::kBorder, kRounding);

    DrawTextAt(d, ImVec2(g.min.x + kGroupPadX, g.min.y + 8.f), col::kChildText, label);
    d->AddLine(ImVec2(g.min.x + 1.f, g.min.y + kGroupPadY - 5.f),
               ImVec2(g.max.x - 1.f, g.min.y + kGroupPadY - 5.f), col::kBorder);

    ImGui::SetCursorScreenPos(ImVec2(pos.x + kGroupPadX, pos.y + kGroupPadY));
    ImGui::BeginChild("##content",
                      ImVec2(sz.x - kGroupPadX * 2.f, sz.y - kGroupPadY - 8.f),
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
}

void EndGroupBox() {
    IM_ASSERT(g_groupDepth > 0);

    const float used = ImGui::GetCursorPosY();
    ImGui::EndChild();

    GroupState& g = g_groups[--g_groupDepth];
    if (g.autoSize && g.storage)
        g.storage->SetFloat(g.id, used + kGroupPadY + 6.f);

    ImGui::SetCursorScreenPos(g.min);
    ImGui::ItemSize(ImVec2(g.max.x - g.min.x, g.max.y - g.min.y), 0.f);
    ImGui::PopID();
}

void Separator() {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    ImGui::ItemSize(ImVec2(w, 5.f), 0.f);
    win->DrawList->AddLine(ImVec2(pos.x, pos.y + 2.f), ImVec2(pos.x + w, pos.y + 2.f),
                           col::kBorder);
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
    TextV(col::kChildText, fmt, args);
    va_end(args);
}

void TextDim(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextV(col::kText, fmt, args);
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
    const float y = g_tabRow.min.y + g_tabRow.height;
    const float w = ImGui::GetContentRegionAvail().x;
    win->DrawList->AddLine(ImVec2(g_tabRow.min.x, y), ImVec2(g_tabRow.min.x + w, y),
                           col::kBorder);
    ImGui::SetCursorScreenPos(ImVec2(g_tabRow.min.x, y + 10.f));
}

bool Tab(const char* label, int index, int* current) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + kTabW, pos.y + kTabH));

    ImGui::ItemSize(ImVec2(kTabW, kTabH), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *current = index;

    const bool active = (*current == index);

    // The pill fades in rather than appearing, and it is inset by 15 px on both
    // sides so the sidebar keeps its margin.
    ImVec4 frame = ImGui::ColorConvertU32ToFloat4(col::kTabFrame);
    frame.w = active ? 1.f : (hovered ? 0.35f : 0.f);
    const ImU32 fill = EaseColor(id, 0, ImGui::ColorConvertFloat4ToU32(frame));

    ImDrawList* d = win->DrawList;
    d->AddRectFilled(ImVec2(bb.Min.x + 15.f, bb.Min.y), ImVec2(bb.Max.x - 15.f, bb.Max.y),
                     fill, kRounding);

    const ImU32 text = EaseColor(id, 1, active ? col::kTextActive : col::kText);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    DrawTextAt(d, ImVec2(bb.Min.x + 30.f, bb.Min.y + (kTabH - ts.y) * 0.5f), text, label);

    // Accent marker on the active tab. Neverlose puts an icon glyph here from a
    // font this project does not ship, so the slot carries a bar instead.
    const float mark = Ease(id, 2, active ? 12.f : 0.f);
    if (mark > 0.5f)
        d->AddRectFilled(ImVec2(bb.Min.x + 15.f, (bb.Min.y + bb.Max.y) * 0.5f - mark * 0.5f),
                         ImVec2(bb.Min.x + 17.f, (bb.Min.y + bb.Max.y) * 0.5f + mark * 0.5f),
                         col::kAccent, 1.f);
    return pressed;
}

bool SubTab(const char* label, int index, int* current) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float w = ts.x + 26.f;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + kSubTabRowH));

    ImGui::ItemSize(ImVec2(w, kSubTabRowH), 0.f);
    if (!ImGui::ItemAdd(bb, id)) {
        ImGui::SameLine(0.f, 0.f);
        return false;
    }

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *current = index;

    const bool active = (*current == index);

    ImDrawList* d = win->DrawList;
    const ImU32 text = EaseColor(id, 0,
                                 active ? col::kTextActive : (hovered ? col::kChildText
                                                                      : col::kText));
    DrawTextAt(d, ImVec2(bb.Min.x + 13.f, bb.Min.y + (kSubTabRowH - ts.y) * 0.5f),
               text, label);

    // Underline grows from the centre out.
    const float half = Ease(id, 1, active ? ts.x * 0.5f : 0.f);
    if (half > 0.5f) {
        const float cx = (bb.Min.x + bb.Max.x) * 0.5f;
        d->AddRectFilled(ImVec2(cx - half, bb.Max.y - 2.f), ImVec2(cx + half, bb.Max.y),
                         col::kAccent, 1.f);
    }

    ImGui::SameLine(0.f, 0.f);
    return pressed;
}

// -------------------------------------------------------------- widgets ---

bool Checkbox(const char* label, bool* value) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float h = ts.y > kToggleH ? ts.y : kToggleH;
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));

    ImGui::ItemSize(ImVec2(w, h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) {
        *value = !*value;
        ImGui::MarkItemEdited(id);
    }

    ImDrawList* d = win->DrawList;

    // Label left, switch right - the switch is what makes this neverlose.
    const ImU32 text = EaseColor(id, 0, *value ? col::kChildText : col::kText);
    DrawTextAt(d, ImVec2(pos.x, pos.y + (h - ts.y) * 0.5f), text, label);

    const ImVec2 tmin(pos.x + w - kToggleW, pos.y + (h - kToggleH) * 0.5f);
    const ImVec2 tmax(tmin.x + kToggleW, tmin.y + kToggleH);

    const ImU32 track = EaseColor(id, 1, *value ? col::kFrameActive
                                                : (hovered ? col::kFrameHover
                                                           : col::kControlBg));
    d->AddRectFilled(tmin, tmax, track, kToggleH * 0.5f);
    d->AddRect(tmin, tmax, *value ? col::kAccent : col::kBorder, kToggleH * 0.5f);

    // The knob slides between the two ends instead of jumping.
    const float travel = kToggleW - kKnobR * 2.f - 3.f;
    const float x = Ease(id, 2, *value ? travel : 0.f);
    const ImU32 knob = EaseColor(id, 3, *value ? col::kAccent : col::kAccentDim);
    d->AddCircleFilled(ImVec2(tmin.x + kKnobR + 1.5f + x, (tmin.y + tmax.y) * 0.5f),
                       kKnobR, knob, 16);
    return pressed;
}

namespace {

bool SliderImpl(const char* label, ImGuiDataType type, void* value,
                const void* min, const void* max, const char* format) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const float textH = ImGui::GetTextLineHeight();
    const float h = textH + 5.f + kSliderBar;
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    const ImRect bar(ImVec2(pos.x, pos.y + textH + 5.f),
                     ImVec2(pos.x + w, pos.y + textH + 5.f + kSliderBar));

    ImGui::ItemSize(ImVec2(w, h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    ImRect grab;
    const bool changed = ImGui::SliderBehavior(bar, id, type, value, min, max,
                                               format, ImGuiSliderFlags_None, &grab);
    if (changed)
        ImGui::MarkItemEdited(id);

    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

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
    t = ImSaturate(t);

    char valueText[64];
    ImGui::DataTypeFormatString(valueText, IM_ARRAYSIZE(valueText), type, value, format);

    ImDrawList* d = win->DrawList;
    DrawTextAt(d, pos, col::kText, label);

    const ImVec2 vs = ImGui::CalcTextSize(valueText);
    const ImU32 valueCol = EaseColor(id, 0, (hovered || active) ? col::kTextActive
                                                               : col::kChildText);
    DrawTextAt(d, ImVec2(pos.x + w - vs.x, pos.y), valueCol, valueText);

    const float radius = kSliderBar * 0.5f;
    d->AddRectFilled(bar.Min, bar.Max, col::kControlBg, radius);

    // The fill eases so dragging reads as motion, not as teleporting pixels.
    const float fillT = Ease(id, 1, t, active ? 30.f : kAnimSpeed);
    if (fillT > 0.001f) {
        const float fillX = bar.Min.x + (bar.Max.x - bar.Min.x) * fillT;
        d->AddRectFilled(bar.Min, ImVec2(fillX, bar.Max.y), col::kAccent, radius);
        d->AddCircleFilled(ImVec2(fillX, (bar.Min.y + bar.Max.y) * 0.5f),
                           radius + (hovered ? 2.f : 1.f), col::kAccent, 16);
    }
    d->AddRect(bar.Min, bar.Max, col::kBorder, radius);
    return changed;
}

ImRect LabelledBox(const char* label, float boxHeight, bool* hovered, bool* pressed,
                   ImGuiID id) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const float textH = ImGui::GetTextLineHeight();
    const float w = RowWidth();
    const ImVec2 pos = win->DC.CursorPos;

    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + textH + 4.f + boxHeight));
    const ImRect box(ImVec2(pos.x, pos.y + textH + 4.f),
                     ImVec2(pos.x + w, pos.y + textH + 4.f + boxHeight));

    ImGui::ItemSize(ImVec2(w, bb.GetHeight()), 0.f);
    if (!ImGui::ItemAdd(bb, id)) {
        *hovered = false;
        *pressed = false;
        return box;
    }

    bool held = false;
    *pressed = ImGui::ButtonBehavior(box, id, hovered, &held);
    DrawTextAt(win->DrawList, pos, col::kText, label);
    return box;
}

void DrawControlFrame(ImDrawList* d, const ImRect& box, ImGuiID id, bool hovered,
                      bool open) {
    const ImU32 bg = EaseColor(id, 10, open ? col::kFrameOpen
                                            : (hovered ? col::kFrameHover
                                                       : col::kControlBg));
    d->AddRectFilled(box.Min, box.Max, bg, kRounding);
    d->AddRect(box.Min, box.Max, EaseColor(id, 11, open ? col::kAccent : col::kBorder),
               kRounding);
}

} // namespace

bool SliderFloat(const char* label, float* value, float min, float max,
                 const char* format) {
    return SliderImpl(label, ImGuiDataType_Float, value, &min, &max, format);
}

bool SliderInt(const char* label, int* value, int min, int max, const char* format) {
    return SliderImpl(label, ImGuiDataType_S32, value, &min, &max, format);
}

bool Combo(const char* label, int* value, const char* const items[], int count) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    ImGui::PushID(label);
    const ImGuiID id = win->GetID("##combo");

    bool hovered = false, pressed = false;
    const ImRect box = LabelledBox(label, kControlH, &hovered, &pressed, id);

    const bool popupOpen = ImGui::IsPopupOpen("##combopopup");
    DrawControlFrame(win->DrawList, box, id, hovered, popupOpen);

    const char* preview = (*value >= 0 && *value < count) ? items[*value] : "-";
    const float ty = box.Min.y + (kControlH - ImGui::GetTextLineHeight()) * 0.5f;
    DrawTextAt(win->DrawList, ImVec2(box.Min.x + 9.f, ty), col::kChildText, preview);
    DrawArrow(win->DrawList, box, popupOpen ? col::kAccent : col::kText,
              Ease(id, 12, popupOpen ? 1.f : 0.f));

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
    DrawControlFrame(win->DrawList, box, id, hovered, popupOpen);

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

    const float limit = box.GetWidth() - 26.f;
    if (ImGui::CalcTextSize(preview).x > limit) {
        for (int len = static_cast<int>(strlen(preview)); len > 3; --len) {
            preview[len - 1] = '\0';
            char probe[132];
            snprintf(probe, sizeof(probe), "%s..", preview);
            if (ImGui::CalcTextSize(probe).x <= limit) {
                snprintf(preview + strlen(preview), sizeof(preview) - strlen(preview), "..");
                break;
            }
        }
    }

    const float ty = box.Min.y + (kControlH - ImGui::GetTextLineHeight()) * 0.5f;
    DrawTextAt(win->DrawList, ImVec2(box.Min.x + 9.f, ty),
               enabled ? col::kChildText : col::kText, preview);
    DrawArrow(win->DrawList, box, popupOpen ? col::kAccent : col::kText,
              Ease(id, 12, popupOpen ? 1.f : 0.f));

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
    case 0:            return "none";
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
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
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
    const float boxW = (ks.x + 18.f < 58.f) ? 58.f : ks.x + 18.f;

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
            *vk = 0;
            g_capturing = 0;
            changed = true;
        } else {
            g_capturing = listening ? 0 : id;
        }
    }

    ImDrawList* d = win->DrawList;
    DrawTextAt(d, ImVec2(pos.x, pos.y + (h - textH) * 0.5f), col::kText, label);
    DrawControlFrame(d, box, id, hovered, listening);
    DrawTextAt(d, ImVec2(box.Min.x + (boxW - ks.x) * 0.5f, box.Min.y + (h - ks.y) * 0.5f),
               listening ? col::kAccent : col::kChildText, keyText);

    // Captured against the OS key state so mouse buttons and modifiers work.
    if (listening) {
        for (int candidate = 0x01; candidate <= 0xFE; ++candidate) {
            if (candidate == VK_LBUTTON)
                continue;
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
    const float h = textH > 14.f ? textH : 14.f;
    const ImVec2 pos = win->DC.CursorPos;
    const float w = RowWidth();

    const float swatchW = 30.f;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    const ImRect swatch(ImVec2(pos.x + w - swatchW, pos.y + (h - 12.f) * 0.5f),
                        ImVec2(pos.x + w, pos.y + (h - 12.f) * 0.5f + 12.f));

    ImGui::ItemSize(ImVec2(w, h), 0.f);
    if (!ImGui::ItemAdd(bb, id)) {
        ImGui::PopID();
        return false;
    }

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(swatch, id, &hovered, &held);

    ImDrawList* d = win->DrawList;
    DrawTextAt(d, ImVec2(pos.x, pos.y + (h - textH) * 0.5f), col::kText, label);

    const ImU32 opaque = ImGui::ColorConvertFloat4ToU32(ImVec4(color->x, color->y, color->z, 1.f));
    const ImU32 actual = ImGui::ColorConvertFloat4ToU32(*color);
    const float mid = (swatch.Min.x + swatch.Max.x) * 0.5f;
    d->AddRectFilled(swatch.Min, ImVec2(mid, swatch.Max.y), opaque, 3.f,
                     ImDrawFlags_RoundCornersLeft);
    d->AddRectFilled(ImVec2(mid, swatch.Min.y), swatch.Max, actual, 3.f,
                     ImDrawFlags_RoundCornersRight);
    d->AddRect(swatch.Min, swatch.Max, hovered ? col::kAccent : col::kBorder, 3.f);

    if (pressed)
        ImGui::OpenPopup("##colorpopup");

    bool changed = false;
    if (BeginStyledPopup("##colorpopup", swatch, 0.f)) {
        constexpr float kSvW = 132.f, kSvH = 100.f, kStrip = 11.f, kGap = 5.f;

        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(color->x, color->y, color->z, hue, sat, val);

        ImDrawList* pd = ImGui::GetWindowDrawList();

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
        pd->AddCircle(cursor, 4.f, IM_COL32_WHITE, 12, 1.5f);

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
        pd->AddRect(huePos, ImVec2(huePos.x + kSvW, huePos.y + kStrip), col::kBorder, 3.f);
        pd->AddCircleFilled(ImVec2(huePos.x + hue * kSvW, huePos.y + kStrip * 0.5f), 4.f,
                            IM_COL32_WHITE, 12);

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
                          IM_COL32(40, 40, 40, 255), 3.f);
        pd->AddRectFilledMultiColor(alphaPos, ImVec2(alphaPos.x + kSvW, alphaPos.y + kStrip),
                                    clear, solid, solid, clear);
        pd->AddRect(alphaPos, ImVec2(alphaPos.x + kSvW, alphaPos.y + kStrip), col::kBorder, 3.f);
        pd->AddCircleFilled(ImVec2(alphaPos.x + color->w * kSvW, alphaPos.y + kStrip * 0.5f),
                            4.f, IM_COL32_WHITE, 12);

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
    const float h = kControlH + 3.f;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + RowWidth(), pos.y + h));

    ImGui::ItemSize(ImVec2(bb.GetWidth(), h), 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* d = win->DrawList;
    const ImU32 bg = EaseColor(id, 0, held ? col::kFrameActive
                                           : (hovered ? col::kFrameHover
                                                      : col::kControlBg));
    d->AddRectFilled(bb.Min, bb.Max, bg, kRounding);
    d->AddRect(bb.Min, bb.Max, EaseColor(id, 1, hovered ? col::kAccent : col::kBorder),
               kRounding);
    DrawTextAt(d, ImVec2(bb.Min.x + (bb.GetWidth() - ts.x) * 0.5f,
                         bb.Min.y + (h - ts.y) * 0.5f),
               EaseColor(id, 2, hovered ? col::kTextActive : col::kChildText), label);
    return pressed;
}

} // namespace theme
