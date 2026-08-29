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
    int   boxType   = BoxCorners;
    ImVec4 color    = ImVec4(1.f, 0.35f, 0.35f, 1.f);

    bool   skeleton      = false;
    ImVec4 skeletonColor = ImVec4(0.85f, 0.90f, 1.00f, 1.f);

    bool   aimHistory      = false;
    ImVec4 aimHistoryColor = ImVec4(0.45f, 0.85f, 1.00f, 0.65f);

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

    // Which hitboxes are candidates. The closest enabled one wins.
    bool parts[PartCount] = { false, true, false, false, false, false, false, false };

    bool   drawFov     = true;
    ImVec4 fovColor    = ImVec4(1.f, 1.f, 1.f, 0.25f);
    bool   drawTarget  = false;
    ImVec4 targetColor = ImVec4(1.f, 0.85f, 0.2f, 1.f);
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
    EspGroup  players;              // real humans
    EspGroup  bots;                 // fake clients (steamID == 0)

    float aimHistoryDelayMs = 50.f; // how far the ghost skeleton trails
    float skeletonThickness = 1.4f;

    Aimbot     aim;
    Ragebot    rage;
    GlowTuning glowTune;

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
