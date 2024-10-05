#include <Windows.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "user32.lib")

EXTERN_C __declspec(dllexport) DWORD InjectDll(const char* windowName, const char* dllPath, const char* functionName)
{
    HWND hwnd = FindWindowA(NULL, windowName);

    if (hwnd == NULL)
    {
        return 10;
    }

    DWORD pid = NULL;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (tid == 0 || pid == 0)
    {
        return 11;
    }

    HMODULE dll = LoadLibraryExA(dllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (dll == NULL)
    {
        return 12;
    }

    FARPROC addr = GetProcAddress(dll, functionName);
    if (addr == NULL)
    {
        return 13;
    }

    HHOOK hookHandle = SetWindowsHookExW(WH_GETMESSAGE, addr, dll, tid);
    if (hookHandle == NULL)
    {
        return 14;
    }

    PostThreadMessageW(tid, WM_NULL, NULL, NULL);

    Sleep(3000);
    UnhookWindowsHookEx(hookHandle);

    return 1;
}

EXTERN_C __declspec(dllexport) DWORD InjectDll_NoUnload(const char* windowName, const char* dllPath, const char* functionName)
{
    HWND hwnd = FindWindowA(NULL, windowName);

    if (hwnd == NULL)
    {
        return 10;
    }

    DWORD pid = NULL;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (tid == 0 || pid == 0)
    {
        return 11;
    }

    HMODULE dll = LoadLibraryExA(dllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (dll == NULL)
    {
        return 12;
    }

    FARPROC addr = GetProcAddress(dll, functionName);
    if (addr == NULL)
    {
        return 13;
    }

    HHOOK hookHandle = SetWindowsHookExW(WH_GETMESSAGE, addr, dll, tid);
    if (hookHandle == NULL)
    {
        return 14;
    }

    PostThreadMessageW(tid, WM_NULL, NULL, NULL);

    Sleep(3000);
    
    return 1;
}

EXTERN_C __declspec(dllexport) DWORD InjectDllAtEntrypoint(const char* windowName, const char* dllPath)
{
    HWND hwnd = FindWindowA(NULL, windowName);

    if (hwnd == NULL)
    {
        return 10;
    }

    DWORD pid = NULL;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (tid == 0 || pid == 0)
    {
        return 11;
    }

    HMODULE dll = LoadLibraryExA(dllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (dll == NULL)
    {
        return 12;
    }

    BYTE* baseAddress = (BYTE*)dll;
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)baseAddress;
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(baseAddress + dosHeader->e_lfanew);
    DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;
    void* entryPoint = (void*)(baseAddress + entryPointRVA);

    //FARPROC addr = (FARPROC)entryPoint;
    HOOKPROC addr = (HOOKPROC)entryPoint;
    if (addr == NULL)
    {
        return 13;
    }

    HHOOK hookHandle = SetWindowsHookExW(WH_GETMESSAGE, addr, dll, tid);
    if (hookHandle == NULL)
    {
        return 14;
    }

    PostThreadMessageW(tid, WM_NULL, NULL, NULL);

    Sleep(3000);
    UnhookWindowsHookEx(hookHandle);

    return 1;
}

#pragma region Driver Loader/Unloader
BOOL InternalLoadDriver(LPCSTR DRIVER_NAME, LPCSTR DRIVER_PATH)
{
    SC_HANDLE scmHandle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scmHandle)
    {
        return FALSE;
    }

    SC_HANDLE serviceHandle = CreateServiceA(
        scmHandle,
        DRIVER_NAME,
        DRIVER_NAME,
        SERVICE_START | SERVICE_STOP | DELETE,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DRIVER_PATH,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (!serviceHandle)
    {
        if (GetLastError() != ERROR_SERVICE_EXISTS)
        {
            CloseServiceHandle(scmHandle);
            return FALSE;
        }
        else
        {
            serviceHandle = OpenServiceA(scmHandle, DRIVER_NAME, SERVICE_START);
            if (!serviceHandle)
            {
                CloseServiceHandle(scmHandle);
                return FALSE;
            }
        }
    }

    if (!StartServiceA(serviceHandle, 0, NULL))
    {
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return FALSE;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);

    return TRUE;
}

BOOL InternalUnloadDriver(LPCSTR DRIVER_NAME)
{
    SC_HANDLE scmHandle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scmHandle)
    {
        return FALSE;
    }

    SC_HANDLE serviceHandle = OpenServiceA(scmHandle, DRIVER_NAME, SERVICE_STOP | DELETE);
    if (!serviceHandle)
    {
        CloseServiceHandle(scmHandle);
        return FALSE;
    }

    SERVICE_STATUS serviceStatus;
    if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
    {
        DeleteService(serviceHandle); //still attempt to delete driver //DEBUG

        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return FALSE;
    }

    if (!DeleteService(serviceHandle))
    {
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return FALSE;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);

    return TRUE;
}
#pragma endregion

EXTERN_C __declspec(dllexport) DWORD LoadDriver(const char* windowName, const char* DriverName, const char* DriverPath)
{
    HWND hwnd = FindWindowA(NULL, windowName);

    if (hwnd != NULL)
    {
        return 10;
    }

    if (InternalLoadDriver((LPCSTR)DriverName, (LPCSTR)DriverPath) == TRUE)
    {
        return 1;
    }
    else
    {
        return 11;
    }
}

EXTERN_C __declspec(dllexport) DWORD UnloadDriver(const char* DriverName)
{
    if (InternalUnloadDriver((LPCSTR)DriverName) == TRUE)
    {
        return 1;
    }
    else
    {
        return 10;
    }
}

EXTERN_C __declspec(dllexport) DWORD DoesWindowExist(const char* windowName)
{
    HWND hwnd = FindWindowA(NULL, windowName);

    if (hwnd == NULL)
    {
        return 10;
    }
    else
    {
        return 1;
    }
}

VOID InternalInjectDll(const char* windowName, const char* mainDllPath)
{
    HWND hwnd = FindWindowA(NULL, windowName);

    if (hwnd == NULL)
    {
        return;
    }

    DWORD pid = NULL;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (tid == 0 || pid == 0)
    {
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    LPVOID dllPathAddressInRemoteMemory = VirtualAllocEx(hProcess, NULL, strlen(mainDllPath), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    BOOL succeededWriting = WriteProcessMemory(hProcess, dllPathAddressInRemoteMemory, mainDllPath, strlen(mainDllPath), NULL);
    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE remoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryAddress, dllPathAddressInRemoteMemory, NULL, NULL);
    CloseHandle(hProcess);
}

//EXTERN_C __declspec(dllexport) DWORD InjectDll_PinkEyeless(const char* windowName, const char* driverDllPath1, const char* functionName, const char* driverDllPath2, const char* mainDllPath)
EXTERN_C __declspec(dllexport) DWORD InjectDll_PinkEyeless(const char* windowName, const char* driverDllPath1, const char* functionName, const char* mainDllPath)
{
    InjectDll(windowName, driverDllPath1, functionName);
    Sleep(3000); //Increase wait time before injecting Stand after unloading "sfc_os.dll" (or whatever the communication dll is) if needed (i.e. game crashing, etc)
    InternalInjectDll(windowName, mainDllPath);
    //InjectDllAtEntrypoint(windowName, driverDllPath2);
    return 1;
}
