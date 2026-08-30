#include <Windows.h>
#include <cstring>

#include "features/skeleton.h"
#include "sdk/entity.h"

namespace {

constexpr int kSlots   = 64;
constexpr int kSamples = 24;   // ~0.4 s of history at 60 fps, more at higher

struct Sample {
    uint32_t          time = 0;
    skeleton::Pose    pose;
};

struct Ring {
    Sample samples[kSamples];
    int    head = 0;      // next slot to write
    int    filled = 0;
};

Ring g_history[kSlots];

// Bone chain: {parent, child} pairs forming the visible skeleton.
struct Link { int a, b; };

// Built each frame from the resolved roles: the indices are no longer compile
// time constants, they are identified geometrically at runtime (sdk/bones.h).
constexpr Link kLinks[] = {
    // spine. The model numbers these oddly - spine_1 is 4, spine_2 is 2 - so
    // the chain is written by role, not by ascending index.
    { bones::Pelvis,    bones::Spine0 },
    { bones::Spine0,    bones::Spine2 },
    { bones::Spine2,    bones::Spine3 },
    { bones::Spine3,    bones::Chest },
    { bones::Chest,     bones::Neck },
    { bones::Neck,      bones::Head },
    // arms. There is no clavicle bone in this table; the upper arm hangs
    // straight off the neck.
    { bones::Neck,      bones::ArmUpperL },
    { bones::ArmUpperL, bones::ArmLowerL },
    { bones::ArmLowerL, bones::HandL },
    { bones::Neck,      bones::ArmUpperR },
    { bones::ArmUpperR, bones::ArmLowerR },
    { bones::ArmLowerR, bones::HandR },
    // legs
    { bones::Pelvis,    bones::LegUpperL },
    { bones::LegUpperL, bones::LegLowerL },
    { bones::LegLowerL, bones::AnkleL },
    { bones::Pelvis,    bones::LegUpperR },
    { bones::LegUpperR, bones::LegLowerR },
    { bones::LegLowerR, bones::AnkleR },
};

} // namespace

namespace skeleton {

bool Live(uintptr_t pawn, Pose& out) {
    if (!bones::layout.calibrated || !mem::Valid(pawn))
        return false;

    out.valid = false;
    for (int i = 0; i < bones::Count; ++i)
        out.bone[i] = bones::Position(pawn, i);

    // The pelvis anchors the whole chain; without it nothing can be drawn.
    out.valid = !out.bone[bones::Pelvis].IsZero();
    return out.valid;
}

void Record(int slot, uintptr_t pawn, uint32_t nowMs) {
    if (slot < 0 || slot >= kSlots)
        return;

    Pose pose;
    if (!Live(pawn, pose))
        return;

    Ring& r = g_history[slot];
    r.samples[r.head].time = nowMs;
    r.samples[r.head].pose = pose;
    r.head = (r.head + 1) % kSamples;
    if (r.filled < kSamples)
        ++r.filled;
}

bool Delayed(int slot, uint32_t nowMs, float delayMs, Pose& out) {
    if (slot < 0 || slot >= kSlots)
        return false;

    const Ring& r = g_history[slot];
    if (r.filled == 0)
        return false;

    const uint32_t target = nowMs > static_cast<uint32_t>(delayMs)
                                ? nowMs - static_cast<uint32_t>(delayMs)
                                : 0;

    // Walk back from newest and take the first sample at or before the target.
    const Sample* best = nullptr;
    for (int i = 1; i <= r.filled; ++i) {
        const int idx = (r.head - i + kSamples) % kSamples;
        const Sample& s = r.samples[idx];
        if (!s.pose.valid)
            continue;
        best = &s;
        if (s.time <= target)
            break;
    }

    if (!best)
        return false;

    out = best->pose;
    return out.valid;
}

void Draw(ImDrawList* d, const Pose& pose, const Matrix4x4& view,
          int screenW, int screenH, ImU32 color, float thickness) {
    if (!pose.valid)
        return;

    // No bone on a human skeleton is this far from its parent. Anything longer
    // means the bone id table is wrong for this model, and drawing it would
    // throw a line across the screen instead of showing the mistake.
    constexpr float kMaxLinkLength = 60.f;

    for (const Link& link : kLinks) {
        const Vector3& a = pose.bone[link.a];
        const Vector3& b = pose.bone[link.b];
        if (a.IsZero() || b.IsZero())
            continue;
        if ((a - b).Length() > kMaxLinkLength)
            continue;

        Vector2 sa{}, sb{};
        if (!WorldToScreen(view, a, sa, screenW, screenH) ||
            !WorldToScreen(view, b, sb, screenW, screenH))
            continue;

        d->AddLine(ImVec2(sa.x, sa.y), ImVec2(sb.x, sb.y), color, thickness);
    }
}

void Reset() {
    memset(g_history, 0, sizeof(g_history));
}

} // namespace skeleton
