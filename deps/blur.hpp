#pragma once

// D3D11 compatibility shim for the Neverlose ImGui fork.
// The original source includes a DX9 blur implementation here. The UI fork
// only needs this declaration; our D3D11 implementation lives in src/ui/nl_blur.cpp.
struct ImDrawList;
void DrawBackgroundBlur(ImDrawList* drawList);
