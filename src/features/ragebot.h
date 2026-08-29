#pragma once
#include <cstdint>

#include "imgui.h"

#include "features/rage/types.h"
#include "sdk/math.h"

// Ragebot orchestrator.
//
// This file owns the ORDER of the pipeline and nothing else. Every step is
// implemented in features/rage/*:
//
//   1  enabled / key                       here
//   2  local player                        here
//   3  weapon                              rage/weapon_settings
//   4  candidate enemies                   rage/target_selector (Collect)
//   5  filters: dead, dormant, team, FOV   rage/target_selector (Collect)
//   6  allowed hitboxes                    rage/hitbox_scanner
//   7  aim points per hitbox               rage/point_scanner
//   8  score each point                    rage/target_selector (ScorePoint)
//   9  best target + best point            rage/target_selector (Pick)
//   10 minimum damage                      rage/damage
//   11 hitchance                           rage/hitchance
//   12 aim                                 rage/fire (WriteViewAngles)
//   13 fire                                rage/fire
namespace ragebot {

constexpr int kMaxDrawnPoints = 128;

// A multipoint sample kept for the debug overlay, with why it survived.
struct DrawnPoint {
    Vector2 screen{};
    bool    passed = false;
    bool    chosen = false;
};

struct State {
    rage::Diagnostics diag{};

    bool    active    = false;   // key held and everything before the scan is fine
    bool    hasTarget = false;
    Vector2 targetScreen{};
    float   maxFov = 0.f;

    int        drawnCount = 0;
    DrawnPoint drawn[kMaxDrawnPoints]{};
};

extern State state;

void Run();
void Draw(ImDrawList* d, int screenW, int screenH);

} // namespace ragebot
