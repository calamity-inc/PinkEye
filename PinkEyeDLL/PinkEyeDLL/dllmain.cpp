#include <Windows.h>
//#include <stdio.h>
//#include <shlobj.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Shell32.lib")

//void loadLibrariesFromDirectory(const char* directory)
//{
//    WIN32_FIND_DATAA findFileData;
//    HANDLE hFind;
//
//    char searchPath[MAX_PATH];
//    lstrcpynA(searchPath, directory, sizeof(searchPath));
//    lstrcatA(searchPath, "\\*.dll");
//
//    // Find first file
//    hFind = FindFirstFileA(searchPath, &findFileData);
//    if (hFind != INVALID_HANDLE_VALUE) {
//        do {
//            char dllFullPath[MAX_PATH];
//            lstrcpynA(dllFullPath, directory, sizeof(dllFullPath));
//            lstrcatA(dllFullPath, "\\");
//            lstrcatA(dllFullPath, findFileData.cFileName);
//
//            LoadLibraryA(dllFullPath);
//        } while (FindNextFileA(hFind, &findFileData) != 0);
//        FindClose(hFind);
//    }
//
//    lstrcpynA(searchPath, directory, sizeof(searchPath));
//    lstrcatA(searchPath, "\\*.asi");
//    hFind = FindFirstFileA(searchPath, &findFileData);
//    if (hFind != INVALID_HANDLE_VALUE) {
//        do {
//            char asiFullPath[MAX_PATH];
//            lstrcpynA(asiFullPath, directory, sizeof(asiFullPath));
//            lstrcatA(asiFullPath, "\\");
//            lstrcatA(asiFullPath, findFileData.cFileName);
//
//            LoadLibraryA(asiFullPath);
//        } while (FindNextFileA(hFind, &findFileData) != 0);
//        FindClose(hFind);
//    }
//}
//
//__declspec(noinline) VOID CodeEntryPoint()
//{
//    char appDataPath[MAX_PATH];
//
//    HRESULT result = SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
//
//    if (result != S_OK)
//    {
//        return;
//    }
//
//    char fullPath[MAX_PATH];
//    lstrcpynA(fullPath, appDataPath, sizeof(fullPath));
//    lstrcatA(fullPath, "\\Stand\\");
//
//    char dllPath[MAX_PATH];
//    lstrcpynA(dllPath, fullPath, sizeof(dllPath));
//    lstrcatA(dllPath, "Bin\\Stand 24.9.7.dll");
//
//    LoadLibraryA(dllPath);
//
//    /*char asiModsPath[MAX_PATH];
//    lstrcpynA(asiModsPath, appDataPath, sizeof(asiModsPath));
//    lstrcatA(asiModsPath, "\\Stand\\ASI Mods");
//
//    loadLibrariesFromDirectory(asiModsPath);
//
//    char soup_dllPath[MAX_PATH];
//    lstrcpynA(soup_dllPath, fullPath, sizeof(soup_dllPath));
//    lstrcatA(soup_dllPath, "Lua Scripts\\lib\\soup\\soup.dll");
//
//    LoadLibraryA(soup_dllPath);
//    
//    char luaffi_dllPath[MAX_PATH];
//    lstrcpynA(luaffi_dllPath, fullPath, sizeof(luaffi_dllPath));
//    lstrcatA(luaffi_dllPath, "Lua Scripts\\lib\\luaffi.dll");
//
//    LoadLibraryA(luaffi_dllPath);
//
//    char SCdllPath[MAX_PATH];
//    lstrcpynA(SCdllPath, fullPath, sizeof(SCdllPath));
//    lstrcatA(SCdllPath, "Bin\\ScriptHookV.dll");
//
//    LoadLibraryA(SCdllPath);*/
//
//    /*char SCdllPath[MAX_PATH];
//    lstrcpynA(SCdllPath, fullPath, sizeof(SCdllPath));
//    lstrcatA(SCdllPath, "Bin\\ScriptHookLock.dll");
//
//    LoadLibraryA(SCdllPath);*/
//}

HMODULE g_hModule = NULL;

//__declspec(noinline) VOID CodeEntryPoint()
//{
//    //char appDataPath[MAX_PATH];
//
//    //HRESULT result = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath);
//
//    //if (result != S_OK)
//    //{
//    //    return;
//    //}
//
//    //lstrcatA(appDataPath, "\\Temp\\.tmp_29381"); //temp (tmp) folder name in %appdata%\local\temp folder
//    //WIN32_FIND_DATAA findFileData;
//    //HANDLE hFind = INVALID_HANDLE_VALUE;
//    //char searchPattern[MAX_PATH];
//    //char dllPath[MAX_PATH];
//    //lstrcpyA(searchPattern, appDataPath);
//    //lstrcatA(searchPattern, "\\*.dll");
//    //hFind = FindFirstFileA(searchPattern, &findFileData);
//    //if (hFind != INVALID_HANDLE_VALUE)
//    //{
//    //    FindClose(hFind);
//    //    lstrcpyA(dllPath, appDataPath);
//    //    lstrcatA(dllPath, "\\");
//    //    lstrcatA(dllPath, findFileData.cFileName);
//    //    LoadLibraryA(dllPath);
//
//    //    HMODULE hModule = LoadLibraryA("sendmail.dll"); //trigger the driver to re-enable battleye driver callbacks
//    //    FreeLibrary(hModule);
//
//    //    FreeLibraryAndExitThread(g_hModule, 0); //free this dll and exit the thread
//    //}
//    //else
//    //{
//    //    return;
//    //}
//
//    HMODULE hModule = LoadLibraryA("sendmail.dll"); //trigger the driver to re-enable battleye driver callbacks
//    FreeLibrary(hModule);
//    FreeLibraryAndExitThread(g_hModule, 0); //free this dll and exit the thread
//}

__declspec(noinline) VOID CodeEntryPoint()
{
    Sleep(3000);

    char currentPath[MAX_PATH];

    HMODULE hModule = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&CodeEntryPoint, &hModule))
    {
        if (GetModuleFileNameA(hModule, currentPath, MAX_PATH) != 0)
        {
            char* lastBackslash = strrchr(currentPath, '\\');
            if (lastBackslash != NULL)
            {
                *lastBackslash = '\0';
            }

            WIN32_FIND_DATAA findFileData;
            HANDLE hFind = INVALID_HANDLE_VALUE;
            char searchPattern[MAX_PATH];
            char dllPath[MAX_PATH];
            lstrcpyA(searchPattern, currentPath);
            lstrcatA(searchPattern, "\\*.dll");
            hFind = FindFirstFileA(searchPattern, &findFileData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                FindClose(hFind);
                lstrcpyA(dllPath, currentPath);
                lstrcatA(dllPath, "\\Temp\\");
                lstrcatA(dllPath, findFileData.cFileName);
                LoadLibraryA(dllPath);
            }
        }
    }

    FreeLibraryAndExitThread(g_hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CodeEntryPoint, NULL, NULL, NULL);
        //CodeEntryPoint();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

EXTERN_C __declspec(dllexport) int SfcClose(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(NULL, code, wParam, lParam);
}
