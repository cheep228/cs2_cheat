#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include "core/globals.h"
#include "ui/menu.h"
#include "ui/tabs.h"
#include "ui/theme.h"

namespace {

struct TabDef {
    const char* label;
    const char* icon;
    const char* section;
    ui::SubTabs (*subs)();
    void (*draw)(int sub);
};

// The provided neverlose source uses glyphs A..L from neverlose-v2-icons.ttf.
// We keep the same glyph slots. If that font is absent, menu::ApplyStyle()
// leaves the icon strings visually empty instead of rendering literal letters.
const TabDef kTabs[] = {
    { "Legitbot", "C", "Aimbot",        ui::LegitSubs,   ui::LegitDraw   },
    { "Visuals",  "D", "Visuals",       ui::VisualsSubs, ui::VisualsDraw },
    { "Misc",     "I", "Miscellaneous", ui::MiscSubs,    ui::MiscDraw    },
    { "Skins",    "J", "Miscellaneous", ui::SkinsSubs,   ui::SkinsDraw   },
    { "Configs",  "L", "Miscellaneous", ui::ConfigSubs,  ui::ConfigDraw  },
};

constexpr int kTabCount = IM_ARRAYSIZE(kTabs);
int g_tab = 0;
int g_sub[kTabCount] = {};
bool g_hasOriginalIconFont = false;

bool FileExists(const std::string& path) {
    const DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::string ModuleDirectory() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(&ModuleDirectory), &module))
        return {};

    char path[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(module, path, MAX_PATH);
    if (!n || n >= MAX_PATH)
        return {};

    std::string out(path, n);
    const size_t slash = out.find_last_of("\\/");
    if (slash != std::string::npos)
        out.resize(slash);
    return out;
}

std::string FindFont(const char* filename) {
    const std::string moduleDir = ModuleDirectory();
    const std::vector<std::string> candidates = {
        moduleDir.empty() ? std::string() : moduleDir + "\\fonts\\" + filename,
        moduleDir.empty() ? std::string() : moduleDir + "\\assets\\fonts\\" + filename,
        moduleDir.empty() ? std::string() : moduleDir + "\\..\\assets\\fonts\\" + filename,
        std::string("assets\\fonts\\") + filename,
        std::string("fonts\\") + filename,
        std::string("C:\\NEVERLOSE\\Fonts\\") + filename,
    };

    for (const std::string& p : candidates)
        if (!p.empty() && FileExists(p))
            return p;
    return {};
}

enum class FallbackWeight {
    Regular,
    SemiBold,
    Bold,
};

ImFont* AddFallbackFont(float sizePx, FallbackWeight weight) {
    ImGuiIO& io = ImGui::GetIO();

    // Clean Windows-native fallback. This keeps the project self-contained
    // without bundling/copying font files. Segoe UI is available on normal
    // Windows 10/11 installations and visually fits this compact menu well.
    const char* primary = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* secondary = "C:\\Windows\\Fonts\\arial.ttf";

    switch (weight) {
        case FallbackWeight::SemiBold:
            primary = "C:\\Windows\\Fonts\\seguisb.ttf";
            secondary = "C:\\Windows\\Fonts\\arialbd.ttf";
            break;
        case FallbackWeight::Bold:
            primary = "C:\\Windows\\Fonts\\segoeuib.ttf";
            secondary = "C:\\Windows\\Fonts\\arialbd.ttf";
            break;
        case FallbackWeight::Regular:
        default:
            break;
    }

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;

    const char* systemCandidates[] = { primary, secondary };
    for (const char* fontPath : systemCandidates)
        if (FileExists(fontPath))
            if (ImFont* f = io.Fonts->AddFontFromFileTTF(fontPath, sizePx, &cfg))
                return f;

    cfg.SizePixels = sizePx;
    return io.Fonts->AddFontDefault(&cfg);
}

ImFont* AddMenuFont(const char* filename, float sizePx, FallbackWeight weight, bool* exact = nullptr) {
    if (exact)
        *exact = false;

    ImGuiIO& io = ImGui::GetIO();
    const std::string path = FindFont(filename);
    if (!path.empty()) {
        ImFontConfig cfg;
        cfg.OversampleH = 1;
        cfg.OversampleV = 1;
        cfg.FontBuilderFlags = 1; // matches the supplied source config
        if (ImFont* f = io.Fonts->AddFontFromFileTTF(path.c_str(), sizePx, &cfg)) {
            if (exact)
                *exact = true;
            return f;
        }
    }
    return AddFallbackFont(sizePx, weight);
}

void LoadNeverloseFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Exact slot order from hooks/hooks.cpp in the supplied Neverlose source.
    AddMenuFont("museosanscyrl-500.ttf", 16.0f * io.GUIScale, FallbackWeight::Regular);  // Fonts[0]
    AddMenuFont("museosanscyrl-900.ttf", 30.0f * io.GUIScale, FallbackWeight::Bold);     // Fonts[1]
    AddMenuFont("museosanscyrl-700.ttf", 16.0f * io.GUIScale, FallbackWeight::SemiBold); // Fonts[2]
    AddMenuFont("museosanscyrl-500.ttf", 11.0f * io.GUIScale, FallbackWeight::Regular);  // Fonts[3]

    bool icon18Exact = false;
    bool icon12Exact = false;
    AddMenuFont("neverlose-v2-icons.ttf", 18.0f * io.GUIScale, FallbackWeight::Regular, &icon18Exact); // Fonts[4]
    AddMenuFont("neverlose-v2-icons.ttf", 12.0f * io.GUIScale, FallbackWeight::Regular, &icon12Exact); // Fonts[5]
    g_hasOriginalIconFont = icon18Exact && icon12Exact;

    if (!io.Fonts->Fonts.empty())
        io.FontDefault = io.Fonts->Fonts[0];
}

ImVec4 TargetColor(int r, int g, int b, float alphaMul = 1.0f) {
    const float a = ImGui::GetStyle().Alpha * alphaMul;
    return ImColor(r, g, b, static_cast<int>(255.0f * a)).Value;
}

void UpdateNeverloseColors() {
    ImGuiIO& io = ImGui::GetIO();
    const float fast = io.DeltaTime * 12.0f * io.AnimationSpeed;
    const float accent = io.DeltaTime * 6.0f;

    // Exact blue-theme targets from cheats/menu.cpp in the supplied source.
    Colors::LeftBar        = ImLerp(Colors::LeftBar,        TargetColor(3, 30, 50),    fast);
    Colors::Background     = ImLerp(Colors::Background,     TargetColor(9, 8, 13),     fast);
    Colors::Child          = ImLerp(Colors::Child,          TargetColor(1, 11, 21),    fast);
    Colors::ChildText      = ImLerp(Colors::ChildText,      TargetColor(200, 203, 208),fast);
    Colors::Borders        = ImLerp(Colors::Borders,        TargetColor(12, 33, 52),   fast);
    Colors::BordersActive  = ImLerp(Colors::BordersActive,  TargetColor(4, 25, 40),    fast);
    Colors::TabFrame       = ImLerp(Colors::TabFrame,       TargetColor(1, 72, 104),   fast);
    Colors::TabText        = ImLerp(Colors::TabText,        TargetColor(255, 255, 255),fast);
    Colors::Logo           = ImLerp(Colors::Logo,           TargetColor(248, 255, 255),fast);
    Colors::Text           = ImLerp(Colors::Text,           TargetColor(162, 176, 185),fast);
    Colors::TextActive     = ImLerp(Colors::TextActive,     TargetColor(255, 255, 255),fast);
    Colors::Accent         = ImLerp(Colors::Accent,         TargetColor(0, 165, 243),  accent);
    Colors::AccentDisabled = ImLerp(Colors::AccentDisabled, TargetColor(76, 90, 101), fast);
    Colors::Frame          = ImLerp(Colors::Frame,          TargetColor(2, 5, 12),     fast);
    Colors::FrameDisabled  = ImLerp(Colors::FrameDisabled,  TargetColor(8, 9, 13),     fast);
    Colors::FrameHovered   = ImLerp(Colors::FrameHovered,   TargetColor(2, 5, 12),     fast);
    Colors::FrameActive    = ImLerp(Colors::FrameActive,    TargetColor(2, 18, 33),    fast);
    Colors::FrameOpened    = ImLerp(Colors::FrameOpened,    TargetColor(2, 18, 33),    fast);

    Colors::theme = 0;
    Colors::username = "4fir";
}

void DrawSectionTitle(const char* title) {
    ImGui::SetCursorPosX(25.0f * ImGui::GetIO().GUIScale);
    ImGui::TextColored(Colors::Text, "%s", title);
}

