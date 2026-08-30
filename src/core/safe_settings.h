#pragma once

// Safe settings: a guard rail over the legit aimbot, not a config file.
//
// While it is on, the legit settings are held at modest values and the widgets
// that would exceed them are bounded in the UI rather than snapped back after
// the fact. The user's own values are stashed on the way in and restored on
// the way out, so toggling it twice is a no-op.
//
// Semantics were read off features/aimbot.cpp rather than assumed:
//   fov    - angular radius in degrees, compared with AngleDelta. SMALLER is
//            tighter, so the safe rule is a MAXIMUM.
//   smooth - a divisor in Smooth(): out = current + delta / smooth, and
//            smooth <= 1 snaps. LARGER is slower, so the safe rule is a
//            MINIMUM.
namespace safeset {

// Call once per frame. Acts only on a transition or on a value that is
// actually out of bounds, so it never rewrites the config every frame.
void Sync();

// True while the guard rail is engaged.
bool Active();

} // namespace safeset
