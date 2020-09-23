// WinSDK
#include <windows.h>
#include <tchar.h>
#include <vector>

// Dear imGui
#include "imgui.h"
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"

#include "dx11.h"       // imgui_impl_dx11.h
#include "win32.h"      // imgui_impl_win32.h

// DriectX 11
#pragma comment(lib,"d3d11.lib")

#include <d3d11.h>
#include <dinput.h>

// DXGI
#include <D3D11_1.h>
IDXGIDevice*       dxgiDevice       = 0;
IDXGIAdapter*      dxgiAdapter      = 0;
IDXGIFactory*      dxgiFactory      = 0;
IDXGIFactory2*     dxgiFactory2     = 0;


#define DIRECTINPUT_VERSION 0x0800

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

// Misc
WNDCLASSEX wc;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

// DirectX Objects
static ID3D11Device*            g_pd3dDevice            = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext     = NULL;
bool   is3DDeviceInitialized    = false;

// Externals
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Public Types
typedef void(*OnRenderEvent)(void);
typedef void* ViewHandle;

// Public imGUIWindow Class
class imGUIWindow
{
public:
    imGUIWindow();
    ~imGUIWindow();
    bool DynamicBuffering = true;
    bool Create();
    bool Destroy();
    HWND GetHandle();
    void SetTitle(wchar_t* title) { wTitle = title; }
    IDXGISwapChain* GetSwapChain();
    ID3D11RenderTargetView* GetRTV();
    bool Render();
    void SetRenderEvent(OnRenderEvent renderEvent)
    {
        OnRenderEventDelegate = renderEvent;
    }
    static LRESULT CALLBACK s_DlgCB(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        imGUIWindow* pThis;
        if (msg == WM_NCCREATE)
        {
            pThis = static_cast<imGUIWindow*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
            SetLastError(0);
            if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis)))
            {
                if (GetLastError() != 0) return FALSE;
            }
        }
        else
            pThis = reinterpret_cast<imGUIWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (pThis)  return pThis->DlgCB(hwnd, msg, wParam, lParam);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    static int WindowsCreated;
private:
    HWND wHandle = 0;
    RECT wRect;
    OnRenderEvent OnRenderEventDelegate = 0;
    wchar_t* wTitle;
    BOOL isRendering = false;
    IDXGISwapChain1* g_pSwapChain = NULL;
    ID3D11RenderTargetView* g_rtv = NULL;
    LRESULT WINAPI DlgCB(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    float f = 0.6f;
    int counter = 0;
};

// Current imGUI Windows
std::vector<imGUIWindow*> imgGUIWindows;

// DirectX Helper functions
bool CreateDeviceD3D()
{
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    
    if (D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, 
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    printf("DirectX11 Device has been created.\n");
    return true;
}
void CleanupDeviceD3D()
{
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}
void CreateRenderTarget(IDXGISwapChain1* g_pSwapChain, ID3D11RenderTargetView** g_rtv)
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, g_rtv);
    pBackBuffer->Release();
}

