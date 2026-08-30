#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <cstdint>

#include "imgui.h"

namespace cfg {

enum BoxType : int { Box2D = 0, BoxCorners = 1, BoxFilled = 2 };

// Chams presets. Only Glow is the game's plain glow; the other three are
// tuned variants of the same glow system, not real material overrides.
enum ChamsMode : int { ChamsGlow = 0, ChamsMetallic, ChamsPlastic, ChamsLatex };

// Aim parts, in the order they appear in the menu.
enum AimPart : int {
    PartPelvis = 0, PartHead, PartTorso, PartNeck,
    PartArmL, PartArmR, PartLegL, PartLegR,
    PartCount
};

struct EspGroup {
    bool  enabled   = true;
    bool  box       = true;
    bool  name      = true;
    bool  health    = true;
    bool  distance  = false;
    bool  snapline  = false;
    bool  weapon    = false;
    bool  flags     = false;
    int   boxType   = BoxCorners;
    ImVec4 color    = ImVec4(1.f, 0.35f, 0.35f, 1.f);

    bool   skeleton      = false;
    ImVec4 skeletonColor = ImVec4(0.85f, 0.90f, 1.00f, 1.f);

    bool   aimHistory      = false;
    ImVec4 aimHistoryColor = ImVec4(0.45f, 0.85f, 1.00f, 0.65f);

    ImVec4 flagsColor = ImVec4(1.00f, 0.80f, 0.35f, 1.f);

    bool   glow      = false;
    ImVec4 glowColor = ImVec4(1.00f, 0.30f, 0.30f, 1.f);

    bool   chams     = false;
    int    chamsMode = ChamsGlow;
    ImVec4 chamsColor = ImVec4(0.60f, 0.30f, 1.00f, 1.f);
};

// CS2's glow needs several fields set together, and which ones actually matter
// is build-dependent. These are exposed so the combination can be dialled in
// live instead of guessing at compile time.
// --------------------------------------------------------------- ragebot --
//
// The ragebot config is deliberately shaped as one struct per weapon group
// rather than a flat pile of rifle_hitchance / pistol_hitchance variables:
// every group answers the same questions, so they share one type and live in
// an array indexed by cfg::WeaponGroup.

// Hitboxes the scanner may aim at. Several map to more than one bone (Arms is
// both upper arms, Feet is both ankles) - see features/rage/hitbox_scanner.
enum RageHitbox : int {
    HitboxHead = 0,
    HitboxNeck,
    HitboxChest,
    HitboxStomach,
    HitboxPelvis,
    HitboxArms,
    HitboxLegs,
    HitboxFeet,
    HitboxCount
};

// How the ragebot picks between enemies once every one of them has been
// scanned. Nothing is chosen before the whole scan finishes.
enum RageTargetMode : int {
    TargetClosestFov = 0,
    TargetLowestHp,
    TargetClosestDistance,
    TargetHighestDamage,
    TargetModeCount
};

// Off: never touch movement. Normal: gate aiming until the shot is accurate
// enough. Full: also release the movement keys while the gate is closed.
enum RageAutoStop : int {
    AutoStopOff = 0,
    AutoStopNormal,
    AutoStopFull,
    AutoStopCount
};

enum WeaponGroup : int {
    GroupGlobal = 0,
    GroupPistols,
    GroupHeavyPistols,
    GroupSmg,
    GroupRifles,
    GroupShotguns,
    GroupSnipers,
    GroupAutoSnipers,
    GroupCount
};

struct RageWeaponConfig {
    bool  enabled = true;

    float fov = 30.f;

    int   hitchance         = 55;
    int   minDamage         = 30;
    int   minDamageOverride = 15;

    // Damage above 100 is read as "enemy HP + (value - 100)"; the rule lives in
    // features/rage/damage.cpp, not spread through the scanner.
    bool  hitboxes[HitboxCount] = { true, true, true, true, false, false, false, false };

    bool  multipoint = true;
    float headScale  = 0.55f;   // 0 = centre only, 1 = out at the hitbox edge
    float bodyScale  = 0.75f;

    bool  autoFire  = false;
    int   autoStop  = AutoStopNormal;
    bool  autoScope = true;

