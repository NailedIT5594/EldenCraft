#include <windows.h>
#include <minhook.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <stdio.h>

ID3D12Device* g_pd3dDevice = nullptr;
ID3D12CommandQueue* g_pCommandQueue = nullptr;
ID3D12CommandAllocator* g_pd3dCommandAllocator = nullptr;
ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
ID3D12Resource* g_mainRenderTargetResource = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor = {};
ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
UINT g_frameIndex = 0;
HWND g_hWnd = FindWindowA(NULL, "Elden Ring");
IDXGISwapChain3* g_pd3dSwapChain = nullptr;
bool g_isInitialized = false;
bool deviceInitialized = false;

void LogDebug(const char* fmt, ...) {
    FILE* f = fopen("C:\\Users\\CW\\Downloads\\EldenCraftRuntime\\eldencraft_debug.log", "a");
    if (f) {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        fprintf(f, "\n");
        va_end(args);
        fclose(f);
    }
}

template <typename T>
inline LPVOID Pun(T addr) {
    union { T a; LPVOID b; } u;
    u.a = addr;
    return u.b;
}

typedef HRESULT(STDMETHODCALLTYPE* Present_t)(IDXGISwapChain3*, UINT, UINT);
Present_t OriginalPresent = nullptr;

typedef void(STDMETHODCALLTYPE* ExecuteCommandLists_t)(ID3D12CommandQueue*, UINT, ID3D12GraphicsCommandList**);
ExecuteCommandLists_t OriginalExecuteCommandLists = nullptr;

HRESULT WINAPI HookedPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
    LogDebug("HookedPresent called!");
    if (!deviceInitialized) {
        LogDebug("[EldenCraft] HookedPresent triggered, initializing device cache...");
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&g_pd3dDevice))) {
            LogDebug("[EldenCraft] Device cached successfully: 0x%p", g_pd3dDevice);
            deviceInitialized = true;
        }
    }

    if (!g_isInitialized) {
        LogDebug("[EldenCraft] HookedPresent triggered, initializing ImGui...");
        // Create ImGui resources
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();

        // Setup Dear ImGui style
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 0.5f;
        }

        // Setup Platform/Renderer bindings
        ImGui_ImplWin32_Init(g_hWnd);

        // Traditional parameter-based initialization signature
        ImGui_ImplDX12_InitInfo initInfo = {};
        initInfo.Device = g_pd3dDevice;
        initInfo.CommandQueue = g_pCommandQueue;
        initInfo.NumFramesInFlight = 3;
        initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        initInfo.SrvDescriptorHeap = g_pd3dSrvDescHeap;
        initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
            // Implement your SRV descriptor allocation logic here
        };
        initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
            // Implement your SRV descriptor free logic here
        };

        if (FAILED(ImGui_ImplDX12_Init(&initInfo))) {
            LogDebug("[EldenCraft] Failed to initialize ImGui_ImplDX12.");
            return OriginalPresent(pSwapChain, SyncInterval, Flags);
        }

        g_isInitialized = true;
    }

    // Record ImGui draw data into our command list
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Custom ImGui window/HUD code
    ImGui::Begin("EldenCraft HUD");
    ImGui::Text("Hello World");
    ImGui::End();

    ImGui::Render();

    // Transition the current back buffer resource state to D3D12_RESOURCE_STATE_RENDER_TARGET (if needed), set the descriptor heaps, and call ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList)
    ID3D12Resource* backBuffer = nullptr;
    if (FAILED(g_pd3dSwapChain->GetBuffer(g_frameIndex, IID_PPV_ARGS(&backBuffer)))) {
        LogDebug("[EldenCraft] Failed to get back buffer.");
        return OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    g_pd3dCommandList->ResourceBarrier(1, &barrier);

    const float clearColor[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
    g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor, clearColor, 0, nullptr);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);

    // Close our ImGui command list and execute it alongside or right before the game's command list
    g_pd3dCommandList->Close();
    OriginalExecuteCommandLists(g_pCommandQueue, 1, &g_pd3dCommandList);

    // Reset the command allocator and command list for the next frame
    if (FAILED(g_pd3dCommandAllocator->Reset())) {
        LogDebug("[EldenCraft] Failed to reset command allocator for ImGui.");
    }

    if (FAILED(g_pd3dCommandList->Reset(g_pd3dCommandAllocator, nullptr))) {
        LogDebug("[EldenCraft] Failed to reset command list for ImGui.");
    }

    return OriginalPresent(pSwapChain, SyncInterval, Flags);
}

