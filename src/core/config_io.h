#pragma once
#include <cstddef>

// Config persistence.
//
// cfg::Config is a plain aggregate - bools, ints, floats, fixed arrays and
// ImVec4, no pointers and no heap - so a slot is the struct itself preceded by
// a magic number, a layout version and its size. That last field is what makes
// this safe: change the struct and every old slot is rejected instead of being
// reinterpreted into garbage.
namespace configio {

constexpr int kSlots = 6;

// %TEMP%\4fir\configs. Created on demand; never null.
const char* Directory();

bool Exists(int slot);

// `error` receives a human-readable reason on failure.
bool Save(int slot, char* error, size_t errorLen);
bool Load(int slot, char* error, size_t errorLen);
bool Remove(int slot, char* error, size_t errorLen);

// Restores every setting to its compiled-in default.
void ResetToDefaults();

} // namespace configio
