#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <Commdlg.h> // Für das Dateiauswahlfenster
#include <TlHelp32.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib") // Fügt die Psapi-Bibliothek hinzu

bool InjectDLL(DWORD processId, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Error: Cannot Open Process." << std::endl;
        return false;
    }

    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pDllPath == NULL) {
        std::cerr << "Error: Memory Error." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pDllPath, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cerr << "Error: Cannot write dll into Process" << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandle("Kernel32");
    if (hKernel32 == NULL) {
        std::cerr << "Error: Cannot load Kernel32." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    FARPROC pfnLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");
    if (pfnLoadLibrary == NULL) {
        std::cerr << "Error: Cannot load LoaderLibrary." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pfnLoadLibrary, pDllPath, 0, NULL);
    if (hRemoteThread == NULL) {
        std::cerr << "Error: Thread Error" << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hRemoteThread, INFINITE);

    CloseHandle(hRemoteThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

DWORD SelectProcess() {
    std::vector<DWORD> processes(1024);
    DWORD bytesReturned;

    if (!EnumProcesses(processes.data(), static_cast<DWORD>(processes.size() * sizeof(DWORD)), &bytesReturned)) {
        std::cerr << "Error: Cannot load Process list" << std::endl;
        return 0;
    }

    const DWORD numProcesses = bytesReturned / sizeof(DWORD);

    std::cout << "Processes:" << std::endl;
    std::cout << "PID\tProcess name" << std::endl;

    for (DWORD i = 0; i < numProcesses; ++i) {
        if (processes[i] != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
            if (hProcess != NULL) {
                TCHAR processName[MAX_PATH];
                if (GetModuleBaseName(hProcess, nullptr, processName, MAX_PATH) > 0) {
                    std::wcout << processes[i] << "\t" << processName << std::endl;
                }
                CloseHandle(hProcess);
            }
        }
    }

    DWORD selectedPID;
    std::cout << "\nEnter PID >> ";
    std::cin >> selectedPID;

    return selectedPID;
}

std::string SelectDLLPath() {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "DLL-Files\0*.dll\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    std::cout << "Sellect Dll" << std::endl;

    if (GetOpenFileName(&ofn) == TRUE) {
        return ofn.lpstrFile;
    } else {
        std::cerr << "Error: Cannot open selector" << std::endl;
        return "";
    }
}

int main() {
    std::string dllPath = SelectDLLPath();

    if (dllPath.empty()) {
        std::cerr << "Error: Corrupt Path or Selection canceled." << std::endl;
        return 1;
    }

    DWORD processId = SelectProcess();

    if (processId != 0) {
        if (InjectDLL(processId, dllPath.c_str())) {
            std::cout << "DLL successful deployed." << std::endl;
        } else {
            std::cerr << "Error: Deployment unsuccessful." << std::endl;
        }
    } else {
        std::cerr << "Error: Unknown PID." << std::endl;
    }

    return 0;
}
