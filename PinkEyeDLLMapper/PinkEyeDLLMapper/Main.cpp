#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include "detours.h"
#include "Offsets.h"

VOID UnlinkDll(const wchar_t* dllName)
{
    PROCESS_BASIC_INFORMATION processBasicInformation;
    ULONG size;
    NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &processBasicInformation, sizeof(PROCESS_BASIC_INFORMATION), &size);
    PEB* pPeb = processBasicInformation.PebBaseAddress;

    for (auto entry = pPeb->Ldr->InMemoryOrderModuleList.Flink; entry != &pPeb->Ldr->InMemoryOrderModuleList; entry = entry->Flink)
    {
        _LDR_DATA_TABLE_ENTRY* pCurEntry = (_LDR_DATA_TABLE_ENTRY*)entry;

        if (!wcscmp(pCurEntry->FullDllName.Buffer, dllName))
        {
            entry->Flink->Blink = entry->Blink;
            entry->Blink->Flink = entry->Flink;
        }
    }
}

LPCWSTR ConvertToLPCWSTR(const char* narrowString) {
    int wideStrSize = MultiByteToWideChar(CP_UTF8, 0, narrowString, -1, NULL, 0);
    wchar_t* wideString = (wchar_t*)malloc(wideStrSize * sizeof(wchar_t));
    if (wideString == NULL) {
        return NULL;
    }
    MultiByteToWideChar(CP_UTF8, 0, narrowString, -1, wideString, wideStrSize);
    return wideString;
}

uintptr_t GetModuleBaseAddress(uintptr_t address)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPVOID)address, &mbi, sizeof(mbi)) == 0)
    {
        return NULL;
    }
    return (uintptr_t)mbi.AllocationBase;
}

VOID UnlinkFromPEB()
{
    char szModName[MAX_PATH];
    HMODULE hModule = (HMODULE)GetModuleBaseAddress((uintptr_t)UnlinkFromPEB);
    GetModuleBaseNameA(GetCurrentProcess(), hModule, szModName, sizeof(szModName) / sizeof(char));
    UnlinkDll(ConvertToLPCWSTR(szModName));
}

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

CRITICAL_SECTION BailProcess_CS;

VOID BailProcess(int FlagID)
{
    EnterCriticalSection(&BailProcess_CS);
    if (FlagID == 0)
    {
        MessageBoxA(NULL, "One or more offsets were not found, and the game needs to bail.", "", 0);
        TerminateProcess(GetCurrentProcess(), 0);
    }
    else
    {
        char message[256];
        snprintf(message, sizeof(message), "A BattlEye flag with the ID (%d) has been tripped, and the game needs to bail.", FlagID);
        MessageBoxA(NULL, message, "", 0);
        TerminateProcess(GetCurrentProcess(), 0);
    }
    LeaveCriticalSection(&BailProcess_CS);
}

typedef int(__stdcall* typedef_BEReport)(DWORD a1, DWORD a2, char a3, DWORD a4);
static typedef_BEReport Original_BEReport;
static int __stdcall Hooked_BEReport(DWORD a1, DWORD a2, char a3, DWORD a4)
{
    if (a4 != 0)
    {
        BailProcess(1);
        return 0;
    }
    else
    {
        return Original_BEReport(a1, a2, a3, a4);
    }
}

typedef BOOL(__stdcall* typedef_BECRC)(char a1, int a2);
static typedef_BEReport Original_BECRC;
static BOOL __stdcall Hooked_BECRC(char a1, int a2)
{
    return TRUE;
}

int main()
{
    UnlinkFromPEB();

    InitializeCriticalSection(&BailProcess_CS);

    LPVOID BEReport_Address = (LPVOID)Get_BEReport_Offset();
    LPVOID BECRC_Address = (LPVOID)Get_BECRC_Offset();

    if (BEReport_Address == NULL ||
        BECRC_Address == NULL)
    {
        BailProcess(0);
    }
    else
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        pInstallHook(BEReport_Address, (LPVOID*)&Original_BEReport, Hooked_BEReport);
        pInstallHook(BECRC_Address, (LPVOID*)&Original_BECRC, Hooked_BECRC);
        DetourTransactionCommit();
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        main();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

EXTERN_C __declspec(dllexport) int DeezTwo(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(NULL, code, wParam, lParam);
}