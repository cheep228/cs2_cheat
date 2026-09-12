#pragma once
#include <cstddef>
#include <cstdint>

// Walks Source 2's own schema system at runtime, exactly like the Windows
// project's sdk/schema.h does - the point of doing it this way is that field
// offsets come straight from the running game instead of a static dump, so
// they are correct for whatever build is actually loaded. The struct layout
// being walked here (CUtlVector, CUtlTSHash, CSchemaClassInfoData, ...) is
// Valve's own C++ code, compiled for both Windows and Linux from the same
// source tree, so these internal field offsets carry over - only the module
// lookup (CreateInterface via dlsym instead of GetProcAddress) differs.
namespace schemasys {

// Resolves the SchemaSystem interface via libschemasystem.so's
// CreateInterface. Returns 0 when that library is not loaded yet.
uintptr_t System();

// Pointer to the CSchemaSystemTypeScope for the given module, e.g.
// "libclient.so".
uintptr_t TypeScope(const char* moduleSoName);

// Offset of `className::fieldName` within the class, or -1 when not found.
ptrdiff_t FieldOffset(uintptr_t typeScope, const char* className, const char* fieldName);

// Number of classes reachable in the scope - a health check for the walk.
int ClassCount(uintptr_t typeScope);

} // namespace schemasys
