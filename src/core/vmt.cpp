#include <Windows.h>

#include "MinHook.h"

#include "core/log.h"
#include "core/vmt.h"
#include "sdk/entity.h"

namespace vmt {

void* Method(uintptr_t instance, int index) {
    if (!mem::Valid(instance) || index < 0 || index > 512)
        return nullptr;

    const uintptr_t table = mem::Read<uintptr_t>(instance);
    if (!mem::Valid(table))
        return nullptr;

    const uintptr_t fn = mem::Read<uintptr_t>(table + static_cast<uintptr_t>(index) * sizeof(void*));
    if (!mem::Valid(fn))
        return nullptr;

    return reinterpret_cast<void*>(fn);
}

bool Hook(uintptr_t instance, int index, void* detour, void** original,
          const char* what) {
    void* target = Method(instance, index);
    if (!target) {
        DBG_E("vmt", "%s: vtable[%d] is unreadable - hook skipped", what, index);
        return false;
    }

    // A vtable entry that does not point into executable memory means the
    // instance is not what we think it is; detouring it would corrupt the
    // process rather than fail cleanly.
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(target, &mbi, sizeof(mbi)) ||
        !(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                         PAGE_EXECUTE_WRITECOPY))) {
        DBG_E("vmt", "%s: vtable[%d] -> 0x%llX is not executable - hook skipped",
              what, index, static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(target)));
        return false;
    }

    MH_STATUS st = MH_CreateHook(target, detour, original);
    if (st != MH_OK) {
        DBG_E("vmt", "%s: MH_CreateHook failed: %s", what, MH_StatusToString(st));
        return false;
    }

    st = MH_EnableHook(target);
    if (st != MH_OK) {
        DBG_E("vmt", "%s: MH_EnableHook failed: %s", what, MH_StatusToString(st));
        MH_RemoveHook(target);
        return false;
    }

    DBG_OK("vmt", "%-22s hooked at vtable[%d] -> 0x%llX", what, index,
           static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(target)));
    return true;
}

} // namespace vmt
