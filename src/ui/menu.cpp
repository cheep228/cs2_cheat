#include <cstdio>

#include "imgui.h"

#include "core/globals.h"
#include "ui/menu.h"
#include "ui/tabs.h"
#include "ui/theme.h"

// Window chrome and tab dispatch. Everything visual lives in ui/theme.cpp and
// every control lives in ui/tab_*.cpp; this file is the seam between them and
// should stay small enough to read in one go.
//
// Layout follows neverlose: a fixed sidebar on the left carries the main tabs,
// the content area on the right carries a sub-tab row and the groupboxes.
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
    const ImVec2 pos(14.f, 14.f);
    const ImVec2 topLeft(pos.x - 8.f, pos.y - 5.f);
    const ImVec2 bottomRight(pos.x + size.x + 8.f, pos.y + size.y + 5.f);

    d->AddRectFilled(topLeft, bottomRight, theme::col::kChildBg, 4.f);
    d->AddRect(topLeft, bottomRight, theme::col::kBorder, 4.f);
    d->AddRectFilled(topLeft, ImVec2(bottomRight.x, topLeft.y + 2.f),
                     theme::col::kAccent, 2.f, ImDrawFlags_RoundCornersTop);
    d->AddText(pos, theme::col::kChildText, buf);
}

} // namespace

namespace menu {

void ApplyStyle() {
    theme::ApplyStyle();
}

void Render() {
    DrawWatermark();

    if (!g::menuOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(800.f, 620.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(cfg::c.menuAlpha);

    // No ImGui title bar: the sidebar header is the title, and the window still
    // drags from any empty spot.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##4fir", nullptr, flags)) {
        ImGui::End();
        return;
    }

    theme::WindowChrome("4fir");

    // ---- sidebar ----
    ImGui::SetCursorPos(ImVec2(10.f, 68.f));
    ImGui::BeginGroup();
    for (int i = 0; i < kTabCount; ++i)
        theme::Tab(kTabs[i].name, i, &g_tab);
    ImGui::EndGroup();

    // ---- content ----
    const ui::SubTabs subs = kTabs[g_tab].subs();
    if (g_sub[g_tab] >= subs.count)
        g_sub[g_tab] = 0;

    ImGui::SetCursorPos(ImVec2(theme::metric::kSidebarW + 16.f, 16.f));
    ImGui::BeginGroup();

    theme::BeginTabRow(theme::metric::kSubTabRowH);
    for (int i = 0; i < subs.count; ++i)
        theme::SubTab(subs.names[i], i, &g_sub[g_tab]);
    theme::EndTabRow();

    // The content scrolls on its own so the sidebar and sub-tabs never move.
    ImGui::BeginChild("##content", ImVec2(-16.f, -16.f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground);
    kTabs[g_tab].draw(g_sub[g_tab]);
    ImGui::EndChild();

    ImGui::EndGroup();
    ImGui::End();
}

} // namespace menu
