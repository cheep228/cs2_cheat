#pragma once

namespace hooks {

// Installs the DXGI Present/ResizeBuffers detours and the WndProc hook.
bool Init();

// Removes every detour and tears down ImGui. Safe to call twice.
void Shutdown();

} // namespace hooks
