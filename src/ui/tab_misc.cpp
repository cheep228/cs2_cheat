#include "core/globals.h"
#include "core/log.h"
#include "features/esp.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/gameinfo.h"
#include "sdk/offsets.h"
#include "sdk/resolve.h"
#include "sdk/trace.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "GENERAL", "DIAGNOSTICS" };
const char* const kLogLevels[] = { "Trace", "Info", "Ok", "Warn", "Error" };

void DrawGeneral() {
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Overlay", ImVec2(w, 0.f));
    theme::Checkbox("Master switch", &cfg::c.masterSwitch);
    theme::TextDim("Turns every visual off at once.");
    theme::Checkbox("Watermark", &cfg::c.watermark);
    theme::Checkbox("Show FPS", &cfg::c.showFps);
    theme::SliderFloat("Menu opacity", &cfg::c.menuAlpha, 0.35f, 1.f, "%.2f");
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Keys", ImVec2(w, 0.f));
    theme::Keybind("Menu key", &cfg::c.menuKey);
    theme::TextDim("Also settable from the loader; the");
    theme::TextDim("loader's value is read at inject time.");
    theme::Separator();
    theme::TextDim("END always unloads the module.");
    theme::EndGroupBox();

    theme::BeginGroupBox("Debug console", ImVec2(w, 0.f));
    if (theme::Checkbox("Show console window", &cfg::c.logConsole))
        dbg::SetVisible(cfg::c.logConsole);
    theme::TextDim("Never close it with the X - that kills");
    theme::TextDim("the game process.");
    theme::Checkbox("Verbose frame dump", &cfg::c.logVerbose);
    if (theme::Combo("Minimum level", &cfg::c.logLevel, kLogLevels,
                     IM_ARRAYSIZE(kLogLevels)))
        dbg::SetMinLevel(static_cast<dbg::Level>(cfg::c.logLevel));
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Module", ImVec2(w, 0.f));
    theme::Text("Built %s %s", __DATE__, __TIME__);
    theme::Text("ImGui %s", IMGUI_VERSION);
    theme::Text("Frame %.2f ms (%.0f fps)", 1000.f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    theme::Separator();
    if (theme::Button("Unload module"))
        cfg::c.unloadQueued = true;
    theme::EndGroupBox();
}

void DrawDiagnostics() {
    const float w = theme::ColumnWidth(2);
    const esp::Stats& st = esp::stats;

    auto status = [](const char* label, bool ok) {
        theme::TextColored(ok ? theme::col::kGood : theme::col::kBad, "%s %s", label,
                           ok ? "ok" : "FAIL");
    };

    theme::BeginGroupBox("Reads", ImVec2(w, 0.f));
    status("client.dll base ", g::client != 0);
    status("entity list     ", st.entityListOk);
    status("local pawn      ", st.localPawnOk);
    status("view matrix     ", st.viewMatrixOk);
    theme::Separator();
    theme::Text("players found: %d (humans %d / bots %d)", st.slotsScanned, st.humans,
                st.bots);
    theme::Text("passed filters: %d", st.drawn);
    theme::Text("local team: %d   %dx%d", st.localTeam, g::screenW, g::screenH);
    theme::Text("faulting reads: %lu", mem::faults);
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Offsets", ImVec2(w, 0.f));
    theme::Text("engine build %d, baked dump %d", gameinfo::BuildNumber(),
                offsets::kDumpBuild);
    theme::Text("entity list: chunk 0x%X stride 0x%X",
                static_cast<unsigned>(entlist::layout.chunkArray),
                static_cast<unsigned>(entlist::layout.stride));
    theme::TextColored(entlist::layout.calibrated ? theme::col::kGood : theme::col::kBad,
                       entlist::layout.calibrated ? "calibrated" : "NOT calibrated");

    const resolve::Result& rr = resolve::Last();
    theme::Text("globals %d/%d, fields %d/%d", rr.globalsFound, rr.globalsTotal,
                rr.fieldsFound, rr.fieldsTotal);
    theme::Text("schema classes: %d", rr.schemaClasses);
    if (rr.fieldsFound == rr.fieldsTotal && rr.fieldsTotal > 0)
        theme::TextColored(theme::col::kGood, "offsets came from the running game");
    else
        theme::TextColored(theme::col::kBad, "live resolution incomplete");
    theme::EndGroupBox();

    theme::BeginGroupBox("Missing systems", ImVec2(w, 0.f));
    theme::TextDim("Written down so nothing here pretends");
    theme::TextDim("to work when it does not:");
    theme::Separator();
    theme::TextColored(trace::Available() ? theme::col::kGood : theme::col::kBad,
                       "ray trace / visibility: %s",
                       trace::Available() ? "available" : "absent");
    theme::TextDim("sdk/trace.h - ragebot points are scored");
    theme::TextDim("without a visibility bonus.");
    theme::Separator();
    theme::TextColored(theme::col::kBad, "usercmd control: absent");
    theme::TextDim("No anti-aim, no fake lag, no tickbase.");
    theme::Separator();
    theme::TextColored(theme::col::kBad, "econ / item schema: absent");
    theme::TextDim("No skin changer - see the SKINS tab.");
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Log", ImVec2(w, 0.f));
    theme::Text("info  %d", dbg::Count(dbg::Level::Info));
    theme::Text("warn  %d", dbg::Count(dbg::Level::Warn));
    theme::Text("error %d", dbg::Count(dbg::Level::Error));
    theme::TextDim("%s", dbg::FilePath());
    theme::Separator();
    if (theme::Button("Dump entities to console (F9)"))
        esp::Dump();
    theme::EndGroupBox();
}

} // namespace

SubTabs MiscSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void MiscDraw(int sub) {
    if (sub == 0)
        DrawGeneral();
    else
        DrawDiagnostics();
}

} // namespace ui
