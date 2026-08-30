#pragma once

// Shim for the quint source tree's own <includes.h>.
//
// hell_gui expects this header to have pulled in ImGui and to have defined the
// hellvec2 / hellcolor aliases before any of its files are compiled. Only the
// parts hell_gui actually uses are reproduced here; the rest of the original
// (their SDK, minhook, encryption, logging) is not needed to draw a menu.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <Windows.h>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <variant>
#include <functional>
#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <imgui_internal.h>

typedef ImVec4 hellvec4;
typedef ImVec2 hellvec2;
typedef ImColor hellcolor;

// The original tree undefines these after <Windows.h>; hell_gui relies on it
// (ImMin/ImMax and std::min/std::max appear throughout the widgets).
#undef min
#undef max

// PORT: xx() is the original tree's compile-time string encryption. This
// project does not obfuscate strings, so it passes them through unchanged.
#define xx( str ) str
