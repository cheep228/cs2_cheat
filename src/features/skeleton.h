#pragma once
#include <cstdint>

#include "imgui.h"
#include "sdk/bones.h"
#include "sdk/math.h"

namespace skeleton {

// One frame of bone positions for one player.
struct Pose {
    Vector3 bone[bones::Count];
    bool    valid = false;
};

// Captures the current pose of `pawn` into slot `slot`'s history ring.
void Record(int slot, uintptr_t pawn, uint32_t nowMs);

// Reads the live pose without touching the history.
bool Live(uintptr_t pawn, Pose& out);

// Pose as it was `delayMs` ago. False when the ring has nothing that old yet.
bool Delayed(int slot, uint32_t nowMs, float delayMs, Pose& out);

// Draws the bone chain of a pose.
void Draw(ImDrawList* d, const Pose& pose, const Matrix4x4& view,
          int screenW, int screenH, ImU32 color, float thickness);

// Drops every ring, e.g. on map change.
void Reset();

} // namespace skeleton
