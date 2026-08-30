#include "imgui.h"
#include "../../deps/blur.hpp"

// NL PORT: D3D11 compatibility.
// The supplied blur.cpp is a DX9 renderer and its compiled shaders cannot be
// bound to the project's D3D11 device. Keep the exact UI call site intact so a
// D3D11 blur pass can be dropped in without touching the Neverlose ImGui fork.
// For now the normal translucent Neverlose window layers are rendered and this
// hook intentionally performs no additional pass.
void DrawBackgroundBlur(ImDrawList* drawList) {
    (void)drawList;
}