// imGui Window Class Functions
int imGUIWindow::WindowsCreated = 0;
imGUIWindow::imGUIWindow()
{
}
imGUIWindow::~imGUIWindow()
{
}
bool imGUIWindow::Create()
{
    if (!g_pd3dDevice) return false;

    wHandle = ::CreateWindow(wc.lpszClassName, wTitle, 
        WS_OVERLAPPEDWINDOW, (400 * WindowsCreated) + 15, 20, 400, 600, NULL, NULL,
        wc.hInstance, this);

    ::ShowWindow(wHandle, SW_SHOWDEFAULT);
    ::UpdateWindow(wHandle);

    DXGI_SWAP_CHAIN_DESC1 sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.Width = 0;
    sd.Height = 0;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT createSwapChain = dxgiFactory2->CreateSwapChainForHwnd(
        g_pd3dDevice,
        wHandle,
        &sd,
        nullptr,
        nullptr,
        &g_pSwapChain);

    if (createSwapChain != S_OK) return false;

    CreateRenderTarget(g_pSwapChain, &g_rtv);

    g_pd3dDeviceContext->ClearRenderTargetView(g_rtv, (float*)&clear_color);
    g_pSwapChain->Present(0, 0);

    WindowsCreated++;
    printf("UI window `%S` has been created.\n", wTitle);
    return true;
}
bool imGUIWindow::Destroy()
{
    if (g_rtv) { g_rtv->Release(); g_rtv = NULL; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    return true;
}
HWND imGUIWindow::GetHandle()
{
    return wHandle;
}
IDXGISwapChain* imGUIWindow::GetSwapChain() 
{
    return g_pSwapChain;
}
ID3D11RenderTargetView* imGUIWindow::GetRTV() 
{
    return g_rtv;
}
bool imGUIWindow::Render()
{
    ImGui_ImplWin32_Init(wHandle);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    GetClientRect(wHandle, &wRect);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(wRect.right - wRect.left, wRect.bottom - wRect.top));

    ImGui::Begin("", 0,
        ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Test Menu"))
        {
            ImGui::MenuItem("Option 1", "Ctrl + F", false);
            ImGui::MenuItem("Option 2", "Ctrl + D", false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Text("Dear ImGUI Demo App");

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

    if (ImGui::Button("+1 to counter")) counter++;

    ImGui::SameLine();
    ImGui::Text("counter : %d", counter);

    ImGui::Separator();

    ImGui::NewLine();
    ImGui::Spacing();
    ImGui::Bullet();
    ImGui::Text("Performance :%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::NewLine();
    ImGui::Separator();

    ImGui::ColorEdit4("Clear Color", (float*)&clear_color);

    ImGui::End();

    // Rendering
    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_rtv, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_rtv, (float*)&clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);

    isRendering = true;

    return true;
}
LRESULT WINAPI imGUIWindow::DlgCB(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pSwapChain != NULL && g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            if (g_rtv) { g_rtv->Release(); g_rtv = NULL; }
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget(g_pSwapChain, &g_rtv);
            if (DynamicBuffering && isRendering) Render();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        Destroy();
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

#define EXPORT_API extern "C" __declspec(dllexport)

// External API
EXPORT_API bool         InitializeGUIEngine()
{
    if (is3DDeviceInitialized) return true;

    // Create application window
    wc = { sizeof(WNDCLASSEX), CS_CLASSDC, imGUIWindow::s_DlgCB, 0L,
        0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        _T("ImGui Example"), NULL };

    ::RegisterClassEx(&wc);

    // Initialize Direct3D
    if (!CreateDeviceD3D())
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    // Get DXGI Interfaces
    HRESULT hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (!SUCCEEDED(hr)) return false;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
    if (!SUCCEEDED(hr)) return false;
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    if (!SUCCEEDED(hr)) return false;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    is3DDeviceInitialized = true;
    return true;
}
EXPORT_API ViewHandle   CreateNewViewWindow(wchar_t* vName) 
{
    imGUIWindow* newView = new imGUIWindow;
    newView->SetTitle(vName);
    newView->Create();
    imgGUIWindows.push_back(newView);
    return newView->GetHandle();
}
EXPORT_API bool         SetViewWindowOnRenderEvent(ViewHandle vHwnd, OnRenderEvent vEvent)
{
    for (size_t i = 0; i < imgGUIWindows.size(); i++)
    {
        if (imgGUIWindows[i]->GetHandle() == vHwnd)
        {
            imgGUIWindows[i]->SetRenderEvent(vEvent);
            return true;
        }
    }
    return false;
}
EXPORT_API bool         DisposeViewWindow(ViewHandle vHwnd)
{
    for (size_t i = 0; i < imgGUIWindows.size(); i++)
    {
        if (imgGUIWindows[i]->GetHandle() == vHwnd)
        {
            imgGUIWindows[i]->Destroy();
            imgGUIWindows.erase(imgGUIWindows.begin() + i);
            return true;
        }
    }
    return false;
}
EXPORT_API bool         RenderViewWindow(ViewHandle vHwnd)
{
    for (size_t i = 0; i < imgGUIWindows.size(); i++)
    {
        if (imgGUIWindows[i]->GetHandle() == vHwnd)
        {
            imgGUIWindows[i]->Render();
            return true;
        }
    }
    return false;
}