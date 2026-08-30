#pragma once
#include <cstddef>
#include <cstdint>

#include "sdk/math.h"

// CS2's user command, as it actually looks in memory.
//
// Source 2 sends the command as a protobuf, so the pieces the cheat cares
// about are separate heap objects hanging off CUserCmd rather than plain
// fields. Two things follow from that and neither is optional:
//
//   1. Every protobuf message carries a "has bits" mask. Writing a field
//      without setting its bit means the field is simply not serialised and
//      the server never sees it. That is why every setter below touches
//      set_bits() - a plain assignment silently does nothing.
//
//   2. The angle that gets *shot* lives in the input history entries, while
//      the angle the player *sees* is the base command's. They are separate
//      values, which is exactly what makes silent aim possible: write the
//      history entries and leave the base command alone.
//
// Every struct here is size-checked. A static_assert firing after a game
// update is the cheapest possible warning that the layout moved.
namespace usercmd {

struct Vector4 { float x = 0.f, y = 0.f, z = 0.f, w = 0.f; };

static_assert(sizeof(Vector3) == 12, "Vector3 must be three tight floats");

// Button bits, matching the game's IN_* set.
enum Button : uint64_t {
    IN_ATTACK   = 1ull << 0,
    IN_JUMP     = 1ull << 1,
    IN_DUCK     = 1ull << 2,
    IN_FORWARD  = 1ull << 3,
    IN_BACK     = 1ull << 4,
    IN_USE      = 1ull << 5,
    IN_TURNLEFT = 1ull << 7,
    IN_TURNRIGHT= 1ull << 8,
    IN_MOVELEFT = 1ull << 9,
    IN_MOVERIGHT= 1ull << 10,
    IN_ATTACK2  = 1ull << 11,
    IN_RELOAD   = 1ull << 13,
    IN_SPEED    = 1ull << 16,
};

// Which protobuf slot a field occupies. Setting the wrong bit is as bad as
// setting none, so these are named rather than written as raw shifts.
enum ProtoSlot : uint32_t {
    Slot1  = 1u << 0,  Slot2  = 1u << 1,  Slot3  = 1u << 2,  Slot4  = 1u << 3,
    Slot5  = 1u << 4,  Slot6  = 1u << 5,  Slot7  = 1u << 6,  Slot8  = 1u << 7,
    Slot9  = 1u << 8,  Slot10 = 1u << 9,  Slot11 = 1u << 10, Slot12 = 1u << 11,
    Slot13 = 1u << 12, Slot14 = 1u << 13, Slot15 = 1u << 14, Slot16 = 1u << 15,
};

// Google protobuf's RepeatedPtrField. Only the read side is exposed: creating
// new entries needs game functions, and nothing in this project needs to.
template <typename T>
struct RepeatedPtrField {
    uint64_t arena;
    int      currentSize;
    int      capacity;

    struct Container {
        int max;
        T** Elements() {
            return reinterpret_cast<T**>(reinterpret_cast<uintptr_t>(this) + 0x8);
        }
    }* container;

    int  Size() const { return currentSize; }
    int  Max() const { return container ? container->max : 0; }
    bool Ok() const { return container != nullptr && currentSize >= 0; }

    T* At(int i) {
        if (!container || i < 0 || i >= currentSize)
            return nullptr;
        return container->Elements()[i];
    }
};

// Common header of every protobuf message in the command.
struct BasePb {
    void*    vtable;
    uint32_t hasBits;
    uint64_t cachedBits;

    void SetBits(uint64_t bits) { cachedBits |= bits; }
};
static_assert(sizeof(BasePb) == 0x18);

struct QAnglePb : BasePb {
    Vector3 value;

    // pitch/yaw/roll all live in separate slots, so all three are marked even
    // when only the yaw changed - the server reads the whole angle or none.
    void Set(const Vector3& v) {
        value = v;
        SetBits(Slot1 | Slot2 | Slot3);
    }
};
static_assert(sizeof(QAnglePb) == 0x28);

struct VectorPb : BasePb {
    Vector4 value;
    void Set(const Vector4& v) { value = v; SetBits(Slot1 | Slot2 | Slot3 | Slot4); }
};
static_assert(sizeof(VectorPb) == 0x28);

struct InterpInfoClPb : BasePb {
    float fraction;
    void SetFraction(float f) { fraction = f; SetBits(Slot1); }
};
static_assert(sizeof(InterpInfoClPb) == 0x20);

struct InterpInfoPb : BasePb {
    float fraction;
    int   srcTick;
    int   dstTick;

