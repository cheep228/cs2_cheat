#include "core/globals.h"
#include "core/log.h"
#include "features/esp.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/gameinfo.h"
#include "sdk/offsets.h"
#include "sdk/resolve.h"
#include "sdk/trace.h"
#include "ui/skeet.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "GENERAL", "DIAGNOSTICS" };
const char* const kLogLevels[] = { "Trace", "Info", "Ok", "Warn", "Error" };

void DrawGeneral() {
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("Overlay", ImVec2(w, 0.f));
    skeet::Checkbox("Master switch", &cfg::c.masterSwitch);
    skeet::TextDim("Turns every visual off at once.");
    skeet::Checkbox("Watermark", &cfg::c.watermark);
    skeet::Checkbox("Show FPS", &cfg::c.showFps);
    skeet::SliderFloat("Menu opacity", &cfg::c.menuAlpha, 0.35f, 1.f, "%.2f");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Keys", ImVec2(w, 0.f));
    skeet::Keybind("Menu key", &cfg::c.menuKey);
    skeet::TextDim("Also settable from the loader; the");
    skeet::TextDim("loader's value is read at inject time.");
    skeet::Separator();
    skeet::TextDim("END always unloads the module.");
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Debug console", ImVec2(w, 0.f));
    if (skeet::Checkbox("Show console window", &cfg::c.logConsole))
        dbg::SetVisible(cfg::c.logConsole);
    skeet::TextDim("Never close it with the X - that kills");
    skeet::TextDim("the game process.");
    skeet::Checkbox("Verbose frame dump", &cfg::c.logVerbose);
    if (skeet::Combo("Minimum level", &cfg::c.logLevel, kLogLevels,
                     IM_ARRAYSIZE(kLogLevels)))
        dbg::SetMinLevel(static_cast<dbg::Level>(cfg::c.logLevel));
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Module", ImVec2(w, 0.f));
    skeet::Text("Built %s %s", __DATE__, __TIME__);
    skeet::Text("ImGui %s", IMGUI_VERSION);
    skeet::Text("Frame %.2f ms (%.0f fps)", 1000.f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    skeet::Separator();
    if (skeet::Button("Unload module"))
        cfg::c.unloadQueued = true;
    skeet::EndGroupBox();
}

void DrawDiagnostics() {
    const float w = skeet::ColumnWidth(2);
    const esp::Stats& st = esp::stats;

    auto status = [](const char* label, bool ok) {
        skeet::TextColored(ok ? skeet::col::kGood : skeet::col::kBad, "%s %s", label,
                           ok ? "ok" : "FAIL");
    };

    skeet::BeginGroupBox("Reads", ImVec2(w, 0.f));
    status("client.dll base ", g::client != 0);
    status("entity list     ", st.entityListOk);
    status("local pawn      ", st.localPawnOk);
    status("view matrix     ", st.viewMatrixOk);
    skeet::Separator();
    skeet::Text("players found: %d (humans %d / bots %d)", st.slotsScanned, st.humans,
                st.bots);
    skeet::Text("passed filters: %d", st.drawn);
    skeet::Text("local team: %d   %dx%d", st.localTeam, g::screenW, g::screenH);
    skeet::Text("faulting reads: %lu", mem::faults);
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Offsets", ImVec2(w, 0.f));
    skeet::Text("engine build %d, baked dump %d", gameinfo::BuildNumber(),
                offsets::kDumpBuild);
    skeet::Text("entity list: chunk 0x%X stride 0x%X",
                static_cast<unsigned>(entlist::layout.chunkArray),
                static_cast<unsigned>(entlist::layout.stride));
    skeet::TextColored(entlist::layout.calibrated ? skeet::col::kGood : skeet::col::kBad,
                       entlist::layout.calibrated ? "calibrated" : "NOT calibrated");

    const resolve::Result& rr = resolve::Last();
    skeet::Text("globals %d/%d, fields %d/%d", rr.globalsFound, rr.globalsTotal,
                rr.fieldsFound, rr.fieldsTotal);
    skeet::Text("schema classes: %d", rr.schemaClasses);
    if (rr.fieldsFound == rr.fieldsTotal && rr.fieldsTotal > 0)
        skeet::TextColored(skeet::col::kGood, "offsets came from the running game");
    else
        skeet::TextColored(skeet::col::kBad, "live resolution incomplete");
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Missing systems", ImVec2(w, 0.f));
    skeet::TextDim("Written down so nothing here pretends");
    skeet::TextDim("to work when it does not:");
    skeet::Separator();
    skeet::TextColored(trace::Available() ? skeet::col::kGood : skeet::col::kBad,
                       "ray trace / visibility: %s",
                       trace::Available() ? "available" : "absent");
    skeet::TextDim("sdk/trace.h - ragebot points are scored");
    skeet::TextDim("without a visibility bonus.");
    skeet::Separator();
    skeet::TextColored(skeet::col::kBad, "usercmd control: absent");
    skeet::TextDim("No anti-aim, no fake lag, no tickbase.");
    skeet::Separator();
    skeet::TextColored(skeet::col::kBad, "econ / item schema: absent");
    skeet::TextDim("No skin changer - see the SKINS tab.");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Log", ImVec2(w, 0.f));
    skeet::Text("info  %d", dbg::Count(dbg::Level::Info));
    skeet::Text("warn  %d", dbg::Count(dbg::Level::Warn));
    skeet::Text("error %d", dbg::Count(dbg::Level::Error));
    skeet::TextDim("%s", dbg::FilePath());
    skeet::Separator();
    if (skeet::Button("Dump entities to console (F9)"))
        esp::Dump();
    skeet::EndGroupBox();
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
