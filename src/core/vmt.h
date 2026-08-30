#pragma once
#include <cstdint>

// Hooking a virtual method of a game object.
//
// The vtable pointer itself is left alone. Instead the method address is read
// out of the table and MinHook detours it in place, which is what the rest of
// this project already uses for Present. That keeps one hooking mechanism in
// the codebase and means Shutdown() unwinds everything the same way.
namespace vmt {

// Reads vtable[index] off an instance. Returns null when the instance or the
// table does not look like a real object.
void* Method(uintptr_t instance, int index);

// Detours vtable[index]. `original` receives the trampoline. `what` names the
// hook in the log. Returns false and logs the reason on any failure - callers
// are expected to keep running with the feature off rather than abort.
bool Hook(uintptr_t instance, int index, void* detour, void** original,
          const char* what);

} // namespace vmt
