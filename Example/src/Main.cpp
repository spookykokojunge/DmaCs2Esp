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
    uint64_t dwEntityList = 0x19BDD58;
    uint64_t dwLocalPlayerController = 0x1A0D988;
    uint64_t dwLocalPlayerPawn = 0x1823A08;
    uint64_t m_vOldOrigin = 0x1274;
    uint64_t dwViewMatrix = 0x1A1FCB0;

    uint64_t m_hPlayerPawn = 0x7DC;
    uint64_t m_lifeState = 0x328;
}


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

    // Declare all variables before the loop so we dont define them over and over aigan.
    uint64_t LocalPlayer;
    uint64_t entityList;
    View_matrix_t View_Matrix;
    Vector3 localplayerpos;
    uint64_t currentController[64] = { 0 };
    uint64_t currentPawn[64] = { 0 };
    uint64_t listEntry2;
    uint64_t pawnHandle = 0;
    Vector3 enemyPos;
    RGB enemy = { 255, 0, 0 };
    uint64_t lifestate;
    uint64_t listEntry;



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
        // Step 1: Create scatter handle for initial reads
        auto Handle = mem.CreateScatterHandle();

        // Step 2: Add scatter read requests for initial values
        mem.AddScatterReadRequest(Handle, base + Offsets::dwLocalPlayerPawn, &LocalPlayer, sizeof(uint64_t));
        mem.AddScatterReadRequest(Handle, base + Offsets::dwEntityList, &entityList, sizeof(uint64_t));
        mem.AddScatterReadRequest(Handle, base + Offsets::dwViewMatrix, &View_Matrix, sizeof(View_matrix_t));

        // Execute the scatter read
        mem.ExecuteReadScatter(Handle);

        // Read local player position
        if (LocalPlayer != 0) {
            mem.AddScatterReadRequest(Handle, LocalPlayer + Offsets::m_vOldOrigin, &localplayerpos, sizeof(Vector3));
            mem.ExecuteReadScatter(Handle);
            //std::cout << "LOCALPLAYER X: " << localplayerpos.x << " Y: " << localplayerpos.y << " Z: " << localplayerpos.z << std::endl;

        }

        if (entityList == 0)
            continue;
        mem.AddScatterReadRequest(Handle, entityList + 0x10, &listEntry, sizeof(uint64_t));
        mem.ExecuteReadScatter(Handle);
        //uint64_t listEntry = mem.Read<uint64_t>(entityList + 0x10);
        if (listEntry == 0)
            continue;



        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        // Step 3: Create scatter handle for entity list reads
        for (int i = 0; i < 64; i++)
        {
            mem.AddScatterReadRequest(Handle, listEntry + i * 0x78, &currentController[i], sizeof(uint64_t));
        }

        // Execute the scatter read
        mem.ExecuteReadScatter(Handle);

        for (int i = 0; i < 64; i++)
        {
            if (currentController[i] == 0) // skip if invalid
                continue;

            // Create scatter handle for pawn handle reads
            mem.AddScatterReadRequest(Handle, currentController[i] + Offsets::m_hPlayerPawn, &pawnHandle, sizeof(uint64_t));
            mem.ExecuteReadScatter(Handle);

            if (pawnHandle == 0)
                continue; // skip if invalid
            mem.AddScatterReadRequest(Handle, entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10, &listEntry2, sizeof(uint64_t));
            mem.ExecuteReadScatter(Handle);
            //uint64_t listEntry2 = mem.Read<uint64_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
            if (listEntry2 == 0)
                continue;

            // Create scatter handle for current pawn reads
            mem.AddScatterReadRequest(Handle, listEntry2 + 0x78 * (pawnHandle & 0x1FF), &currentPawn[i], sizeof(uint64_t));
            mem.ExecuteReadScatter(Handle);

            if (currentPawn[i] == 0)
                continue;


            mem.AddScatterReadRequest(Handle, currentPawn[i] + Offsets::m_lifeState, &lifestate, sizeof(uint64_t));
            mem.ExecuteReadScatter(Handle);

            //uint64_t lifestate = mem.Read<uint64_t>(currentPawn[i] + Offsets::m_lifeState);
            if (lifestate != 256)
            {
                continue;
            }
            if (currentPawn[i] == LocalPlayer)
            {
                continue;
            }

            // Create scatter handle for enemy position reads
            mem.AddScatterReadRequest(Handle, currentPawn[i] + Offsets::m_vOldOrigin, &enemyPos, sizeof(Vector3));
            mem.ExecuteReadScatter(Handle);
            //We're done with using the scatter handle...
            //mem.CloseScatterHandle(Handle);
            //std::cout << "ENEMYS X: " << enemyPos.x << " Y: " << enemyPos.y << " Z: " << enemyPos.z << std::endl;


            Vector3 head = { enemyPos.x, enemyPos.y, enemyPos.z + 75.f };

            Vector3 screenPos = enemyPos.WTS(View_Matrix);
            Vector3 screenHead = head.WTS(View_Matrix);

            float height = screenPos.y - screenHead.y;
            float width = height / 2.4f;



            //ImGui::GetBackgroundDrawList()->AddCircleFilled({ 500, 500 }, 10.f, ImColor(1.f, 0.f, 0.f));
            // draws box around enemy
            Render::DrawRect(screenHead.x - width / 2, screenHead.y, width, height, enemy, 1.5);

        }
        ImGui::Render();
        float color[4]{ 0, 0, 0, 0 };
        device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_context->ClearRenderTargetView(render_target_view, color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swap_chain->Present(1U, 0U); //if (1U, 0U) you replace that 0 with a 1 vsync will be on!
        //We're done with using the scatter handle...
        mem.CloseScatterHandle(Handle);
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
