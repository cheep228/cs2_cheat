#include <Windows.h>
#include <cstring>

#include "sdk/entity.h"
#include "sdk/entitylist.h"

namespace mem {

bool CopyBytes(uintptr_t address, void* dst, size_t size) {
    __try {
        memcpy(dst, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++faults;
        return false;
    }
}

bool WriteBytes(uintptr_t address, const void* src, size_t size) {
    __try {
        memcpy(reinterpret_cast<void*>(address), src, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++faults;
        return false;
    }
}

// SEH cannot live in a frame that needs C++ unwinding, so the guarded copy is
// kept in its own POD-only function.
static bool CopyRaw(uintptr_t address, char* dst, size_t maxLen) {
    __try {
        const char* src = reinterpret_cast<const char*>(address);
        size_t i = 0;
        for (; i < maxLen && src[i]; ++i)
            dst[i] = src[i];
        dst[i] = '\0';
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

std::string ReadString(uintptr_t address, size_t maxLen) {
    if (!Valid(address))
        return {};

    char buf[128]{};
    if (maxLen > sizeof(buf) - 1)
        maxLen = sizeof(buf) - 1;

    if (!CopyRaw(address, buf, maxLen)) {
        ++faults;
        return {};
    }
    return std::string(buf);
}

} // namespace mem

namespace sdk {

uintptr_t GetEntityByIndex(uintptr_t entityList, int index) {
    // The chunk-array offset and identity stride are calibrated at runtime; see
    // sdk/entitylist.cpp.
    return entlist::Get(entityList, index);
}

Vector3 GetAbsOrigin(uintptr_t entity) {
    const uintptr_t node = mem::Read<uintptr_t>(entity + schema::m_pGameSceneNode);
    if (!mem::Valid(node))
        return {};
    return mem::Read<Vector3>(node + schema::m_vecAbsOrigin);
}

bool GetPlayer(uintptr_t entityList, int index, PlayerInfo& out) {
    const uintptr_t controller = GetEntityByIndex(entityList, index);
    if (!mem::Valid(controller))
        return false;

    // Alive flag lives on the controller, so dead players are filtered before
    // we chase the pawn pointer.
    if (!mem::Read<bool>(controller + schema::m_bPawnIsAlive))
        return false;

    const uint32_t handle = mem::Read<uint32_t>(controller + schema::m_hPlayerPawn);
    const uintptr_t pawn = GetEntityByHandle(entityList, handle);
    if (!mem::Valid(pawn))
        return false;

    const int health = mem::Read<int>(pawn + schema::m_iHealth);
    if (health <= 0 || health > 100)
        return false;

    const Vector3 origin = GetAbsOrigin(pawn);
    if (origin.IsZero())
        return false;

    out.controller = controller;
    out.pawn       = pawn;
    out.team       = mem::Read<uint8_t>(pawn + schema::m_iTeamNum);
    out.health     = health;
    // Fake clients (bots) carry no Steam ID.
    out.isBot      = mem::Read<uint64_t>(controller + schema::m_steamID) == 0;
    out.origin     = origin;

    const uintptr_t namePtr = mem::Read<uintptr_t>(controller + schema::m_sSanitizedPlayerName);
    out.name = mem::ReadString(namePtr, 32);
    if (out.name.empty())
        out.name = out.isBot ? "BOT" : "unknown";

    return true;
}

} // namespace sdk
