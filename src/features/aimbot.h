#pragma once
#include <cstdint>

#include "imgui.h"
#include "sdk/math.h"

namespace aimbot {

struct State {
    bool    active      = false;   // aim key held and aimbot enabled
    bool    hasTarget   = false;
    int     targetSlot  = -1;
    int     targetPart  = -1;
    float   targetFov   = 0.f;     // angular distance to the chosen point
    Vector2 targetScreen{};
    int     candidates  = 0;       // players considered this frame
};

extern State state;

// Picks a target among the enabled parts and nudges the view angles towards it.
// Writes to the game's view angles; does nothing while the aim key is up.
void Run();

// Draws the FOV circle and the target marker.
void Draw(ImDrawList* d, int screenW, int screenH);

// Bone id behind a cfg::AimPart, or -1.
int BoneForPart(int part);

// Human-readable part names, indexed by cfg::AimPart.
extern const char* const kPartNames[];

} // namespace aimbot
