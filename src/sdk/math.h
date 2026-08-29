#pragma once
#include <cmath>

struct Vector3 {
    float x = 0.f, y = 0.f, z = 0.f;

    constexpr Vector3() = default;
    constexpr Vector3(float a, float b, float c) : x(a), y(b), z(c) {}

    constexpr Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    constexpr Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    constexpr Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    constexpr bool IsZero() const { return x == 0.f && y == 0.f && z == 0.f; }
};

// Source 2 view-projection matrix, row-major.
struct Matrix4x4 {
    float m[4][4]{};
};

struct Vector2 { float x = 0.f, y = 0.f; };

// View angles are stored as (pitch, yaw, roll); only the first two matter here.
struct Angle {
    float pitch = 0.f, yaw = 0.f, roll = 0.f;
};

// Unit vector, or a zero vector when the input has no length.
inline Vector3 Normalize(const Vector3& v) {
    const float len = v.Length();
    if (len <= 1e-6f)
        return {};
    return { v.x / len, v.y / len, v.z / len };
}

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kRad2Deg = 180.f / kPi;

// Folds a yaw into (-180, 180].
inline float NormalizeYaw(float yaw) {
    while (yaw > 180.f)  yaw -= 360.f;
    while (yaw < -180.f) yaw += 360.f;
    return yaw;
}

inline float ClampPitch(float pitch) {
    if (pitch > 89.f)  return 89.f;
    if (pitch < -89.f) return -89.f;
    return pitch;
}

// Angles that look from `from` towards `to`.
inline Angle CalcAngle(const Vector3& from, const Vector3& to) {
    const Vector3 d = to - from;
    const float flat = std::sqrt(d.x * d.x + d.y * d.y);

    Angle a;
    a.pitch = ClampPitch(-std::atan2(d.z, flat) * kRad2Deg);
    a.yaw = NormalizeYaw(std::atan2(d.y, d.x) * kRad2Deg);
    a.roll = 0.f;
    return a;
}

// Shortest angular distance between two view angles, in degrees.
inline float AngleDelta(const Angle& a, const Angle& b) {
    const float dp = a.pitch - b.pitch;
    const float dy = NormalizeYaw(a.yaw - b.yaw);
    return std::sqrt(dp * dp + dy * dy);
}

// Projects a world position into screen space. Returns false if behind the camera.
inline bool WorldToScreen(const Matrix4x4& vm, const Vector3& in, Vector2& out,
                          int screenW, int screenH) {
    const float w = vm.m[3][0] * in.x + vm.m[3][1] * in.y + vm.m[3][2] * in.z + vm.m[3][3];
    if (w < 0.01f)
        return false;

    const float nx = vm.m[0][0] * in.x + vm.m[0][1] * in.y + vm.m[0][2] * in.z + vm.m[0][3];
    const float ny = vm.m[1][0] * in.x + vm.m[1][1] * in.y + vm.m[1][2] * in.z + vm.m[1][3];

    const float invW = 1.f / w;
    out.x = (screenW * 0.5f) * (1.f + nx * invW);
    out.y = (screenH * 0.5f) * (1.f - ny * invW);
    return true;
}
