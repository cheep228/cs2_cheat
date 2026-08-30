#pragma once
#include <cstdint>

#include "sdk/usercmd.h"

// Movement written into the user command.
//
// This only works because the project is internal: both features edit the
// command inside CreateMove, before it is signed and sent. That also makes
// them tick-based rather than frame-based - CreateMove runs once per command,
// so the behaviour is identical at 60 and at 240 fps. Nothing here may ever be
// driven from the present hook.
namespace movement {

// How the strafe direction is chosen when the mouse is not turning.
enum StrafeMode : int {
    // Follows the mouse only. With no mouse input, movement is left alone.
    StrafeMouseOnly = 0,
    // Keeps gaining speed with no mouse input by alternating sides on a fixed
    // number of commands, the way a player alternates A and D.
    StrafeDirectional,
    StrafeModeCount
};

struct Stats {
    bool  active       = false;   // ran inside a valid command this tick
    bool  onGround     = false;
    bool  airborne     = false;
    int   moveType     = -1;
    bool  jumpReleased = false;   // bhop cleared the jump this tick
    int   subticksCut  = 0;       // subtick jump steps cleared this tick

    // Autostrafe working values, all live, for the debug panel.
    float velX = 0.f, velY = 0.f, velZ = 0.f;
    float speed2D     = 0.f;
    float viewYaw     = 0.f;
    float velocityYaw = 0.f;
    float yawDelta    = 0.f;   // view yaw minus velocity yaw, normalised
    int   mouseDx     = 0;
    float forwardMove = 0.f;
    float sideMove    = 0.f;
    bool  manualInput = false;  // player is holding WASD, so we stood back
    int   sequence    = 0;      // command number, proves this is tick driven
};

extern Stats stats;

// Called from the CreateMove hook with the command being built.
void Run(usercmd::UserCmd* cmd, uintptr_t localPawn);

} // namespace movement
