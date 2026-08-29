#include <cstdio>

#include "imgui.h"

#include "core/globals.h"
#include "ui/menu.h"
#include "ui/skeet.h"
#include "ui/tabs.h"

// Window chrome and tab dispatch. Everything visual lives in ui/skeet.cpp and
// every control lives in ui/tab_*.cpp; this file is the seam between them and
// should stay small enough to read in one go.
namespace {

struct TabDef {
    const char* name;
    ui::SubTabs (*subs)();
    void (*draw)(int sub);
};

const TabDef kTabs[] = {
    { "RAGE",    ui::RageSubs,    ui::RageDraw    },
    { "LEGIT",   ui::LegitSubs,   ui::LegitDraw   },
    { "VISUALS", ui::VisualsSubs, ui::VisualsDraw },
    { "MISC",    ui::MiscSubs,    ui::MiscDraw    },
    { "SKINS",   ui::SkinsSubs,   ui::SkinsDraw   },
    { "CONFIG",  ui::ConfigSubs,  ui::ConfigDraw  },
};

constexpr int kTabCount = IM_ARRAYSIZE(kTabs);

int g_tab = 0;
int g_sub[kTabCount] = {};

void DrawWatermark() {
    if (!cfg::c.watermark && !cfg::c.showFps)
        return;

    char buf[96];
    if (cfg::c.watermark && cfg::c.showFps)
        snprintf(buf, sizeof(buf), "4fir  |  %.0f fps", ImGui::GetIO().Framerate);
    else if (cfg::c.watermark)
        snprintf(buf, sizeof(buf), "4fir");
    else
        snprintf(buf, sizeof(buf), "%.0f fps", ImGui::GetIO().Framerate);

    ImDrawList* d = ImGui::GetForegroundDrawList();
    const ImVec2 size = ImGui::CalcTextSize(buf);
    const ImVec2 pos(12.f, 12.f);
    const ImVec2 topLeft(pos.x - 6.f, pos.y - 3.f);
    const ImVec2 bottomRight(pos.x + size.x + 6.f, pos.y + size.y + 3.f);

    d->AddRectFilled(topLeft, bottomRight, skeet::col::kWindowBg);
    d->AddRect(topLeft, bottomRight, skeet::col::kBorder);
    d->AddLine(topLeft, ImVec2(bottomRight.x, topLeft.y), skeet::col::kAccent);
    d->AddText(pos, skeet::col::kText, buf);
}

} // namespace

namespace menu {

void ApplyStyle() {
    skeet::ApplyStyle();
}

void Render() {
    DrawWatermark();

    if (!g::menuOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(658.f, 436.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(cfg::c.menuAlpha);

    // No ImGui title bar: the strip drawn by WindowChrome is the title, and the
    // window still drags from any empty spot.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##4fir", nullptr, flags)) {
        ImGui::End();
        return;
    }

    skeet::WindowChrome("4fir");

    // Below the title strip, inset by the double border.
    ImGui::SetCursorPos(ImVec2(9.f, 25.f));

    skeet::BeginTabRow(skeet::metric::kTabRowH);
    for (int i = 0; i < kTabCount; ++i)
        skeet::Tab(kTabs[i].name, i, &g_tab);
    skeet::EndTabRow();

    const ui::SubTabs subs = kTabs[g_tab].subs();

    if (g_sub[g_tab] >= subs.count)
        g_sub[g_tab] = 0;

    skeet::BeginTabRow(skeet::metric::kSubTabRowH);
    for (int i = 0; i < subs.count; ++i)
        skeet::SubTab(subs.names[i], i, &g_sub[g_tab]);
    skeet::EndTabRow();

    // Content scrolls on its own so the tab strips never move.
    ImGui::BeginChild("##content", ImVec2(-9.f, -6.f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground);
    kTabs[g_tab].draw(g_sub[g_tab]);
    ImGui::EndChild();

    ImGui::End();
}

} // namespace menu
