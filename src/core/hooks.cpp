#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iterator>

#include "MinHook.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "core/globals.h"
#include "core/hooks.h"
#include "core/log.h"
#include "features/aimbot.h"
#include "features/esp.h"
#include "core/game_hooks.h"
#include "core/safe_settings.h"
#include "features/world.h"
#include "ui/menu.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

PresentFn       oPresent       = nullptr;
ResizeBuffersFn oResizeBuffers = nullptr;

bool imguiReady = false;
bool shuttingDown = false;

// ---------------------------------------------------------------------------
// Window discovery
// ---------------------------------------------------------------------------
BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER))
        return TRUE;

    *reinterpret_cast<HWND*>(lParam) = hwnd;
    return FALSE;
}

HWND FindGameWindow() {
    HWND hwnd = nullptr;
    // CS2 runs on SDL; try the known class first, then fall back to a scan.
    hwnd = FindWindowW(L"SDL_app", nullptr);
    if (!hwnd)
        EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&hwnd));
    return hwnd;
}

// ---------------------------------------------------------------------------
// Render target helpers
// ---------------------------------------------------------------------------
void CreateRenderTarget(IDXGISwapChain* swapChain) {
    ID3D11Texture2D* backBuffer = nullptr;
    const HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                            reinterpret_cast<void**>(&backBuffer));
    if (SUCCEEDED(hr) && backBuffer) {
        const HRESULT rtvHr = g::device->CreateRenderTargetView(backBuffer, nullptr, &g::rtv);
        if (FAILED(rtvHr))
            DBG_EVERY(2000, dbg::Level::Error, "d3d11",
                      "CreateRenderTargetView failed hr=0x%08lX", rtvHr);
        backBuffer->Release();
    } else {
        DBG_EVERY(2000, dbg::Level::Error, "d3d11",
                  "swapchain GetBuffer failed hr=0x%08lX", hr);
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    if (SUCCEEDED(swapChain->GetDesc(&desc))) {
        const int w = static_cast<int>(desc.BufferDesc.Width);
        const int h = static_cast<int>(desc.BufferDesc.Height);
        if (w != g::screenW || h != g::screenH)
            DBG_I("d3d11", "backbuffer %dx%d  windowed=%d  format=%d",
                  w, h, desc.Windowed ? 1 : 0, static_cast<int>(desc.BufferDesc.Format));
        g::screenW = w;
        g::screenH = h;
    }
}

void ReleaseRenderTarget() {
    if (g::rtv) {
        g::rtv->Release();
        g::rtv = nullptr;
    }
}

// ---------------------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (imguiReady && g::menuOpen) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        // Swallow input while the menu owns it so the game does not also react.
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard)
            return TRUE;
    }
    return CallWindowProcW(g::oWndProc, hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// ImGui bootstrap
// ---------------------------------------------------------------------------
void InitImGui(IDXGISwapChain* swapChain) {
    DBG_I("imgui", "first Present intercepted, initialising ...");

    const HRESULT hr = swapChain->GetDevice(__uuidof(ID3D11Device),
                                            reinterpret_cast<void**>(&g::device));
    if (FAILED(hr)) {
        DBG_EVERY(5000, dbg::Level::Error, "imgui",
                  "swapchain has no D3D11 device (hr=0x%08lX) - game on Vulkan?", hr);
        return;
    }

    g::device->GetImmediateContext(&g::context);
    DBG_OK("imgui", "device 0x%llX  context 0x%llX",
           static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(g::device)),
           static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(g::context)));

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    g::window = desc.OutputWindow ? desc.OutputWindow : FindGameWindow();
    if (!g::window) {
        DBG_E("imgui", "no game window found - input will not work");
    } else {
        wchar_t cls[64]{};
        GetClassNameW(g::window, cls, 64);
        DBG_OK("imgui", "window 0x%llX  class '%ls'",
               static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(g::window)), cls);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    menu::ApplyStyle();

    ImGui_ImplWin32_Init(g::window);
    ImGui_ImplDX11_Init(g::device, g::context);

    g::oWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g::window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

    CreateRenderTarget(swapChain);
    imguiReady = true;
    DBG_OK("imgui", "overlay online (ImGui %s)", IMGUI_VERSION);
}

