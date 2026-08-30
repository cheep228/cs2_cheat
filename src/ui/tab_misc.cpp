#include "core/game_hooks.h"
#include "core/globals.h"
#include "features/movement.h"
#include "core/log.h"
#include "features/esp.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/gameinfo.h"
#include "sdk/interfaces.h"
#include "sdk/modules.h"
#include "sdk/usercmd.h"
#include "sdk/offsets.h"
#include "sdk/resolve.h"
#include "sdk/trace.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "GENERAL", "DIAGNOSTICS", "INTERNAL" };
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

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Movement", ImVec2(w, 0.f));
    cfg::Movement& mv = cfg::c.movement;

    theme::Checkbox("Bunny hop", &mv.bhop);
    theme::TextDim("Releases jump in the air so the next");
    theme::TextDim("landing starts a new one. Hold space.");

    theme::Separator();

    theme::Checkbox("Auto strafe", &mv.autoStrafe);
    static const char* const kStrafeModes[] = { "Mouse only", "Directional" };
    theme::Combo("When mouse is still", &mv.strafeMode, kStrafeModes, 2);
    theme::TextDim("Mouse only: follows your turn.");
    theme::TextDim("Directional: keeps gaining with no");
    theme::TextDim("mouse input, alternating every 8 commands.");
    theme::TextDim("Never fights you: holding WASD stands down.");

    if (mv.bhop || mv.autoStrafe) {
        theme::Separator();
        if (!gamehooks::state.createMoveOk) {
            theme::TextColored(theme::col::kBad, "CreateMove hook not installed");
        } else if (!movement::stats.active) {
            theme::TextDim("waiting for a command");
        } else {
            const movement::Stats& s = movement::stats;
            theme::Text("cmd %d   %s   movetype %d", s.sequence,
                        s.onGround ? "on ground" : (s.airborne ? "airborne" : "air (no strafe)"),
                        s.moveType);
            if (mv.bhop)
                theme::Text("jump cut: %s  subticks: %d",
                            s.jumpReleased ? "yes" : "no", s.subticksCut);
            if (mv.autoStrafe) {
                if (s.manualInput) {
                    theme::TextDim("WASD held - standing down");
                } else {
                    theme::Text("speed2D %.0f  mousedx %d", s.speed2D, s.mouseDx);
                    theme::Text("view %.1f  vel %.1f  delta %.1f",
                                s.viewYaw, s.velocityYaw, s.yawDelta);
                    theme::Text("fwd %.0f  side %.0f", s.forwardMove, s.sideMove);
                }
            }
        }
    }
    theme::EndGroupBox();

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

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

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

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

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

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

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

// What the internal layer actually managed to resolve. Every row is a live
// read, so this is the first place to look after a game update: a red line
// here names the exact thing that moved.
void DrawInternal() {
    const float w = theme::ColumnWidth(2);

    auto row = [](const char* label, bool ok, const char* okText = "ok",
                  const char* badText = "MISSING") {
        theme::Text("%-18s", label);
        ImGui::SameLine();
        theme::TextColored(ok ? theme::col::kGood : theme::col::kBad, "%s",
                           ok ? okText : badText);
    };

    theme::BeginGroupBox("Modules", ImVec2(w, 0.f));
    row("client.dll",      modules::set.client.base != 0);
    row("engine2.dll",     modules::set.engine2.base != 0);
    row("scenesystem.dll", modules::set.scenesystem.base != 0);
    row("materialsystem2", modules::set.materialsystem2.base != 0);
    row("vphysics2.dll",   modules::set.vphysics2.base != 0);
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Interfaces", ImVec2(w, 0.f));
    row("CCSGOInput",      ifaces::set.csgoInput != 0);
    row("Source2Client",   ifaces::set.source2Client != 0);
    row("EngineClient",    ifaces::set.engineClient != 0);
    row("EntitySystem",    ifaces::set.entitySystem != 0);
    row("SceneSystem",     ifaces::set.sceneSystem != 0);
    row("MaterialSystem",  ifaces::set.materialSystem != 0);
    row("GlobalVars",      ifaces::set.globalVarsTrusted, "trusted", "untrusted");
    if (const ifaces::GlobalVars* g = ifaces::Globals())
        theme::TextDim("tick %d, curtime %.1f", g->tickCount, g->curTime);
    theme::EndGroupBox();

    const gamehooks::State& gs = gamehooks::state;

    theme::BeginGroupBox("CreateMove", ImVec2(w, 0.f));
    row("hook installed",  gs.installed);
    theme::Text("calls: %llu", static_cast<unsigned long long>(gs.createMoveCalls));
    theme::Separator();
    row("local controller", gs.controllerOk);
    row("command manager",  gs.cmdManagerOk);
    row("ring calibrated",  gs.ringCalibrated);
    if (usercmd::cmdmgr::layout.calibrated)
        theme::TextDim("sequence at +0x%llX",
                       static_cast<unsigned long long>(usercmd::cmdmgr::layout.sequence));
    else
        theme::TextDim("probing both candidate offsets ...");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("User command", ImVec2(w, 0.f));
    row("command found",   gs.cmdOk);
    row("base command",    gs.baseCmdOk);
    row("visible angle",   gs.visibleAngleOk);
    row("shoot angle",     gs.shootAngleOk);
    theme::Text("sequence:  %d", gs.lastSequence);
    theme::Text("history:   %d entries", gs.historyEntries);
    theme::Text("angle now: %.1f %.1f", gs.lastVisibleAngle.x, gs.lastVisibleAngle.y);
    theme::Separator();
    if (gs.shootAngleOk && gs.visibleAngleOk)
        theme::TextColored(theme::col::kGood, "Shot and camera angles are");
    else
        theme::TextColored(theme::col::kBad, "Angles not both readable -");
    if (gs.shootAngleOk && gs.visibleAngleOk)
        theme::TextColored(theme::col::kGood, "separate: silent aim is possible.");
    else
        theme::TextColored(theme::col::kBad, "silent aim stays unavailable.");
    theme::TextDim("Nothing writes to the command yet.");
    theme::EndGroupBox();
}

} // namespace

SubTabs MiscSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void MiscDraw(int sub) {
    if (sub == 0)
        DrawGeneral();
    else if (sub == 1)
        DrawDiagnostics();
    else
        DrawInternal();
}

} // namespace ui
