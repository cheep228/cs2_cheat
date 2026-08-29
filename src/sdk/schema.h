#pragma once
#include <cstddef>
#include <cstdint>

// Walks Source 2's own schema system, which is what a2x/cs2-dumper reads from
// the outside. Doing it in-process means field offsets are always correct for
// the running build, with no dump to keep in sync.
//
// Struct layouts below mirror cs2-dumper's src/source2/schema_system.
namespace schemasys {

// Resolves the SchemaSystem interface. Returns 0 when schemasystem.dll is not
// loaded yet or the interface version changed.
uintptr_t System();

// Pointer to the CSchemaSystemTypeScope for the given module ("client.dll").
uintptr_t TypeScope(const char* moduleName);

// Offset of `className::fieldName` within the class, or -1 when not found.
ptrdiff_t FieldOffset(uintptr_t typeScope, const char* className, const char* fieldName);

// Number of classes reachable in the scope — a health check for the walk.
int ClassCount(uintptr_t typeScope);

// Writes every field of the listed classes to `path`, for sending back when
// something still does not add up. Returns the number of classes written.
int DumpClasses(uintptr_t typeScope, const char* const* classNames, int classCount,
                const char* path);

} // namespace schemasys
