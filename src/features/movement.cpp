#include <Windows.h>

#include <cmath>

#include "core/globals.h"
#include "features/movement.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"

namespace movement {

Stats stats;

namespace {

// FL_ONGROUND is bit 0 of m_fFlags, the same flag the engine's own jump code
// tests before it lets a jump start.
constexpr uint32_t kFlOnGround = 1u << 0;

// Source 2 move types. Air strafing only applies while actually falling.
constexpr int kMoveTypeWalk    = 2;
constexpr int kMoveTypeFly     = 4;
constexpr int kMoveTypeFlyGrav = 5;
constexpr int kMoveTypeNoClip  = 8;
constexpr int kMoveTypeLadder  = 9;
constexpr int kMoveTypeObserver = 10;

// What the game itself clamps movement to. Writing more does not move faster,
// it just fails the server's bounds check.
constexpr float kMaxSpeed = 450.f;

// Below this the player is barely moving and there is no direction to gain
// speed along; forcing a strafe there only fights them.
constexpr float kMinSpeed = 15.f;

bool OnGround(uintptr_t pawn) {
    return (mem::Read<uint32_t>(pawn + schema::m_fFlags) & kFlOnGround) != 0;
}

bool Alive(uintptr_t pawn) {
    // LIFE_ALIVE is 0.
    return mem::Read<uint8_t>(pawn + schema::m_lifeState) == 0;
}

// Only the two falling move types strafe. Ladder, noclip, observer and
// swimming all use different movement rules, and a strafe written there is
// just a fight with the player's own input.
bool MoveTypeStrafeable(int moveType) {
    return moveType == kMoveTypeWalk || moveType == kMoveTypeFly ||
           moveType == kMoveTypeFlyGrav;
}

// Bunny hop.
//
// The whole trick is that a jump only starts on the tick the button goes from
// released to pressed while standing on the ground. Holding space therefore
// gives one jump; the engine sees it still held on landing and refuses to
// start another. So while airborne the jump is taken away, and it comes back
// the moment the ground is under the player again.
//
// CS2 makes this harder than CS:GO did. Sub-tick input means the press is not
// only a button bit on the command - it is also recorded as a sub-tick move
// step carrying its own timestamp inside the tick. Clearing the button while
// leaving that step behind lets the jump register anyway, which is why the
// naive port of a CS:GO bhop does nothing here. Both have to go.
void RunBhop(usercmd::UserCmd* cmd, bool onGround) {
    stats.jumpReleased = false;
    stats.subticksCut = 0;

    // Only ever takes the jump away while in the air. It never presses jump on
    // its own, so holding space is still required and letting go still stops.
    if (onGround)
        return;

    const bool wantsJump = (cmd->buttons.value & usercmd::IN_JUMP) != 0;
    if (!wantsJump)
        return;

    cmd->buttons.Release(usercmd::IN_JUMP);
    stats.jumpReleased = true;

    usercmd::BaseUserCmdPb* base = cmd->pb.baseCmd;
    if (!base)
        return;

    // Mirror it in the protobuf button state, or the serialised command still
    // carries the press the local ButtonState no longer has.
    if (usercmd::ButtonStatePb* pbButtons = base->inButtonState) {
        pbButtons->SetValue(pbButtons->value & ~usercmd::IN_JUMP);
        pbButtons->SetValueChanged(pbButtons->valueChanged & ~usercmd::IN_JUMP);
    }

    // And the sub-tick steps. Only the jump steps are touched - wiping every
    // step would throw away the strafe input recorded in the same tick.
    for (int i = 0; i < base->subtickMoves.Size(); ++i) {
        usercmd::SubtickMoveStep* step = base->subtickMoves.At(i);
        if (!step)
            continue;
        if ((step->button & usercmd::IN_JUMP) == 0)
            continue;

        step->SetButton(0);
        step->SetPressed(false);
        ++stats.subticksCut;
    }
}

// Auto strafe.
//
// Air acceleration in Source works on the angle between where you are looking
// and where you are asking to move. Each tick the engine grants speed
// proportional to the cosine of that angle, capped, which is why holding the
// strafe key that points into the turn gains speed and holding W does not:
// W lines the wish direction up with the velocity, and once the projection
// exceeds the cap nothing is added at all.
//
// So the job is to keep the wish direction roughly perpendicular to the
// velocity, on the side the player is turning towards.
void RunAutostrafe(usercmd::UserCmd* cmd, uintptr_t pawn, bool airborne) {
    stats.forwardMove = 0.f;
    stats.sideMove = 0.f;
    stats.mouseDx = 0;
    stats.manualInput = false;
    stats.speed2D = 0.f;
    stats.yawDelta = 0.f;

    if (!airborne)
        return;

    usercmd::BaseUserCmdPb* base = cmd->pb.baseCmd;
    if (!base)
        return;

    // Do not fight the player. If they are actively steering with WASD, their
    // wish direction is deliberate and overwriting it is worse than doing
    // nothing - the classic "autostrafe makes me uncontrollable" bug.
    const uint64_t held = cmd->buttons.value;
    const bool manual = (held & (usercmd::IN_FORWARD | usercmd::IN_BACK |
                                 usercmd::IN_MOVELEFT | usercmd::IN_MOVERIGHT)) != 0;
    if (manual) {
        stats.manualInput = true;
        return;
    }

    const Vector3 vel = mem::Read<Vector3>(pawn + schema::m_vecVelocity);
    if (!std::isfinite(vel.x) || !std::isfinite(vel.y))
        return;

    const float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    stats.velX = vel.x; stats.velY = vel.y; stats.velZ = vel.z;
    stats.speed2D = speed;

    const int dx = base->mouseDx;
    stats.mouseDx = dx;

    // Which way to strafe.
    //
    //   mouse turning  -> follow it, that is what the player asked for
    //   mouse still    -> in directional mode keep the gain going by
    //                     alternating sides on a fixed number of COMMANDS.
    //                     Counting commands rather than frames is what makes
    //                     this behave the same at 60 and at 240 fps.
    int dir = 0;   // -1 left, +1 right
    if (dx < 0)
        dir = -1;
    else if (dx > 0)
        dir = 1;
    else if (cfg::c.movement.strafeMode == StrafeDirectional && speed > kMinSpeed) {
        constexpr int kCommandsPerSide = 8;
        const int seq = cmd->sequenceNumber;
        dir = ((seq / kCommandsPerSide) & 1) ? 1 : -1;
    }

    if (dir == 0)
        return;

    // Where the player is looking and where they are actually moving.
    const Vector2 rawView = mem::Read<Vector2>(g::client + offsets::dwViewAngles);
    const float viewYaw = rawView.y;
    stats.viewYaw = viewYaw;

    if (speed > kMinSpeed) {
        stats.velocityYaw = std::atan2(vel.y, vel.x) * kRad2Deg;
        stats.yawDelta = NormalizeYaw(viewYaw - stats.velocityYaw);
    } else {
        stats.velocityYaw = viewYaw;
        stats.yawDelta = 0.f;
    }

    // The wish direction is expressed as forward/side in view space, so a pure
    // sideways wish is sideMove alone. The ideal angle between view and wish
    // is not a constant: the faster you go, the smaller the turn each tick can
    // usefully be, because the engine's cap is reached sooner.
    //
    //   slow  -> a wide wish angle still adds speed
    //   fast  -> the wish must sit close to perpendicular
    //
    // This is the part the old version got wrong: it wrote a full sideways
    // wish at every speed and zeroed forwardMove unconditionally.
    float forwardFraction = 0.f;
    if (speed > kMinSpeed) {
        // Roughly the optimal-gain angle, falling off as speed rises.
        const float ideal = 30.f * (100.f / (speed < 100.f ? 100.f : speed));
        const float clamped = ideal > 45.f ? 45.f : (ideal < 2.f ? 2.f : ideal);
        forwardFraction = std::cos((90.f - clamped) / kRad2Deg);
    }

    const float side = static_cast<float>(dir) * kMaxSpeed;
    const float forward = forwardFraction * kMaxSpeed;

    base->SetSideMove(side);
    base->SetForwardMove(forward);
    stats.sideMove = side;
    stats.forwardMove = forward;
}

} // namespace

void Run(usercmd::UserCmd* cmd, uintptr_t localPawn) {
    stats.active = false;
    stats.airborne = false;
    stats.moveType = -1;

    const cfg::Movement& m = cfg::c.movement;
    if (!m.bhop && !m.autoStrafe)
        return;
    if (!cmd || !mem::Valid(localPawn))
        return;
    if (!Alive(localPawn))
        return;

    const bool onGround = OnGround(localPawn);
    const int moveType = mem::Read<uint8_t>(localPawn + schema::m_MoveType) & 0x0F;

    stats.onGround = onGround;
    stats.moveType = moveType;
    stats.airborne = !onGround && MoveTypeStrafeable(moveType);
    stats.sequence = cmd->sequenceNumber;
    stats.active = true;

    if (m.bhop)
        RunBhop(cmd, onGround);

    // Deliberately after the bhop: the jump has already been taken away for
    // this tick, so the strafe is applied to the airborne command the server
    // will actually act on.
    if (m.autoStrafe)
        RunAutostrafe(cmd, localPawn, stats.airborne);
}

} // namespace movement
