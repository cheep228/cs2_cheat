#pragma once
#include "imgui.h"

namespace esp {

// Per-frame diagnostics, surfaced by the Main tab so a silent ESP can be
// traced to the step that actually failed.
struct Stats {
    bool localPawnOk   = false;
    bool entityListOk  = false;
    bool viewMatrixOk  = false;
    int  slotsScanned  = 0;   // slots holding a live player
    int  humans        = 0;
    int  bots          = 0;
    int  drawn         = 0;   // survived every filter and got projected
    int  localTeam     = 0;
};

inline Stats stats;

// Draws every enabled overlay element onto the background draw list.
void Render(ImDrawList* draw);

// One-shot deep dump of the raw entity-list reads to the debug console.
// Bound to F9 and to the button in the Main tab.
void Dump();

// Prints every bone index and position for the local pawn and the first enemy
// found, so the bone id table can be checked against a real model. Bound to F10.
void DumpBones();

} // namespace esp
