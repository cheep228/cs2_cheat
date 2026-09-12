#pragma once

// The feature itself: periodically finds the local player's weapons and
// rewrites their econ item's "fallback" fields to match config::Find's
// entry for that weapon's definition index.
//
// IMPORTANT, and this is not a technical limitation that a future version
// fixes: this only changes what the weapon model looks like in the LOCAL
// client's own renderer. It never touches the real Steam inventory, never
// talks to Valve's servers, and (being entirely local) is not visible to
// other players on a secure server. A skin changer that actually granted an
// item would mean writing to Steam's backend economy, which is not
// something a local memory patch can do.
namespace skinchanger {

// Runs resolve/apply once. Call this from a loop (main.cpp does, at a few
// hundred ms interval - there is no need to do this every frame).
void Tick();

} // namespace skinchanger
