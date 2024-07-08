#include <iostream>
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <thread>

#include <windowsx.h>
#include"../../DMALibrary/Memory/Memory.h"

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_impl_win32.h"

#include "Vector.h"
#include "Render.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 1;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    switch (message) {
    case WM_NCHITTEST: {
        const LONG borderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
        const LONG titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
        POINT cursorPos = { GET_X_LPARAM(w_param), GET_Y_LPARAM(l_param) };
        RECT windowRect;
        GetWindowRect(window, &windowRect);

        if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titleBarHeight) {
            return HTCAPTION;
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(window, message, w_param, l_param);
}

namespace Offsets {
    //offsets
    uint64_t dwEntityList = 0x19BDD78;

    uint64_t m_hPlayerPawn = 0x7DC;
    uint64_t dwLocalPlayerController = 0x1A0D9A8;
    uint64_t m_lifeState = 0x328;
    uint64_t dwLocalPlayerPawn = 0x1823A08;
    //uint64_t m_iTeamNum = 0x3C3;



}





    uint64_t m_vOldOrigin = 0x1274;
    uint64_t dwViewMatrix = 0x1A1FCD0;

void CreateConsole() {
    AllocConsole();
    FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONOUT$", "w", stderr);
    freopen_s(&file, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, int cmd_show)
{


    CreateConsole();
    std::cout << "Console allocated successfully!" << std::endl;

    if (!mem.Init("cs2.exe", true, true))
    {
        std::cerr << "Failed to initialize DMA" << std::endl;
        return 1;
    }
    std::cout << "DMA initialized" << std::endl;

    uintptr_t base = mem.GetBaseDaddy("client.dll");
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_procedure;
    wc.hInstance = instance;
    wc.lpszClassName = L"External Overlay Class";

    RegisterClassExW(&wc);

    const HWND overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        L"External Overlay",
        WS_POPUP,
        0,
        0,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);
    {
        RECT client_area{};
        GetClientRect(overlay, &client_area);

        RECT window_area{};
        GetWindowRect(overlay, &window_area);

        POINT diff{};
        ClientToScreen(overlay, &diff);

        const MARGINS margins
        {
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        DwmExtendFrameIntoClientArea(overlay, &margins);
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 60U; //fps
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = overlay;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL levels[2]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* device_context{ nullptr };
    IDXGISwapChain* swap_chain{ nullptr };
    ID3D11RenderTargetView* render_target_view{ nullptr };
    D3D_FEATURE_LEVEL level{};

    D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        levels,
        2U,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        &level,
        &device_context
    );

    ID3D11Texture2D* back_buffer{ nullptr };
    swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (back_buffer)
    {
        device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        back_buffer->Release();
    }
    else
        return 1;

    ShowWindow(overlay, cmd_show);
    UpdateWindow(overlay);

    ImGui::CreateContext();
    ImGui::StyleColorsClassic();

    ImGui_ImplWin32_Init(overlay);
    ImGui_ImplDX11_Init(device, device_context);


    bool running = true;

    while (running)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                running = false;
            }
        }

        if (!running)
        {
            break;
        }


        //read LocalPlayer
        uint64_t LocalPlayer = mem.Read<uint64_t>(base + Offsets::dwLocalPlayerPawn);

        uint64_t entityList = mem.Read<uint64_t>(base + Offsets::dwEntityList);
        if (entityList == 0)
            continue;

        uint64_t listEntry = mem.Read<uint64_t>(entityList + 0x10);
        if (listEntry == 0)
            continue;

        //read View_Matrix
        View_matrix_t View_Matrix = mem.Read<View_matrix_t>(base + dwViewMatrix);
        //read localPlayerPos
        Vector3 localplayerpos = mem.Read<Vector3>(LocalPlayer + m_vOldOrigin);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        for (int i = 0; i < 64; i++)
        {
            if (listEntry == 0) // skip if invalid
                continue;

            uint64_t currentController = mem.Read<uint64_t>(listEntry + i * 0x78);
            if (currentController == 0)
                continue; // skip if invalid

            uint64_t pawnHandle = mem.Read<uint64_t>(currentController + Offsets::m_hPlayerPawn);
            if (pawnHandle == 0)
                continue; // skip if invalid

            uint64_t listEntry2 = mem.Read<uint64_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
            if (listEntry2 == 0)
                continue;

            uint64_t currentPawn = mem.Read<uint64_t>(listEntry2 + 0x78 * (pawnHandle & 0x1FF));
            if (currentPawn == 0)
                continue;

            uint64_t lifestate = mem.Read<uint64_t>(currentPawn + Offsets::m_lifeState);
            if (lifestate != 256)
            {
                continue;
            }
            if (currentPawn == LocalPlayer)
            {
                continue;
            }
            //reading enemy pos
            Vector3 enemyPos = mem.Read<Vector3>(currentPawn + m_vOldOrigin);


            Vector3 head = { enemyPos.x, enemyPos.y, enemyPos.z + 75.f };

            Vector3 screenPos = enemyPos.WTS(View_Matrix);
            Vector3 screenHead = head.WTS(View_Matrix);

            float height = screenPos.y - screenHead.y;
            float width = height / 2.4f;

            RGB enemy = { 255, 0, 0 };


            //ImGui::GetBackgroundDrawList()->AddCircleFilled({ 500, 500 }, 10.f, ImColor(1.f, 0.f, 0.f));
            //draws box around enemy
            Render::DrawRect(screenHead.x - width / 2, screenHead.y, width, height, enemy, 1.5);
        }

        ImGui::Render();
        float color[4]{ 0, 0, 0, 0 };
        device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_context->ClearRenderTargetView(render_target_view, color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swap_chain->Present(1U, 0U); //if (1U, 0U) you replace that 0 with a 1 vsync will be on!
    }

    //exiting
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX11_Shutdown();

    ImGui::DestroyContext();

    if (swap_chain)
    {
        swap_chain->Release();
    }
    if (device_context)
    {
        device_context->Release();
    }
    if (device)
    {
        device->Release();
    }
    if (render_target_view)
    {
        render_target_view->Release();
    }

    DestroyWindow(overlay);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}