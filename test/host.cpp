// Test harness: stands in for CS2 so the overlay can be exercised without the
// game. It creates a real D3D11 swapchain, loads a stub named client.dll so the
// payload's module wait succeeds, injects the payload with LoadLibrary, and
// presents frames for a few seconds.
//
// The ESP will read garbage through the stub's base address; that is the point.
// It proves the SEH guard holds and that the console reports stale offsets
// instead of crashing.

#include <Windows.h>
#include <d3d11.h>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")

namespace {

IDXGISwapChain*         g_swap    = nullptr;
ID3D11Device*           g_device  = nullptr;
ID3D11DeviceContext*    g_context = nullptr;
ID3D11RenderTargetView* g_rtv     = nullptr;

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

} // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int) {
    const int seconds = 8;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"SDL_app";           // same class CS2 uses
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"overlay test host",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                80, 80, 900, 560, nullptr, nullptr, inst, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 900;
    sd.BufferDesc.Height = 560;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL level{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_swap, &g_device, &level, &g_context);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"D3D11 device creation failed", L"test host", MB_ICONERROR);
        return 1;
    }

    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back));
    g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
    back->Release();

    // Stub first: the payload waits for this module before hooking anything.
    if (!LoadLibraryW(L"client.dll")) {
        MessageBoxW(nullptr, L"client.dll stub missing - run build_test.bat",
                    L"test host", MB_ICONERROR);
        return 1;
    }
    if (!LoadLibraryW(L"cs2_overlay.dll")) {
        MessageBoxW(nullptr, L"cs2_overlay.dll missing", L"test host", MB_ICONERROR);
        return 1;
    }

    const DWORD deadline = GetTickCount() + seconds * 1000;
    const float clear[4] = { 0.08f, 0.09f, 0.12f, 1.f };

    MSG msg{};
    while (GetTickCount() < deadline) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                goto done;
        }
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        g_swap->Present(1, 0);
    }

done:
    if (g_rtv)     g_rtv->Release();
    if (g_swap)    g_swap->Release();
    if (g_context) g_context->Release();
    if (g_device)  g_device->Release();
    return 0;
}
