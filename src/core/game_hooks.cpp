#include <Windows.h>

#include "MinHook.h"

#include "core/game_hooks.h"
#include "features/aimbot.h"
#include "features/movement.h"
#include "features/recoil.h"
#include "features/triggerbot.h"
#include "core/globals.h"
#include "core/log.h"
#include "core/vmt.h"
#include "sdk/entity.h"
#include "sdk/interfaces.h"
#include "sdk/modules.h"
#include "sdk/offsets.h"
#include "sdk/usercmd.h"

namespace gamehooks {

State state;

namespace {

// CCSGOInput::CreateMove. Index 5 of the input object's vtable; taking it from
// the table rather than a byte pattern is what makes this survive the routine
// code shuffling of a game update.
constexpr int kCreateMoveIndex = 5;

using CreateMoveFn = void(__fastcall*)(void* input, int slot, bool active);
CreateMoveFn oCreateMove = nullptr;

// CCSPlayerController::GetCommandManager - a plain function, so this one does
// need a pattern.
using GetCmdManagerFn = uintptr_t(__fastcall*)(uintptr_t controller);
GetCmdManagerFn g_getCmdManager = nullptr;

usercmd::UserCmd* g_cmd = nullptr;

bool g_reportedFirstCall = false;

// Reads everything we want to know about the command and records it. Kept
// separate from the hook body so the hook stays obviously side-effect free.
void InspectCommand(uintptr_t controller) {
    state.controllerOk = mem::Valid(controller);
    state.cmdManagerOk = false;
    state.cmdOk = false;
    state.baseCmdOk = false;
    state.visibleAngleOk = false;
    state.shootAngleOk = false;
    state.historyEntries = 0;
    g_cmd = nullptr;

    if (!state.controllerOk || !g_getCmdManager)
        return;

    const uintptr_t manager = g_getCmdManager(controller);
    state.cmdManagerOk = mem::Valid(manager);
    if (!state.cmdManagerOk)
        return;

    // The ring layout settles over the first couple of seconds of play; until
    // it does, Current() refuses to hand anything back.
    state.ringCalibrated = usercmd::cmdmgr::Calibrate(manager);
    if (!state.ringCalibrated)
        return;

    state.lastSequence = mem::Read<int>(manager + usercmd::cmdmgr::layout.sequence);

    usercmd::UserCmd* cmd = usercmd::cmdmgr::Current(manager);
    if (!cmd)
        return;

    state.cmdOk = true;
    g_cmd = cmd;

    usercmd::BaseUserCmdPb* base = cmd->pb.baseCmd;
    state.baseCmdOk = mem::Valid(reinterpret_cast<uintptr_t>(base));

    if (state.baseCmdOk && mem::Valid(reinterpret_cast<uintptr_t>(base->viewAngles))) {
        state.visibleAngleOk = true;
        state.lastVisibleAngle = base->viewAngles->value;
    }

    if (cmd->pb.inputHistory.Ok()) {
        state.historyEntries = cmd->pb.inputHistory.Size();
        for (int i = 0; i < state.historyEntries; ++i) {
            usercmd::InputHistoryEntryPb* e = cmd->pb.inputHistory.At(i);
            if (e && mem::Valid(reinterpret_cast<uintptr_t>(e->viewAngles))) {
                state.shootAngleOk = true;
                break;
            }
        }
    }

    if (!g_reportedFirstCall) {
        g_reportedFirstCall = true;
        DBG_OK("cmd", "first command inspected: seq=%d history=%d "
                      "visibleAngle=%s shootAngle=%s",
               state.lastSequence, state.historyEntries,
               state.visibleAngleOk ? "yes" : "NO",
               state.shootAngleOk ? "yes" : "NO");
        DBG_I("cmd", "visible angle now: %.2f %.2f",
              state.lastVisibleAngle.x, state.lastVisibleAngle.y);
    }
}

void __fastcall hkCreateMove(void* input, int slot, bool active) {
    // The original runs first: the command is not finished being built until
    // it returns, and inspecting a half-built command reads stale pointers.
    if (oCreateMove)
        oCreateMove(input, slot, active);

    ++state.createMoveCalls;

    if (!g::client)
        return;

    const uintptr_t controller =
        mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerController);

    InspectCommand(controller);

    // Movement is the first thing allowed to write to the command: bhop and
    // autostrafe only work before it is signed and sent, which is exactly what
    // being inside CreateMove buys.
    if (g_cmd) {
        const uintptr_t localPawn =
            mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
        movement::Run(g_cmd, localPawn);

        // Silent aim: the angle the aimbot picked goes into the command's
        // input history here, which is the only place the shot angle and the
        // camera angle can be different values.
        aimbot::ApplyToCommand(g_cmd);

        // Recoil control comes after the aim and before the trigger: it reads
        // the angle the aimbot just produced, adds its correction and writes
        // once, so there is a single final angle per command rather than two
        // writers fighting. It also runs with the aimbot off, for manual fire.
        recoil::Run(g_cmd, localPawn);

        // The trigger runs last: it traces from the angle that is going out in
        // THIS command, which is the one the server will resolve the shot
        // against - including a silent aim angle written a moment ago.
        triggerbot::Run(g_cmd, localPawn);
    }
}

} // namespace

usercmd::UserCmd* CurrentCmd() {
    return g_cmd;
}

bool Install() {
    if (state.installed)
        return true;

    if (!ifaces::Resolve()) {
        DBG_W("gamehook", "interfaces not ready - game hooks not installed yet");
        return false;
    }

    if (!g_getCmdManager) {
        const uintptr_t at = modules::Find(modules::set.client,
                                           "41 56 41 57 48 83 EC ? 48 8D 54 24",
                                           "GetCommandManager");
        g_getCmdManager = reinterpret_cast<GetCmdManagerFn>(at);
    }

    if (!g_getCmdManager) {
        DBG_E("gamehook", "no command manager accessor - CreateMove would be useless");
        return false;
    }

    state.createMoveOk = vmt::Hook(ifaces::set.csgoInput, kCreateMoveIndex,
                                   reinterpret_cast<void*>(&hkCreateMove),
                                   reinterpret_cast<void**>(&oCreateMove),
                                   "CCSGOInput::CreateMove");

    state.installed = state.createMoveOk;
    if (state.installed)
        DBG_OK("gamehook", "game hooks armed - watching the command pipeline");
    else
        DBG_E("gamehook", "game hooks NOT armed - running as an overlay only");

    return state.installed;
}

void Remove() {
    if (!state.installed)
        return;

    if (void* target = vmt::Method(ifaces::set.csgoInput, kCreateMoveIndex)) {
        MH_DisableHook(target);
        MH_RemoveHook(target);
    }

    oCreateMove = nullptr;
    g_cmd = nullptr;
    state = State{};
    DBG_OK("gamehook", "game hooks removed");
}

} // namespace gamehooks