    bool  preferBody    = false;   // body scores higher, head still allowed
    bool  forceBody     = false;   // head excluded from the scan entirely
    bool  bodyAimLethal = true;    // prefer body when a body shot already kills
    bool  headshotOnly  = false;   // scan is built from the head alone
};

struct Ragebot {
    bool enabled = false;

    // 0 means "always on"; any other value is a hold key.
    int  key = VK_XBUTTON2;   // mouse 5

    int  targetMode = TargetClosestFov;

    int  minDamageOverrideKey = VK_MENU;   // left alt
    int  forceBodyKey         = 0;

    // When off, every weapon uses the Global group.
    bool perWeaponGroups = true;
    RageWeaponConfig groups[GroupCount];

    // Penetration is modelled, not traced - see features/autowall.h.
    bool autowall      = false;
    int  autowallWalls = 1;

    // --- debug rendering -------------------------------------------------
    bool   debug      = true;
    bool   drawFov    = true;
    bool   drawTarget = true;
    bool   drawPoints = false;

    ImVec4 fovColor    = ImVec4(0.00f, 0.78f, 0.63f, 0.35f);
    ImVec4 targetColor = ImVec4(0.00f, 0.78f, 0.63f, 1.00f);
    ImVec4 pointColor  = ImVec4(0.30f, 1.00f, 0.50f, 0.90f);
};

struct GlowTuning {
    bool usePresets = true;   // let the chams mode choose type/range/alpha

    int  type       = 3;
    int  range      = 8000;
    int  rangeMin   = 0;
    int  team       = -1;
    bool eligible   = true;   // m_bEligibleForScreenHighlight
    bool flashing   = false;
    bool floatColor = true;   // also write m_fGlowColor
    bool glowTime   = true;   // refresh m_flGlowTime / m_flGlowStartTime

    bool verify     = true;   // read back after writing and report mismatches
};

struct Aimbot {
    bool  enabled   = false;
    int   key       = VK_XBUTTON1;   // mouse 4 by default; nothing fires by accident
    float fov       = 6.f;           // degrees
    float smooth    = 5.f;           // 1 = instant snap
    bool  teamCheck = true;
    bool  onPlayers = true;
    bool  onBots    = true;
    bool  requireAlive = true;

    // Skips enemies the game itself does not consider visible to us. Backed by
    // the spotted mask the server replicates, not by a ray trace - see
    // sdk/visibility.h for what that does and does not answer.
    bool  visibilityCheck = false;

    // Writes the aim into the command's input history instead of the camera,
    // so the shot resolves against the target while the view stays put. Needs
    // the CreateMove hook; the panel says so when it is not installed.
    bool  silentAim = false;

    // Which hitboxes are candidates. The closest enabled one wins.
    bool parts[PartCount] = { false, true, false, false, false, false, false, false };

    bool   drawFov     = true;
    ImVec4 fovColor    = ImVec4(1.f, 1.f, 1.f, 0.25f);
    bool   drawTarget  = false;
    ImVec4 targetColor = ImVec4(1.f, 0.85f, 0.2f, 1.f);
};

// Local-only visual suppression. Neither of these touches the entity's game
// logic: anti-smoke rewrites the smoke projectile's own render-start tick, and
// anti-flash clamps the local pawn's flash overlay alpha. Both are restored
// when the option goes back off.
// Compensates the weapon's own recoil by pulling the view back against the
// punch the game reports. Recoil is the systematic part of where the gun goes;
// spread is the random part of a single bullet, and this deliberately does not
// touch it - full strength still leaves the cone.
struct RecoilControl {
    bool enabled = false;

    // Per-axis strength, 0..100 %. 100 cancels that component of the punch.
    int  vertical   = 100;
    int  horizontal = 70;

    // Leaves the first shots alone; the spray only becomes systematic after
    // the first couple of rounds.
    int  startAfterShots = 2;

    // 0 applies the whole correction on the command it is computed; higher
    // spreads it over following commands. Not a sleep - an interpolation
    // driven by the command pipeline.
    int  smooth = 0;

