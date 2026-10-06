#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

DWORD GetProcessIdByName(const std::string& processName) {
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            std::wstring wExeFile(pe32.szExeFile);
            std::string exeFile(wExeFile.begin(), wExeFile.end());
            if (processName == exeFile) {
                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processId;
}

bool InjectDLL(DWORD processId, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    LPVOID pRemoteString = VirtualAllocEx(hProcess, NULL, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteString) {
        std::cerr << "Failed to allocate memory in target process." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pRemoteString, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        std::cerr << "Failed to write DLL path to process memory." << std::endl;
        VirtualFreeEx(hProcess, pRemoteString, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteString, 0, NULL);
    if (!hThread) {
        std::cerr << "Failed to create remote thread." << std::endl;
        VirtualFreeEx(hProcess, pRemoteString, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteString, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return true;
}

int main() {
    std::string processName = "eldenring.exe";
    std::string dllPath = "EldenCraft.dll";

    std::cout << "Searching for " << processName << "..." << std::endl;
    DWORD pid = GetProcessIdByName(processName);

    if (pid == 0) {
        std::cerr << "Process not found! Make sure Elden Ring is running." << std::endl;
        return 1;
    }

    std::cout << "Found " << processName << " (PID: " << pid << "). Injecting " << dllPath << "..." << std::endl;
    if (InjectDLL(pid, dllPath)) {
        std::cout << "Successfully injected " << dllPath << " into " << processName << "!" << std::endl;
    } else {
        std::cerr << "Injection failed." << std::endl;
        return 1;
    }

    return 0;
}
