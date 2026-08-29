#pragma once
#include <cstddef>
#include <cstdint>

namespace sig {

struct Region {
    uintptr_t base = 0;   // module base
    uintptr_t start = 0;  // first byte of the executable section
    size_t    size = 0;
};

// Locates a module and its executable section. `base` is 0 on failure.
Region Executable(const char* moduleName);

// Finds the first match of an IDA-style pattern ("48 89 0D ? ? ? ? E9").
// Returns the absolute address of the first matched byte, or 0.
uintptr_t Find(const Region& region, const char* pattern);

// Resolves a RIP-relative operand: `at` is the start of the instruction,
// `relOffset` where its int32 displacement sits, `instrLen` its total length.
// Returns the absolute address it points at, or 0.
uintptr_t RipTarget(uintptr_t at, int relOffset, int instrLen);

// Reads a plain uint32 embedded in an instruction (a struct member offset).
uint32_t Imm32(uintptr_t at, int offset);

} // namespace sig
