#include <Windows.h>
#include <winternl.h>
#include "detours.h"
#include "Verify.h"

LPVOID GetFunction(LPCSTR dll, LPCSTR function)
{
    HMODULE module = GetModuleHandleA(dll);
    return module ? (LPVOID)GetProcAddress(module, function) : NULL;
}

static VOID InstallHook(LPCSTR dll, LPCSTR function, LPVOID* originalFunction, LPVOID hookedFunction)
{
    *originalFunction = GetFunction(dll, function);
    if (*originalFunction) DetourAttach(originalFunction, hookedFunction);
}

static VOID pInstallHook(LPVOID functionAddress, LPVOID* originalFunction, LPVOID hookedFunction)
{
    *originalFunction = functionAddress;
    if (*originalFunction) DetourAttach(originalFunction, hookedFunction);
}

char windowsPath_Char[MAX_PATH];
char driveLetter_Char[3];

#define CUSTOM_CERTIFICATE_SIGNATURE L"D39FCF6A625ED1B13B28DCC8148672ADCBE9969C"

#define ASLR(x) (x - (uintptr_t)0x140000000 + (uintptr_t)GetModuleHandleA(NULL))

typedef VOID(__stdcall* typedef_BERetFlag1)(LPVOID a1);
static typedef_BERetFlag1 BERetFlag1; //we could also spoof the return address probably but that comes with its own issues, if we cant find the addresses later on or a new check is added/enabled, we will resort to that instead

uintptr_t RWCheck = NULL;

static LONG WINAPI HookedWinVerifyTrust(HWND hwnd, GUID* pgActionID, LPVOID pWVTData)
{
    WINTRUST_DATA wd = *(WINTRUST_DATA*)pWVTData;
    SignResult signResult;
    GetSignerInfo(wd.hWVTStateData, &signResult);
    if (_wcsicmp(signResult.HashFinalCert, CUSTOM_CERTIFICATE_SIGNATURE) == 0)
    {
        *(BOOL*)RWCheck = FALSE;
        BERetFlag1(_ReturnAddress()); //add this call to allowed chain of calls
        *(BOOL*)RWCheck = TRUE; //we was never here :kek:
        return ERROR_SUCCESS;
    }
    else
    {
        *(BOOL*)RWCheck = FALSE;
        BERetFlag1(_ReturnAddress()); //add this call to allowed chain of calls
        *(BOOL*)RWCheck = TRUE; //we was never here :kek:
        return OriginalWinVerifyTrust(hwnd, pgActionID, pWVTData);
    }
}

__declspec(noinline) VOID CodeEntryPoint()
{
    BERetFlag1 = ASLR(0x1416FDD8A);
    RWCheck = (uintptr_t)0x1416FBE44;

    GetWindowsDirectoryA(windowsPath_Char, MAX_PATH);
    driveLetter_Char[0] = windowsPath_Char[0];
    driveLetter_Char[1] = ':';
    driveLetter_Char[2] = '\0';

    char ntdll_DllPath[MAX_PATH];
    strcpy(ntdll_DllPath, driveLetter_Char);
    strcat(ntdll_DllPath, "\\Windows\\System32\\wintrust.dll");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    InstallHook(ntdll_DllPath, "WinVerifyTrust", (LPVOID*)&OriginalWinVerifyTrust, HookedWinVerifyTrust);
    DetourTransactionCommit();
}

VOID BlockThread()
{
    while (TRUE)
    {
        Sleep(INFINITE);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            CodeEntryPoint();
            BlockThread(); //Prevent crash after reflective injection
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}