#include <cmath>

#include "features/rage/point_scanner.h"

namespace rage {
namespace points {
namespace {

// Horizontal vector perpendicular to the line of sight. Falls back to world X
// when the shot is straight up or down, where "right" has no meaning anyway.
Vector3 RightOf(const Vector3& dir) {
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len <= 1e-4f)
        return { 1.f, 0.f, 0.f };
    return { dir.y / len, -dir.x / len, 0.f };
}

} // namespace

int Generate(const hitboxes::Entry& box, const Vector3& centre, const Vector3& eye,
             const cfg::RageWeaponConfig& weaponCfg, Vector3* out, int max) {
    if (!out || max <= 0)
        return 0;

    out[0] = centre;
    if (!weaponCfg.multipoint || max == 1)
        return 1;

    // Limbs are thin enough that an edge sample is either the same point or off
    // the model; one centre sample is the honest answer for them.
    if (box.limb)
        return 1;

    const float scale = box.head ? weaponCfg.headScale : weaponCfg.bodyScale;
    if (scale <= 0.01f)
        return 1;

    const float reach = box.radius * scale;
    const Vector3 dir = Normalize(centre - eye);
    if (dir.IsZero())
        return 1;

    const Vector3 right = RightOf(dir);
    constexpr Vector3 up{ 0.f, 0.f, 1.f };

    int count = 1;
    if (count < max)
        out[count++] = centre - right * reach;   // left as seen by the shooter
    if (count < max)
        out[count++] = centre + right * reach;

    // The top of the head is the classic fourth point: it is the part of the
    // model that stays exposed over cover the longest.
    if (box.head && count < max)
        out[count++] = centre + up * reach;

    return count;
}

} // namespace points
} // namespace rage
