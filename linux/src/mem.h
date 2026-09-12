#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

// In-process memory access. This runs injected into the game's own address
// space (LD_PRELOAD), so "reading remote memory" is just dereferencing a
// pointer - the only real hazard is a stale/wrong offset pointing at
// unmapped memory, which CopyBytes/WriteBytes survive via a SIGSEGV guard
// instead of taking the game down with us.
namespace mem {

inline bool Valid(uintptr_t p) {
    return p > 0x10000 && p < 0x0000800000000000ull;
}

// Reads that hit unmapped memory. A rising count means an offset is wrong.
inline unsigned long faults = 0;

// Installs the fault guard. Called lazily by CopyBytes/WriteBytes; safe to
// call more than once or from more than one thread.
void InstallGuard();

bool CopyBytes(uintptr_t address, void* dst, size_t size);
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

// Writes a NUL-terminated ASCII string into a fixed-size char[cap] field,
// truncating and always leaving the buffer NUL-terminated.
bool WriteString(uintptr_t address, const std::string& value, size_t cap);

} // namespace mem