    void SetFraction(float f) { fraction = f; SetBits(Slot1); }
    void SetSrcTick(int t)    { srcTick = t;  SetBits(Slot2); }
    void SetDstTick(int t)    { dstTick = t;  SetBits(Slot3); }
};
static_assert(sizeof(InterpInfoPb) == 0x28);

// One entry per rendered frame inside the command. The shot is resolved
// against these, which is why writing them moves the bullet without moving
// the camera.
struct InputHistoryEntryPb : BasePb {
    QAnglePb*       viewAngles;
    InterpInfoClPb* clInterp;
    InterpInfoPb*   svInterp0;
    InterpInfoPb*   svInterp1;
    InterpInfoPb*   playerInterp;
    VectorPb*       shootPosition;
    VectorPb*       targetHeadPositionCheck;
    VectorPb*       targetAbsPositionCheck;
    QAnglePb*       targetAngPositionCheck;
    int             renderTickCount;
    float           renderTickFraction;
    int             playerTickCount;
    float           playerTickFraction;
    int             frameNumber;
    int             targetEntIndex;
};
static_assert(sizeof(InputHistoryEntryPb) == 0x78);

struct ButtonStatePb : BasePb {
    uint64_t value;
    uint64_t valueChanged;
    uint64_t valueScroll;

    void SetValue(uint64_t v)        { value = v;        SetBits(Slot1); }
    void SetValueChanged(uint64_t v) { valueChanged = v; SetBits(Slot2); }
    void SetValueScroll(uint64_t v)  { valueScroll = v;  SetBits(Slot3); }
};
static_assert(sizeof(ButtonStatePb) == 0x30);

struct SubtickMoveStep : BasePb {
    uint64_t button;
    bool     pressed;
    float    when;
    float    analogForwardDelta;
    float    analogLeftDelta;
    float    analogPitchDelta;
    float    analogYawDelta;

    void SetButton(uint64_t b) { button = b;  SetBits(Slot1); }
    void SetPressed(bool p)    { pressed = p; SetBits(Slot2); }
};
static_assert(sizeof(SubtickMoveStep) == 56);

struct BaseUserCmdPb : BasePb {
    RepeatedPtrField<SubtickMoveStep> subtickMoves;
    void*          moveCrc;
    ButtonStatePb* inButtonState;
    QAnglePb*      viewAngles;        // the angle the player sees
    void*          executionNotes;
    int            legacyCommandNumber;
    int            clientTick;
    float          forwardMove;
    float          sideMove;
    float          upMove;
    int            impulse;
    int            weaponSelect;
    int            randomSeed;
    int            mouseDx;
    int            mouseDy;
    uint32_t       predictionOffsetTicksX256;
    uint32_t       consumedServerAngleChanges;
    int            cmdFlags;
    uint32_t       pawnEntityHandle;

    void SetForwardMove(float v) { forwardMove = v; SetBits(Slot6); }
    void SetSideMove(float v)    { sideMove = v;    SetBits(Slot7); }
    void SetUpMove(float v)      { upMove = v;      SetBits(Slot8); }

    // Clears every subtick button press. Movement written into subticks
    // survives a plain forwardMove = 0, which is why auto-stop has to do both.
    void ClearSubtickButtons() {
        for (int i = 0; i < subtickMoves.Size(); ++i) {
            SubtickMoveStep* s = subtickMoves.At(i);
            if (!s)
                continue;
            s->SetButton(0);
            s->SetPressed(false);
        }
    }
};
static_assert(sizeof(BaseUserCmdPb) == 0x88);

struct CsgoUserCmdPb {
    void*    vtable;
    uint64_t hasBits;
    uint64_t cachedSize;
    RepeatedPtrField<InputHistoryEntryPb> inputHistory;
    BaseUserCmdPb* baseCmd;
    bool     leftHandDesired;
    bool     predictingBodyShotFx;
    bool     predictingHeadShotFx;
    bool     predictingKillRagdolls;
    int32_t  attack1StartHistoryIndex;
    int32_t  attack2StartHistoryIndex;