void HookFunctions() {
    // Define explicit function pointer typedefs
    typedef HRESULT(STDMETHODCALLTYPE* Present_t)(IDXGISwapChain3*, UINT, UINT);
    typedef void(STDMETHODCALLTYPE* ExecuteCommandLists_t)(ID3D12CommandQueue*, UINT, ID3D12GraphicsCommandList**);

    // Cast member function pointers to the typedefs
    Present_t fnPresent = (Present_t)&IDXGISwapChain::Present;
    ExecuteCommandLists_t fnExecuteCommandLists = (ExecuteCommandLists_t)&ID3D12CommandQueue::ExecuteCommandLists;

    LogDebug("[EldenCraft] Target address for hook: 0x%p", (LPVOID)fnPresent);

    if (!fnPresent) {
        LogDebug("[EldenCraft] Target address for hook is nullptr or invalid.");
        return;
    }

    // Hook Present
    LogDebug("[EldenCraft] Creating hook for Present.");
    MH_STATUS createStatus = MH_CreateHook(Pun(fnPresent), (LPVOID)&HookedPresent, (LPVOID*)&OriginalPresent);
    LogDebug("[EldenCraft] MH_CreateHook returned: %d", createStatus);

    if (createStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to create hook for Present.");
        return;
    }

    LogDebug("[EldenCraft] Enabling hook for Present.");
    MH_STATUS enableStatus = MH_EnableHook(Pun(fnPresent));
    LogDebug("[EldenCraft] MH_EnableHook returned: %d", enableStatus);

    if (enableStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to enable hook for Present.");
        return;
    }

    // Hook OriginalExecuteCommandLists
    LogDebug("[EldenCraft] Target address for hook: 0x%p", (LPVOID)fnExecuteCommandLists);

    if (!fnExecuteCommandLists) {
        LogDebug("[EldenCraft] Target address for hook is nullptr or invalid.");
        return;
    }

    LogDebug("[EldenCraft] Creating hook for OriginalExecuteCommandLists.");
    createStatus = MH_CreateHook(Pun(fnExecuteCommandLists), (LPVOID)&OriginalExecuteCommandLists, (LPVOID*)&OriginalExecuteCommandLists);
    LogDebug("[EldenCraft] MH_CreateHook returned: %d", createStatus);

    if (createStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to create hook for OriginalExecuteCommandLists.");
        return;
    }

    LogDebug("[EldenCraft] Enabling hook for OriginalExecuteCommandLists.");
    enableStatus = MH_EnableHook(Pun(fnExecuteCommandLists));
    LogDebug("[EldenCraft] MH_EnableHook returned: %d", enableStatus);

    if (enableStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to enable hook for OriginalExecuteCommandLists.");
        return;
    }
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    LogDebug("[EldenCraft] MainThread started.");
    if (MH_Initialize() != MH_OK) {
        LogDebug("[EldenCraft] Failed to initialize MinHook.");
        return 1;
    }

    LogDebug("[EldenCraft] Creating dummy device for hooking.");
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&g_pd3dDevice))) {
        LogDebug("[EldenCraft] Failed to create dummy device.");
        return 1;
    }

    LogDebug("[EldenCraft] Extracting Present function pointer from vtable.");
    if (FAILED(g_pd3dDevice->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&g_pd3dSwapChain))) {
        LogDebug("[EldenCraft] Failed to get IDXGISwapChain3 interface.");
        g_pd3dDevice->Release();
        return 1;
    }

    LogDebug("[EldenCraft] Target address for hook: 0x%p", (LPVOID)g_pd3dSwapChain->Present);

    LogDebug("[EldenCraft] Creating hook for Present.");
    MH_STATUS createStatus = MH_CreateHook(Pun((LPVOID)g_pd3dSwapChain->Present), (LPVOID)&HookedPresent, (LPVOID*)&OriginalPresent);
    LogDebug("[EldenCraft] MH_CreateHook returned: %d", createStatus);

    if (createStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to create hook for Present.");
        g_pd3dSwapChain->Release();
        g_pd3dDevice->Release();
        return 1;
    }

    LogDebug("[EldenCraft] Enabling hook for Present.");
    MH_STATUS enableStatus = MH_EnableHook(Pun((LPVOID)g_pd3dSwapChain->Present));
    LogDebug("[EldenCraft] MH_EnableHook returned: %d", enableStatus);

    if (enableStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to enable hook for Present.");
        g_pd3dSwapChain->Release();
        g_pd3dDevice->Release();
        return 1;
    }

    // Hook OriginalExecuteCommandLists
    LogDebug("[EldenCraft] Extracting OriginalExecuteCommandLists function pointer from vtable.");
    ExecuteCommandLists_t executeCommandLists = (ExecuteCommandLists_t)((BYTE*)g_pCommandQueue + 12 * sizeof(LPVOID));

    LogDebug("[EldenCraft] Target address for hook: 0x%p", (LPVOID)executeCommandLists);

    if (!executeCommandLists) {
        LogDebug("[EldenCraft] Target address for hook is nullptr or invalid.");
        g_pd3dSwapChain->Release();
        g_pd3dDevice->Release();
        return 1;
    }

    LogDebug("[EldenCraft] Creating hook for OriginalExecuteCommandLists.");
    createStatus = MH_CreateHook(Pun((LPVOID)executeCommandLists), (LPVOID)&OriginalExecuteCommandLists, (LPVOID*)&OriginalExecuteCommandLists);
    LogDebug("[EldenCraft] MH_CreateHook returned: %d", createStatus);

    if (createStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to create hook for OriginalExecuteCommandLists.");
        g_pd3dSwapChain->Release();
        g_pd3dDevice->Release();
        return 1;
    }

    LogDebug("[EldenCraft] Enabling hook for OriginalExecuteCommandLists.");
    enableStatus = MH_EnableHook(Pun((LPVOID)executeCommandLists));
    LogDebug("[EldenCraft] MH_EnableHook returned: %d", enableStatus);

    if (enableStatus != MH_OK) {
        LogDebug("[EldenCraft] Failed to enable hook for OriginalExecuteCommandLists.");
        g_pd3dSwapChain->Release();
        g_pd3dDevice->Release();
        return 1;
    }

    // Clean up dummy objects
    g_pd3dSwapChain->Release();
    g_pd3dDevice->Release();

    // Main thread logic here
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        DeleteFileA("C:\\Users\\CW\\Downloads\\EldenCraftRuntime\\eldencraft_debug.log");
        LogDebug("[EldenCraft] DLL_PROCESS_ATTACH triggered.");
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
    }
    return TRUE;
}
