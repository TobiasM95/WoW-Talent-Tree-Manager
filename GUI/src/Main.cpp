/*
    WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
    Copyright(C) 2022 Tobias Mielich

    This program is free software : you can redistribute it and /or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see < https://www.gnu.org/licenses/>.

    Contact via https://github.com/TobiasM95/WoW-Talent-Tree-Manager/discussions or BuffMePls#2973 on Discord
*/

#include <memory>
#include <filesystem>
#include <chrono>
#include <thread>

#include "curl.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

#include "TalentTreeManager.h"
#include "TalentTreeManagerDefinitions.h"
#include "Updater.h"
#include "resource.h"
#include "roboto_medium.h"

#include "TTMGUIPresets.h"

#define WM_SAVEBEFOREDESTROY (WM_APP + 0)

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
//int main(int, char**)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    //set working directory to the file directory
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::filesystem::path filedirpath{ buffer };
    std::filesystem::current_path(filedirpath.parent_path());
    std::filesystem::path cwd = std::filesystem::current_path();

    //if this gets changed check Updater.h of AppUpdater project as well
    std::filesystem::path tempUpdaterFile{ "AppUpdaterTemp.exe" };
    if (std::filesystem::is_regular_file(tempUpdaterFile)) {
        try {
            std::filesystem::remove(tempUpdaterFile);
        }
        catch (const std::filesystem::filesystem_error& e) {
            ImGui::LogText(e.what());
        }
    }

    //init curl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Talent Tree Manager"), WS_OVERLAPPEDWINDOW, 100, 100, 1600, 900, NULL, NULL, wc.hInstance, NULL);
    
    WINDOWPLACEMENT wp = TTM::loadWindowPlacement();
    SetWindowPlacement(hwnd, &wp);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 1.00f);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../resources/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    //io.Fonts->AddFontDefault(); //Size 13
    float baseSizes[5] = { 11.0f, 14.0f, 17.0f, 22.0f, 27.0f };
    for (auto& size : baseSizes) {
        io.Fonts->AddFontFromMemoryCompressedTTF(TTMFonts::Roboto_Medium_compressed_data, TTMFonts::Roboto_Medium_compressed_size, size);
        io.Fonts->AddFontFromMemoryCompressedTTF(TTMFonts::Roboto_Medium_compressed_data, TTMFonts::Roboto_Medium_compressed_size, size + 3.0f);
        io.Fonts->AddFontFromMemoryCompressedTTF(TTMFonts::Roboto_Medium_compressed_data, TTMFonts::Roboto_Medium_compressed_size, size + 10.0f);
        io.Fonts->AddFontFromMemoryCompressedTTF(TTMFonts::Roboto_Medium_compressed_data, TTMFonts::Roboto_Medium_compressed_size, size - 3.0f);
    }

    int banner_width = 0;
    int banner_height = 0;
    ID3D11ShaderResourceView* TTMBannerRV;
    bool banner_success = TTM::LoadTTMBanner(&TTMBannerRV, &banner_width, &banner_height, g_pd3dDevice);

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));

    Presets::PUSH_FONT(Presets::FONTSIZE::DEFAULT, 0);
    if (ImGui::Begin("MainWindow", nullptr, ImGuiWindowFlags_NoDecoration))
    {
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        ImGui::Text("Initialize workspace (can take a while on first open) and load resources...");
        if (banner_success) {
            ImGui::SetCursorPos(ImVec2(0.5f * contentRegion.x - 0.5f * banner_width, 0.5f * contentRegion.y - 0.5f * banner_height));
            ImGui::Image(
                TTMBannerRV,
                ImVec2(static_cast<float>(banner_width), static_cast<float>(banner_height)),
                ImVec2(0, 0), ImVec2(1, 1),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
            );
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    Presets::POP_FONT();
    ImGui::Render();
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0); // Present with vsync
    //g_pSwapChain->Present(0, 0); // Present without vsync
    

    std::string IniPath = std::string((Presets::getAppPath() / "imgui.ini").string());
    std::string LogPath = std::string((Presets::getAppPath() / "imgui_log.txt").string());
    io.IniFilename = IniPath.c_str();
    io.LogFilename = LogPath.c_str();

    TTM::initWorkspace();

    TTM::UIData uiData;
    uiData.g_pd3dDevice = g_pd3dDevice;
    uiData.hwnd = &hwnd;
    TTM::refreshIconMap(uiData);
    TTM::TalentTreeCollection talentTreeCollection = TTM::loadWorkspace(uiData);
    talentTreeCollection.presets = Presets::LOAD_PRESETS();
    TTM::loadActiveIcons(uiData, talentTreeCollection, true);

    // Main loop
    bool done = false;
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            if (msg.message == WM_SAVEBEFOREDESTROY && !uiData.resetWorkspace) {
                TTM::saveWorkspace(uiData, talentTreeCollection);
                TTM::stopAllSolvers(talentTreeCollection);
                TTM::updateSolverStatus(uiData, talentTreeCollection, true);
                while (uiData.currentSolvers.size() > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    TTM::updateSolverStatus(uiData, talentTreeCollection, true);
                }
                ::DefWindowProc(hwnd, WM_CLOSE, 0, 0);
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Presets::PUSH_FONT(uiData.fontsize, 0);

        currentTime = std::chrono::high_resolution_clock::now();
        if (currentTime - uiData.lastSaveTime > uiData.autoSaveInterval) {
            uiData.lastSaveTime = currentTime;
            TTM::saveWorkspace(uiData, talentTreeCollection);
        }

        if (!(uiData.updateStatus == TTM::UpdateStatus::UPTODATE || uiData.updateStatus == TTM::UpdateStatus::UPDATEERROR || uiData.updateStatus == TTM::UpdateStatus::IGNOREUPDATE)) {
            TTM::RenderUpdateWindow(uiData, talentTreeCollection);
        } 
        else {
            TTM::RenderMainWindow(uiData, talentTreeCollection, done);
        }

        Presets::POP_FONT();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    curl_global_cleanup();

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        ::PostMessage(NULL, WM_SAVEBEFOREDESTROY, 0, 0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}