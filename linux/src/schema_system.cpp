#include "schema_system.h"

#include <cstring>
#include <dlfcn.h>

#include "log.h"
#include "mem.h"
#include "modules.h"

namespace {

// Mirrors cs2-dumper's src/source2/schema_system (same layout the Windows
// build's sdk/schema.cpp walks) - see schema_system.h for why this carries
// over to the Linux build unchanged.
namespace layout {

constexpr ptrdiff_t kTypeScopesVector = 0x190;   // UtlVector<CSchemaSystemTypeScope*>

constexpr ptrdiff_t kVectorCount = 0x00;
constexpr ptrdiff_t kVectorData  = 0x08;

constexpr ptrdiff_t kScopeName          = 0x008;   // inline char[256]
constexpr ptrdiff_t kScopeClassBindings = 0x560;   // UtlTsHash<CSchemaClassBinding>

constexpr ptrdiff_t kHashBuckets = 0x60;
constexpr int       kBucketCount = 256;
constexpr ptrdiff_t kBucketStride           = 0x18;
constexpr ptrdiff_t kBucketFirst            = 0x08;
constexpr ptrdiff_t kBucketFirstUncommitted = 0x10;

constexpr ptrdiff_t kNodeNext = 0x08;
constexpr ptrdiff_t kNodeData = 0x10;

constexpr ptrdiff_t kClassName       = 0x08;
constexpr ptrdiff_t kClassFieldCount = 0x24;   // int16
constexpr ptrdiff_t kClassFields     = 0x30;

constexpr ptrdiff_t kFieldStride = 0x20;
constexpr ptrdiff_t kFieldName   = 0x00;
constexpr ptrdiff_t kFieldOffset = 0x10;

} // namespace layout

constexpr int kMaxNodesPerBucket = 4096;

using CreateInterfaceFn = void*(*)(const char*, int*);

bool ReadNamePtr(uintptr_t at, char* out, size_t cap) {
    const uintptr_t p = mem::Read<uintptr_t>(at);
    if (!mem::Valid(p))
        return false;
    const std::string s = mem::ReadString(p, cap - 1);
    if (s.empty())
        return false;
    std::strncpy(out, s.c_str(), cap - 1);
    out[cap - 1] = '\0';
    return true;
}

template <typename F>
void ForEachClass(uintptr_t typeScope, F&& fn) {
    if (!mem::Valid(typeScope))
        return;

    const uintptr_t buckets = typeScope + layout::kScopeClassBindings + layout::kHashBuckets;

    for (int b = 0; b < layout::kBucketCount; ++b) {
        const uintptr_t bucket = buckets + b * layout::kBucketStride;

        const ptrdiff_t chains[] = { layout::kBucketFirstUncommitted, layout::kBucketFirst };

        for (ptrdiff_t chain : chains) {
            uintptr_t node = mem::Read<uintptr_t>(bucket + chain);

            for (int n = 0; n < kMaxNodesPerBucket && mem::Valid(node); ++n) {
                const uintptr_t classInfo = mem::Read<uintptr_t>(node + layout::kNodeData);
                if (mem::Valid(classInfo) && !fn(classInfo))
                    return;
                node = mem::Read<uintptr_t>(node + layout::kNodeNext);
            }
        }
    }
}

} // namespace

namespace schemasys {

uintptr_t System() {
    const modules::Region region = modules::Find("libschemasystem.so");
    if (!region.base) {
        DBG_W("schema", "libschemasystem.so is not loaded");
        return 0;
    }

    void* handle = dlopen(region.path.c_str(), RTLD_NOLOAD | RTLD_NOW);
    if (!handle) {
        DBG_E("schema", "dlopen(RTLD_NOLOAD) failed for %s: %s",
              region.path.c_str(), dlerror());
        return 0;
    }

    const auto createInterface =
        reinterpret_cast<CreateInterfaceFn>(dlsym(handle, "CreateInterface"));
    if (!createInterface) {
        DBG_E("schema", "libschemasystem.so has no CreateInterface export");
        return 0;
    }

    const uintptr_t sys = reinterpret_cast<uintptr_t>(createInterface("SchemaSystem_001", nullptr));
    if (!mem::Valid(sys)) {
        DBG_E("schema", "CreateInterface(SchemaSystem_001) returned nothing");
        return 0;
    }

    DBG_OK("schema", "SchemaSystem_001 @ 0x%llX", static_cast<unsigned long long>(sys));
    return sys;
}

uintptr_t TypeScope(const char* moduleSoName) {
    const uintptr_t sys = System();
    if (!sys)
        return 0;

    const uintptr_t vec = sys + layout::kTypeScopesVector;
    const int count = mem::Read<int>(vec + layout::kVectorCount);
    const uintptr_t data = mem::Read<uintptr_t>(vec + layout::kVectorData);

    if (count <= 0 || count > 256 || !mem::Valid(data)) {
        DBG_E("schema", "type scope vector looks wrong (count=%d data=0x%llX)",
              count, static_cast<unsigned long long>(data));
        return 0;
    }

    for (int i = 0; i < count; ++i) {
        const uintptr_t scope = mem::Read<uintptr_t>(data + 8ull * i);
        if (!mem::Valid(scope))
            continue;

        const std::string name = mem::ReadString(scope + layout::kScopeName, 64);
        if (name == moduleSoName) {
            DBG_OK("schema", "type scope '%s' @ 0x%llX",
                   name.c_str(), static_cast<unsigned long long>(scope));
            return scope;
        }
    }

    DBG_E("schema", "no type scope named '%s'", moduleSoName);
    return 0;
}

int ClassCount(uintptr_t typeScope) {
    int count = 0;
    ForEachClass(typeScope, [&](uintptr_t) {
        ++count;
        return true;
    });
    return count;
}

ptrdiff_t FieldOffset(uintptr_t typeScope, const char* className, const char* fieldName) {
    ptrdiff_t result = -1;

    ForEachClass(typeScope, [&](uintptr_t classInfo) {
        char name[128]{};
        if (!ReadNamePtr(classInfo + layout::kClassName, name, sizeof(name)))
            return true;
        if (std::strcmp(name, className) != 0)
            return true;

        const int16_t fieldCount = mem::Read<int16_t>(classInfo + layout::kClassFieldCount);
        const uintptr_t fields = mem::Read<uintptr_t>(classInfo + layout::kClassFields);
        if (fieldCount <= 0 || !mem::Valid(fields))
            return true;

        for (int f = 0; f < fieldCount; ++f) {
            const uintptr_t field = fields + static_cast<ptrdiff_t>(f) * layout::kFieldStride;

            char fname[128]{};
            if (!ReadNamePtr(field + layout::kFieldName, fname, sizeof(fname)))
                continue;
            if (std::strcmp(fname, fieldName) != 0)
                continue;

            result = mem::Read<int32_t>(field + layout::kFieldOffset);
            return false;
        }
        return true;
    });

    return result;
}

} // namespace schemasys
