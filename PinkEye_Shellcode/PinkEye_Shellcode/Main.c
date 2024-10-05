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

wchar_t windowsPath[MAX_PATH];
wchar_t driveLetter[3];

char windowsPath_Char[MAX_PATH];
char driveLetter_Char[3];

#define CUSTOM_CERTIFICATE_SIGNATURE L"D39FCF6A625ED1B13B28DCC8148672ADCBE9969C"

static NTSTATUS NTAPI HookedNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
    if (wcsstr(ObjectAttributes->ObjectName->Buffer, L":\\Windows\\System32\\CatRoot") == 0 && wcsstr(ObjectAttributes->ObjectName->Buffer, L":\\Windows\\Globalization") == 0)
    {
        SignResult signResult;
        if (VerifyCustomSignature(ObjectAttributes->ObjectName->Buffer, &signResult) == TRUE)
        {
            if (_wcsicmp(signResult.HashFinalCert, CUSTOM_CERTIFICATE_SIGNATURE) == 0)
            {
                wchar_t dllPath[MAX_PATH];
                wcscpy(dllPath, L"\\??\\");
                wcscat(dllPath, driveLetter);
                wcscat(dllPath, L"\\Windows\\System32\\kernel32.dll");

                UNICODE_STRING fileName;
                RtlInitUnicodeString(&fileName, dllPath);

                ObjectAttributes->ObjectName = &fileName;
            }
        }
    }

    return OriginalNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

__declspec(noinline) VOID CodeEntryPoint()
{
    GetWindowsDirectoryW((LPWSTR)windowsPath, MAX_PATH);
    driveLetter[0] = windowsPath[0];
    driveLetter[1] = L':';
    driveLetter[2] = L'\0';

    GetWindowsDirectoryA(windowsPath_Char, MAX_PATH);
    driveLetter_Char[0] = windowsPath_Char[0];
    driveLetter_Char[1] = ':';
    driveLetter_Char[2] = '\0';

    char ntdll_DllPath[MAX_PATH];
    strcpy(ntdll_DllPath, driveLetter_Char);
    strcat(ntdll_DllPath, "\\Windows\\System32\\ntdll.dll");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    InstallHook(ntdll_DllPath, "NtCreateFile", (LPVOID*)&OriginalNtCreateFile, HookedNtCreateFile);
    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            CodeEntryPoint();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}