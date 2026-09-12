#pragma once
#include <cstdint>

#include "modules.h"

// Byte-pattern scanner over an ELF module's executable mapping. Same IDA-style
// pattern syntax and the same three-function shape as the Windows project's
// sdk/sigscan.h, so the same kind of signature table can drive it - but the
// actual bytes are NOT portable between the Windows and Linux builds (they
// come from different compilers), so no patterns are baked in here. See
// linux/README.md for where real ones have to come from.
namespace sig {

using Region = modules::Region;

// Finds the first match of "48 89 0D ? ? ? ? E9" style pattern. Returns the
// absolute address of the first matched byte, or 0.
uintptr_t Find(const Region& region, const char* pattern);

// Resolves a RIP-relative operand: `at` is the start of the instruction,
// `relOffset` where its int32 displacement sits, `instrLen` its total length.
uintptr_t RipTarget(uintptr_t at, int relOffset, int instrLen);

// Reads a plain uint32 embedded in an instruction (a struct member offset).
uint32_t Imm32(uintptr_t at, int offset);

} // namespace sig