void PollHotkeys() {
    // The menu key is user-bound and can now land on F9/F10/END, which the
    // fixed hotkeys below already use. Whoever owns the key wins, and the menu
    // owns it - otherwise binding F9 would dump entities on every open.
    const int menuKey = cfg::c.menuKey;

    static bool menuKeyDown = false;
    const bool down = (GetAsyncKeyState(menuKey) & 0x8000) != 0;
    if (down && !menuKeyDown) {
        g::menuOpen = !g::menuOpen;
        DBG_T("input", "menu %s", g::menuOpen ? "opened" : "closed");
    }
    menuKeyDown = down;

    static bool dumpKeyDown = false;
    const bool dumpDown =
        menuKey != VK_F9 && (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (dumpDown && !dumpKeyDown)
        esp::Dump();
    dumpKeyDown = dumpDown;

    static bool boneKeyDown = false;
    const bool boneDown =
        menuKey != VK_F10 && (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    if (boneDown && !boneKeyDown)
        esp::DumpBones();
    boneKeyDown = boneDown;

    static bool endKeyDown = false;
    const bool endDown =
        menuKey != VK_END && (GetAsyncKeyState(VK_END) & 0x8000) != 0;
    if (endDown && !endKeyDown) {
        DBG_W("input", "END pressed - queueing unload");
        cfg::c.unloadQueued = true;
    }
    endKeyDown = endDown;
}

// ---------------------------------------------------------------------------
// Detours
// ---------------------------------------------------------------------------
HRESULT WINAPI hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    if (shuttingDown)
        return oPresent(swapChain, syncInterval, flags);

    if (!imguiReady)
        InitImGui(swapChain);

    if (imguiReady) {
        if (!g::rtv)
            CreateRenderTarget(swapChain);

        PollHotkeys();

        // The game hooks need CCSGOInput, which only exists once the client
        // has spun up. Retried from here about once a second until it takes,
        // then this is a single bool test per frame.
        if (!gamehooks::state.installed) {
            static uint32_t lastTry = 0;
            const uint32_t now = GetTickCount();
            if (now - lastTry > 1000) {
                lastTry = now;
                gamehooks::Install();
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::GetIO().MouseDrawCursor = g::menuOpen;

        // Aim runs before the frame is drawn so the markers match what it did.
        // The ragebot has no UI and is not driven from here any more.
        // The guard rail runs first so the aimbot never reads a value it is
        // about to have taken away.
        safeset::Sync();
        aimbot::Run();

        if (cfg::c.masterSwitch) {
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            world::Run();
            esp::Render(bg);
            world::Draw(bg);
            aimbot::Draw(bg, g::screenW, g::screenH);
        }

        menu::Render();

        ImGui::Render();
        g::context->OMSetRenderTargets(1, &g::rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        DBG_ONCE(dbg::Level::Ok, "render", "first frame drawn through the hook");
    }

    return oPresent(swapChain, syncInterval, flags);
}

HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, UINT width,
                               UINT height, DXGI_FORMAT format, UINT flags) {
    DBG_I("d3d11", "ResizeBuffers -> %ux%u", width, height);
    ReleaseRenderTarget();
    const HRESULT hr = oResizeBuffers(swapChain, bufferCount, width, height, format, flags);
    if (imguiReady)
        CreateRenderTarget(swapChain);
    return hr;
}

// ---------------------------------------------------------------------------
// Grab the IDXGISwapChain vtable from a throwaway device.
// ---------------------------------------------------------------------------
bool GetSwapChainVTable(void** outVTable, size_t count) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"dummy_wnd_cls";
    if (!RegisterClassExW(&wc))
        return false;

    HWND dummy = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                                 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (!dummy) {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = dummy;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL obtained{};

    IDXGISwapChain*      sc  = nullptr;
    ID3D11Device*        dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;

    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels,
        static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION,
        &sd, &sc, &dev, &obtained, &ctx);

    bool ok = false;
    if (SUCCEEDED(hr) && sc) {
        memcpy(outVTable, *reinterpret_cast<void***>(sc), count * sizeof(void*));
        ok = true;
        DBG_OK("hook", "dummy swapchain ok, feature level 0x%04X",
               static_cast<unsigned>(obtained));
    } else {
        DBG_E("hook", "D3D11CreateDeviceAndSwapChain failed hr=0x%08lX", hr);
    }

    if (ctx) ctx->Release();
    if (dev) dev->Release();
    if (sc)  sc->Release();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

} // namespace

namespace hooks {

bool Init() {
    void* vtable[18]{};
    if (!GetSwapChainVTable(vtable, std::size(vtable))) {
        DBG_E("hook", "could not read the IDXGISwapChain vtable");
        return false;
    }

    DBG_I("hook", "Present       @ 0x%llX",
          static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(vtable[8])));
    DBG_I("hook", "ResizeBuffers @ 0x%llX",
          static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(vtable[13])));

    MH_STATUS st = MH_Initialize();
    if (st != MH_OK) {
        DBG_E("hook", "MH_Initialize: %s", MH_StatusToString(st));
        return false;
    }

    // 8 = Present, 13 = ResizeBuffers
    st = MH_CreateHook(vtable[8], reinterpret_cast<void*>(&hkPresent),
                       reinterpret_cast<void**>(&oPresent));
    if (st != MH_OK) {
        DBG_E("hook", "MH_CreateHook(Present): %s", MH_StatusToString(st));
        return false;
    }

    st = MH_CreateHook(vtable[13], reinterpret_cast<void*>(&hkResizeBuffers),
                       reinterpret_cast<void**>(&oResizeBuffers));
    if (st != MH_OK) {
        DBG_E("hook", "MH_CreateHook(ResizeBuffers): %s", MH_StatusToString(st));
        return false;
    }

    st = MH_EnableHook(MH_ALL_HOOKS);
    if (st != MH_OK) {
        DBG_E("hook", "MH_EnableHook: %s", MH_StatusToString(st));
        return false;
    }

    DBG_OK("hook", "detours armed, waiting for the first Present");
    return true;
}

void Shutdown() {
    shuttingDown = true;
    DBG_I("hook", "disabling detours");

    MH_DisableHook(MH_ALL_HOOKS);
    // Let any in-flight Present return through the trampoline before we free it.
    Sleep(200);
    MH_Uninitialize();

    if (g::oWndProc && g::window) {
        SetWindowLongPtrW(g::window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g::oWndProc));
        g::oWndProc = nullptr;
    }

    if (imguiReady) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiReady = false;
    }

    ReleaseRenderTarget();
    if (g::context) { g::context->Release(); g::context = nullptr; }
    if (g::device)  { g::device->Release();  g::device  = nullptr; }
    DBG_OK("hook", "detours removed, WndProc restored");
}

} // namespace hooks
