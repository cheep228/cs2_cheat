#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

#include "sdk/math.h"
#include "sdk/offsets.h"

namespace mem {

// Cheap sanity filter for user-mode canonical pointers.
inline bool Valid(uintptr_t p) {
    return p > 0x10000 && p < 0x7FFFFFFFFFFFull;
}

// Reads that raised an access violation. A rising count is the signature of
// stale offsets; the ESP reports it to the debug console.
inline unsigned long faults = 0;

// Guarded raw copy; SEH is confined here so callers stay free to use C++ types.
bool CopyBytes(uintptr_t address, void* dst, size_t size);

// Guarded write. Used only by the glow/chams and aim paths — everything else
// in this project is read-only.
bool WriteBytes(uintptr_t address, const void* src, size_t size);

template <typename T>
bool Write(uintptr_t address, const T& value) {
    if (!Valid(address))
        return false;
    return WriteBytes(address, &value, sizeof(T));
}

template <typename T>
T Read(uintptr_t address) {
    T value{};
    if (!Valid(address))
        return value;
    if (!CopyBytes(address, &value, sizeof(T)))
        return T{};
    return value;
}

// Reads a NUL-terminated ASCII string, bounded.
std::string ReadString(uintptr_t address, size_t maxLen = 64);

} // namespace mem

namespace sdk {

// Source 2 entity list is a two-level table: 64 chunks of 512 entries,
// each entry being 120 bytes with the pointer at offset 0.
uintptr_t GetEntityByIndex(uintptr_t entityList, int index);

// Resolves a CHandle (uint32) to its entity pointer.
inline uintptr_t GetEntityByHandle(uintptr_t entityList, uint32_t handle) {
    if (handle == 0xFFFFFFFF)
        return 0;
    return GetEntityByIndex(entityList, static_cast<int>(handle & 0x7FFF));
}

// Absolute origin via m_pGameSceneNode.
Vector3 GetAbsOrigin(uintptr_t entity);

struct PlayerInfo {
    uintptr_t controller = 0;
    uintptr_t pawn       = 0;
    int       team       = 0;
    int       health     = 0;
    bool      isBot      = false;
    Vector3   origin{};
    std::string name;
};

// Fills `out` for entity slot `index`; false if the slot holds no live player.
bool GetPlayer(uintptr_t entityList, int index, PlayerInfo& out);

} // namespace sdk
