#pragma once
#include <cstdint>

#include "sdk/usercmd.h"

// Hooks into the game itself, as opposed to the D3D hooks in core/hooks.
//
// This is the part that makes the project internal in the real sense: it runs
// inside CS2's own command pipeline rather than beside it. CreateMove is the
// hook everything else needs, because the user command is the only place
// where the angle a shot is resolved against can be set independently of the
// angle the player sees.
//
// Nothing here writes to the command yet. The hook is installed, the command
// is located and validated, and what it found is reported - so that the
// layout can be confirmed against a live game before anything starts
// modifying commands the server will act on.
namespace gamehooks {

struct State {
    bool installed    = false;
    bool createMoveOk = false;

    uint64_t createMoveCalls = 0;

    // Filled from inside the hook, read by the diagnostics panel.
    bool controllerOk  = false;
    bool cmdManagerOk  = false;
    bool cmdOk         = false;
    bool ringCalibrated = false;

    int  lastSequence    = -1;
    int  historyEntries  = 0;   // input history entries in the last command
    bool baseCmdOk       = false;
    bool visibleAngleOk  = false;   // base command carries a view-angle message
    bool shootAngleOk    = false;   // history entries carry view-angle messages

    Vector3 lastVisibleAngle{};
};

extern State state;

// Resolves what it needs and installs the game hooks. Never throws and never
// aborts: a failure leaves `installed` false and the game untouched.
bool Install();

// Removes every game hook. Safe to call when nothing was installed.
void Remove();

// The command being built this tick, or null outside CreateMove.
usercmd::UserCmd* CurrentCmd();

} // namespace gamehooks
