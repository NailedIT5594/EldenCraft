#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <iostream>

// Entry point called automatically when the game loads the DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Disable thread library calls for performance
        DisableThreadLibraryCalls(hModule);
        
        // Initialize your mod runtime hooks here
        // e.g., setup render loop hooks, ImGui context, or console outputs
        break;

    case DLL_PROCESS_DETACH:
        // Cleanup runtime resources
        break;
    }
    return TRUE;
}