void DrawSidebar() {
    ImGuiIO& io = ImGui::GetIO();
    const float sc = io.GUIScale;

    ImGui::SetCursorPos(ImVec2(0.0f, 62.0f * sc));
    ImGui::BeginGroup();

    const char* currentSection = nullptr;
    for (int i = 0; i < kTabCount; ++i) {
        if (!currentSection || std::strcmp(currentSection, kTabs[i].section) != 0) {
            if (currentSection) {
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
            }
            currentSection = kTabs[i].section;
            DrawSectionTitle(currentSection);
        }

        const char* icon = g_hasOriginalIconFont ? kTabs[i].icon : "";
        if (ImGui::TabButton(icon, kTabs[i].label, g_tab == i))
            g_tab = i;
    }

    ImGui::EndGroup();
}

void DrawHeaderSubTabs() {
    const ui::SubTabs subs = kTabs[g_tab].subs();
    if (subs.count <= 0)
        return;
    if (g_sub[g_tab] < 0 || g_sub[g_tab] >= subs.count)
        g_sub[g_tab] = 0;

    const float sc = ImGui::GetIO().GUIScale;
    ImGui::PushID(g_tab);
    ImGui::SetCursorPos(ImVec2(200.0f * sc, 11.0f * sc));
    ImGui::BeginGroup();
    for (int i = 0; i < subs.count; ++i) {
        if (i)
            ImGui::SameLine(0.0f, 4.0f * sc);
        theme::SubTab(subs.names[i], i, &g_sub[g_tab]);
    }
    ImGui::EndGroup();
    ImGui::PopID();
}

void DrawWatermark() {
    if (!cfg::c.watermark && !cfg::c.showFps)
        return;

    char buf[96];
    if (cfg::c.watermark && cfg::c.showFps)
        std::snprintf(buf, sizeof(buf), "4fir  |  %.0f fps", ImGui::GetIO().Framerate);
    else if (cfg::c.watermark)
        std::snprintf(buf, sizeof(buf), "4fir");
    else
        std::snprintf(buf, sizeof(buf), "%.0f fps", ImGui::GetIO().Framerate);

    ImDrawList* d = ImGui::GetForegroundDrawList();
    const ImVec2 size = ImGui::CalcTextSize(buf);
    const ImVec2 pos(14.f, 14.f);
    const ImVec2 a(pos.x - 8.f, pos.y - 5.f);
    const ImVec2 b(pos.x + size.x + 8.f, pos.y + size.y + 5.f);
    d->AddRectFilled(a, b, ImColor(Colors::Child), 4.f);
    d->AddRect(a, b, ImColor(Colors::Borders), 4.f);
    d->AddRectFilled(a, ImVec2(b.x, a.y + 2.f), ImColor(Colors::Accent), 2.f,
                     ImDrawCornerFlags_Top);
    d->AddText(pos, ImColor(Colors::ChildText), buf);
}

} // namespace

namespace menu {

void ApplyStyle() {
    ImGuiIO& io = ImGui::GetIO();
    io.GUIScale = 1.0f;
    io.AnimationSpeed = 1.0f;

    // The original GUI init did not restyle ImGui; it relied on the defaults
    // of this exact customized 1.87 fork plus Colors::* above.
    LoadNeverloseFonts();
}

void Render() {
    // Preserve the project's opacity setting by feeding it through ImGui's
    // global alpha, which is exactly what the source color formulas read.
    ImGui::GetStyle().Alpha = ImClamp(cfg::c.menuAlpha, 0.05f, 1.0f);
    UpdateNeverloseColors();
    DrawWatermark();

    if (!g::menuOpen)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const float sc = io.GUIScale;

    ImGui::SetNextWindowSize(ImVec2(800.0f * sc, 620.0f * sc));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::MainBegin("Menu", nullptr, flags)) {
        DrawSidebar();
        DrawHeaderSubTabs();

        // Exact Neverlose content origin and child dimensions.
        ImGui::SetCursorPos(ImVec2(180.0f * sc, 50.0f * sc));
        ImGui::BeginChild("content", ImVec2(1110.0f, 1110.0f));

        // theme::BeginGroupBox places a 30px caption spacer before MenuChild;
        // starting at y=18 therefore puts the real MenuChild body at y=48,
        // exactly matching the supplied source geometry.
        ImGui::SetCursorPos(ImVec2(18.0f * sc, 18.0f * sc));
        ImGui::PushID(g_tab);
        ImGui::PushID(g_sub[g_tab]);
        kTabs[g_tab].draw(g_sub[g_tab]);
        ImGui::PopID();
        ImGui::PopID();

        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace menu
