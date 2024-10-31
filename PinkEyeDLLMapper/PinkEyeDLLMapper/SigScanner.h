#include <Windows.h>
#include <psapi.h>

int MemoryCompare(const BYTE* bData, const BYTE* bMask, const char* szMask)
{
    for (; *szMask; ++szMask, ++bData, ++bMask)
    {
        if (*szMask == 'x' && *bData != *bMask)
        {
            return 0;
        }
    }
    return (*szMask == '\0');
}

uintptr_t FindSignature(uintptr_t start, SIZE_T size, const char* sig, const char* mask)
{
    BYTE* data = (BYTE*)malloc(size);
    SIZE_T bytesRead;

    if (!data)
    {
        return 0;
    }

    if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)start, data, size, &bytesRead) || bytesRead != size)
    {
        free(data);
        return 0;
    }

    for (SIZE_T i = 0; i < size; i++)
    {
        if (MemoryCompare((const BYTE*)(data + i), (const BYTE*)sig, mask))
        {
            free(data);
            return start + i;
        }
    }

    free(data);
    return 0;
}

uintptr_t ScanPattern(const char* moduleName, const char* signature, const char* mask)
{
    HMODULE hModule = NULL;

    if (moduleName == "")
    {
        hModule = GetModuleHandleA(NULL);
    }
    else
    {
        hModule = GetModuleHandleA(moduleName);
    }

    if (!hModule)
    {
        return 0;
    }

    MODULEINFO modInfo;
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

    uintptr_t baseAddress = (uintptr_t)modInfo.lpBaseOfDll;
    SIZE_T moduleSize = (SIZE_T)modInfo.SizeOfImage;

    return FindSignature(baseAddress, moduleSize, signature, mask);
}