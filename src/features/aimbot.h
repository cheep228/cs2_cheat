#pragma once
#include <cstdint>

#include "imgui.h"
#include "sdk/math.h"
#include "sdk/usercmd.h"

namespace aimbot {

struct State {
    bool    active      = false;   // aim key held and aimbot enabled
    bool    hasTarget   = false;
    int     targetSlot  = -1;
    int     targetPart  = -1;
    float   targetFov   = 0.f;     // angular distance to the chosen point
    Vector2 targetScreen{};
    int     candidates  = 0;       // players considered this frame

    // Aim points thrown out by the visibility gate this frame. Stays 0 when
    // the gate is off or when the spotted mask is unavailable.
    int     rejectedOccluded = 0;

    // The angle the aimbot settled on this frame, in world terms. CreateMove
    // reads this: with silent aim on it goes into the command's input history
    // and the camera is left alone, so the two angles stay separate.
    Angle   desiredAngle{};
    bool    silentPending = false;   // desiredAngle is waiting to be applied

    // Where the chosen aim point actually is, for the panel and the marker.
    Vector3 targetWorld{};
    int     headBone = -1;           // bone index the head resolved to
};

extern State state;

// Picks a target among the enabled parts and nudges the view angles towards it.
// Writes to the game's view angles; does nothing while the aim key is up.
void Run();

// Draws the FOV circle and the target marker.
void Draw(ImDrawList* d, int screenW, int screenH);

// Applies the pending aim to the command being built. Called from CreateMove;
// this is the only place silent aim can happen, because the shot angle lives
// in the command and nowhere else.
void ApplyToCommand(usercmd::UserCmd* cmd);

// Bone id behind a cfg::AimPart, or -1.
int BoneForPart(int part);

// Human-readable part names, indexed by cfg::AimPart.
extern const char* const kPartNames[];

} // namespace aimbot
