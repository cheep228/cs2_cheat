#include <Windows.h>

#include "core/log.h"
#include "features/chams.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"

namespace {

// CS2 stores colours as four bytes, RGBA.
uint32_t PackColor(const ImVec4& c) {
    auto b = [](float v) -> uint32_t {
        const int i = static_cast<int>(v * 255.f + 0.5f);
        return static_cast<uint32_t>(i < 0 ? 0 : (i > 255 ? 255 : i));
    };
    return b(c.x) | (b(c.y) << 8) | (b(c.z) << 16) | (b(c.w) << 24);
}

struct GlowStyle {
    int   type;
    int   range;
    bool  flashing;
    float alphaScale;
};

// Presets over the glow system. Which type renders as what is build-dependent,
// which is why the Glow tuning panel can override all of it.
GlowStyle StyleFor(int chamsMode) {
    switch (chamsMode) {
    case cfg::ChamsMetallic: return { 2, 8000, false, 1.00f };
    case cfg::ChamsPlastic:  return { 1, 8000, false, 0.80f };
    case cfg::ChamsLatex:    return { 0, 8000, false, 0.55f };
    case cfg::ChamsGlow:
    default:                 return { 3, 8000, false, 1.00f };
    }
}

// Writes the whole property, not just m_bGlowing: CS2 gates the highlight on
// several fields at once, so a partial write silently does nothing.
void WriteGlow(uintptr_t pawn, const ImVec4& color, GlowStyle style) {
    const cfg::GlowTuning& t = cfg::c.glowTune;
    if (!t.usePresets) {
        style.type = t.type;
        style.range = t.range;
        style.flashing = t.flashing;
        style.alphaScale = 1.f;
    }

    const uintptr_t glow = pawn + schema::m_Glow;

    ImVec4 c = color;
    c.w *= style.alphaScale;
    const uint32_t packed = PackColor(c);

    if (t.floatColor) {
        const float rgb[3] = { c.x, c.y, c.z };
        mem::WriteBytes(glow + schema::m_fGlowColor, rgb, sizeof(rgb));
    }

    mem::Write<int32_t>(glow + schema::m_iGlowType, style.type);
    mem::Write<int32_t>(glow + schema::m_iGlowTeam, t.team);
    mem::Write<int32_t>(glow + schema::m_nGlowRange, style.range);
    mem::Write<int32_t>(glow + schema::m_nGlowRangeMin, t.rangeMin);
    mem::Write<uint32_t>(glow + schema::m_glowColorOverride, packed);
    mem::Write<uint8_t>(glow + schema::m_bFlashing, style.flashing ? 1 : 0);

    if (t.glowTime) {
        mem::Write<float>(glow + schema::m_flGlowTime, 999.f);
        mem::Write<float>(glow + schema::m_flGlowStartTime, 0.f);
    }

    mem::Write<uint8_t>(glow + schema::m_bEligibleForScreenHighlight, t.eligible ? 1 : 0);
    mem::Write<uint8_t>(glow + schema::m_bGlowing, 1);

    if (!t.verify)
        return;

    // A write that does not stick means the game overwrites it every frame, or
    // the property is not where we think it is. Those need different fixes, so
    // the log has to tell them apart.
    const uint8_t  glowingBack = mem::Read<uint8_t>(glow + schema::m_bGlowing);
    const uint32_t colorBack   = mem::Read<uint32_t>(glow + schema::m_glowColorOverride);
    const int32_t  typeBack    = mem::Read<int32_t>(glow + schema::m_iGlowType);

    if (glowingBack != 1 || colorBack != packed || typeBack != style.type) {
        DBG_EVERY(3000, dbg::Level::Warn, "chams",
                  "write did not stick: glowing=%u (want 1), color=0x%08X (want 0x%08X), "
                  "type=%d (want %d)",
                  glowingBack, colorBack, packed, typeBack, style.type);
    } else {
        DBG_EVERY(10000, dbg::Level::Trace, "chams",
                  "glow property holds: type=%d range=%d color=0x%08X",
                  typeBack, style.range, colorBack);
    }
}

} // namespace

namespace chams {

int lastApplied = 0;

void Apply(uintptr_t pawn, const cfg::EspGroup& group) {
    if (!mem::Valid(pawn))
        return;

    // Chams wins when both are on: it is the more specific setting.
    if (group.chams) {
        WriteGlow(pawn, group.chamsColor, StyleFor(group.chamsMode));
        ++lastApplied;
    } else if (group.glow) {
        WriteGlow(pawn, group.glowColor, StyleFor(cfg::ChamsGlow));
        ++lastApplied;
    }
}

void Clear(uintptr_t pawn) {
    if (!mem::Valid(pawn))
        return;
    mem::Write<uint8_t>(pawn + schema::m_Glow + schema::m_bGlowing, 0);
}

} // namespace chams