    // How the punch maps onto the view. Left at 1.0 because CS2's
    // m_aimPunchAngle is already in view degrees; exposed rather than baked in
    // so it can be checked against the game instead of assumed.
    float scale = 1.0f;
};

// Fires when the player's own view ray actually crosses an enemy hitbox.
// Lives in the command path, so both the delay and the fire rate follow game
// ticks rather than the frame rate.
struct TriggerBot {
    bool enabled = false;

    // 0 = always armed; anything else is a hold key.
    int  key = VK_MENU;   // left alt

    // How long the same target must stay under the ray before the shot.
    int  delayMs = 0;

    bool teamCheck = true;
    bool visibilityCheck = false;

    // Head, Chest, Stomach, Arms, Legs - see triggerbot::HitGroup.
    bool groups[5] = { true, true, true, false, false };
};

// Written into the user command inside CreateMove. Needs the game hooks; with
// them off both options are inert.
struct Movement {
    // Takes the jump away while airborne so the next landing starts a new one.
    // Never presses jump by itself - space still has to be held.
    bool bhop = false;

    // Strafes into the turn the mouse is already making.
    bool autoStrafe = false;

    // What to do when the mouse is NOT turning. Mouse-only leaves movement
    // alone; directional keeps the gain going by alternating sides on a fixed
    // number of commands, which is tick-based and so fps-independent.
    int  strafeMode = 0;   // movement::StrafeMode
};

struct World {
    bool antiSmoke = false;

    bool antiFlash = false;
    // 0 = no flash at all, 100 = the game's normal flash.
    int  flashAmount = 0;

    // Planted C4. The ESP is just a marker and a label; the timer adds the
    // seconds left before detonation, taken from the bomb's own blow time.
    bool   bombEsp   = false;
    bool   bombTimer = false;
    ImVec4 bombColor = ImVec4(1.00f, 0.45f, 0.10f, 1.f);
};

// A guard rail over the legit aimbot rather than a separate config file. While
// it is on, the legit settings are clamped to modest values and the widgets
// that would exceed them are locked. The user's own values are stashed on the
// way in and put back on the way out.
struct SafeSettings {
    bool enabled = false;

    // Limits. Semantics verified against features/aimbot.cpp: fov is an
    // angular radius in degrees (smaller = tighter) and smooth is a divisor,
    // so a LARGER smooth means a slower drag. Hence a max on fov and a
    // minimum on smooth.
    static constexpr float kMaxFov    = 3.f;
    static constexpr float kMinSmooth = 8.f;

    // Stashed user values, only meaningful while `saved` is true.
    bool  saved = false;
    float prevFov = 0.f;
    float prevSmooth = 0.f;
    bool  prevVisibilityCheck = false;
};

struct Config {
    // Main
    bool masterSwitch = true;
    int  menuKey      = VK_INSERT;
    bool unloadQueued = false;

    // Visual
    bool      espEnabled  = true;
    bool      teamCheck   = true;   // do not draw teammates
    float     maxDistance = 250.f;  // meters
    // One group for every player pawn. Bots are players; nothing about a fake
    // client changes how it should be drawn, so there is no second group.
    EspGroup  players;

    float aimHistoryDelayMs = 50.f; // how far the ghost skeleton trails
    float skeletonThickness = 1.4f;

    Aimbot       aim;
    Ragebot      rage;
    GlowTuning   glowTune;
    RecoilControl recoil;
    TriggerBot   trigger;
    Movement     movement;
    World        world;
    SafeSettings safe;

    // Misc
    bool  watermark = true;
    bool  showFps   = true;
    float menuAlpha = 0.96f;

    // Debug console
    bool  logConsole = true;   // window shown
    bool  logVerbose = false;  // per-second frame dump
    int   logLevel   = 0;      // index into dbg::Level
};

inline Config c;

} // namespace cfg

namespace g {

inline bool unload   = false;
inline bool menuOpen = true;

inline HWND     window   = nullptr;
inline WNDPROC  oWndProc = nullptr;

inline ID3D11Device*           device  = nullptr;
inline ID3D11DeviceContext*    context = nullptr;
inline ID3D11RenderTargetView* rtv     = nullptr;

inline HMODULE  selfModule = nullptr;
inline uintptr_t client    = 0;   // base of client.dll

inline int screenW = 0;
inline int screenH = 0;

} // namespace g
