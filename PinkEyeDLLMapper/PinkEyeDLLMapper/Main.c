#include <Windows.h>
#include <stdio.h>
#include <shlwapi.h>
#include "bytes.h"

unsigned char* readFile(const char* filename, size_t* fileSize)
{
	unsigned char* buffer = NULL;
	FILE* file = fopen(filename, "rb");
	if (file == NULL)
	{
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (unsigned char*)malloc(*fileSize);
	if (buffer == NULL)
	{
		fclose(file);
		return NULL;
	}

	size_t bytesRead = fread(buffer, 1, *fileSize, file);
	if (bytesRead != *fileSize)
	{
		fclose(file);
		free(buffer);
		return NULL;
	}

	fclose(file);
	return buffer;
}

DWORD GetExecutableFunction(LPBYTE image, LPCSTR functionName)
{
	PIMAGE_EXPORT_DIRECTORY exportDirectory;
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(image + ((PIMAGE_DOS_HEADER)image)->e_lfanew);
	exportDirectory = (PIMAGE_EXPORT_DIRECTORY)(image + RvaToOffset(image, ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));

	LPDWORD nameDirectory = (LPDWORD)(image + RvaToOffset(image, exportDirectory->AddressOfNames));
	LPWORD nameOrdinalDirectory = (LPWORD)(image + RvaToOffset(image, exportDirectory->AddressOfNameOrdinals));

	for (DWORD i = 0; i < exportDirectory->NumberOfNames; i++)
	{
		if (StrStrA((PCHAR)(image + RvaToOffset(image, *nameDirectory)), functionName))
		{
			return RvaToOffset(image, *(LPDWORD)(image + RvaToOffset(image, exportDirectory->AddressOfFunctions) + *nameOrdinalDirectory * sizeof(DWORD)));
		}

		nameDirectory++;
		nameOrdinalDirectory++;
	}

	return 0;
}

DWORD RvaToOffset(LPBYTE image, DWORD rva)
{
	PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(image + ((PIMAGE_DOS_HEADER)image)->e_lfanew);
	PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)((LPBYTE)&ntHeaders->OptionalHeader + ntHeaders->FileHeader.SizeOfOptionalHeader);

	if (rva < sections[0].PointerToRawData)
	{
		return rva;
	}
	else
	{
		for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
		{
			if (rva >= sections[i].VirtualAddress && rva < sections[i].VirtualAddress + sections[i].SizeOfRawData)
			{
				return rva - sections[i].VirtualAddress + sections[i].PointerToRawData;
			}
		}

		return 0;
	}
}

VOID MapSainan() //peg me pls :troll:
{
	//SIZE_T stubSize = 0;
	//unsigned char* stubShellcode = readFile("C:\\Users\\C5\\source\\repos\\PinkEyeDLLMapper\\x64\\Release\\PinkEyeMapperShellcode.vmp.dll", &stubSize);

	SIZE_T stubSize = sizeof(rawData);
	unsigned char* stubShellcode = rawData;

	DWORD entryPoint = GetExecutableFunction((LPBYTE)stubShellcode, "AKpXCsURzpWSGacwmXSeAHwIcDaQB");
	if (entryPoint)
	{
		LPBYTE allocatedStubMemory = (LPBYTE)VirtualAlloc(NULL, stubSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (allocatedStubMemory)
		{
			memcpy(allocatedStubMemory, stubShellcode, stubSize);
			HANDLE thread = CreateThread(NULL, NULL, allocatedStubMemory + entryPoint, allocatedStubMemory, NULL, NULL);
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			MapSainan();
			break;
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