    void SetBits(uint64_t bits) { cachedSize |= bits; }
    void SetAttack1HistoryIndex(int i) { attack1StartHistoryIndex = i; SetBits(Slot6); }
    void SetAttack2HistoryIndex(int i) { attack2StartHistoryIndex = i; SetBits(Slot7); }
};
static_assert(sizeof(CsgoUserCmdPb) == 0x48);

// The non-protobuf button state that rides alongside the message.
struct ButtonState {
    void*    vtable;
    uint64_t value;
    uint64_t valueChanged;
    uint64_t valueScroll;

    void Press(uint64_t b)   { value |= b; valueChanged |= b; }
    void Release(uint64_t b) { value &= ~b; valueChanged &= ~b; valueScroll &= ~b; }
};
static_assert(sizeof(ButtonState) == 0x20);

struct UserCmd {
    void*         vtable;
    int           sequenceNumber;
    CsgoUserCmdPb pb;
    ButtonState   buttons;
    char          pad[8];
    double        realTime;
    float         curTime;
    bool          hasBeenPredicted;
    int           previousFlags;
    int           currentFlags;

    // ---- the two angles, kept deliberately distinct --------------------

    // What the shot is resolved against. Writing only this is silent aim.
    // Returns how many history entries were written; 0 means the command had
    // none this tick and the caller must fall back to the visible angle.
    int SetShootAngle(const Vector3& angle) {
        int written = 0;
        for (int i = 0; i < pb.inputHistory.Size(); ++i) {
            InputHistoryEntryPb* e = pb.inputHistory.At(i);
            if (!e || !e->viewAngles)
                continue;
            e->viewAngles->Set(angle);
            ++written;
        }
        return written;
    }

    // What the player sees. Writing this is a visible snap.
    bool SetVisibleAngle(const Vector3& angle) {
        if (!pb.baseCmd || !pb.baseCmd->viewAngles)
            return false;
        pb.baseCmd->viewAngles->Set(angle);
        return true;
    }

    // Points attack1 at the newest history entry, so the server resolves the
    // shot against the angle we just wrote rather than an older one.
    void AimAttackAtNewestHistory() {
        const int n = pb.inputHistory.Size();
        pb.SetAttack1HistoryIndex(n > 0 ? n - 1 : -1);
    }

    void PressButton(uint64_t b) {
        buttons.Press(b);
        if (pb.baseCmd && pb.baseCmd->inButtonState) {
            ButtonStatePb* s = pb.baseCmd->inButtonState;
            s->SetValue(s->value | b);
            s->SetValueChanged(s->valueChanged | b);
        }
    }
};
static_assert(sizeof(UserCmd) == 0x98);


// ---------------------------------------------------------------- manager --
//
// The commands live in a ring buffer on the local controller, and the current
// one is commands[sequence % 150].
//
// The offset of `sequence` is NOT hardcoded. The array of 150 commands ends at
// 0x5910, but the reference layout this was ported from marks the sequence at
// 0x59A8 - exactly one command further on, because a spare command sits
// between them in some builds. Picking the wrong one reads a float as a ring
// index, so instead the two candidates are probed against live behaviour:
// a real sequence number is small, non-negative and moves forward every tick.
namespace cmdmgr {

inline constexpr int      kRingSize = 150;
inline constexpr ptrdiff_t kCommands = 0x0;

// The two plausible homes for the sequence counter.
inline constexpr ptrdiff_t kSeqCandidates[] = { 0x5910, 0x59A8 };

struct Layout {
    ptrdiff_t sequence   = 0x5910;
    bool      calibrated = false;
};

extern Layout layout;

// Watches both candidates across successive calls and keeps the one that
// behaves like a counter. Returns true once settled; cheap after that.
bool Calibrate(uintptr_t manager);

// Current command, or null until the layout is settled.
UserCmd* Current(uintptr_t manager);

} // namespace cmdmgr
} // namespace usercmd
