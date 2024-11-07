#include <Windows.h>
#include <stdio.h>
#include <tlhelp32.h>

//hi sainan :D

unsigned char* readFile(const char* filename, size_t* fileSize) {
	unsigned char* buffer = NULL;
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		return NULL;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate memory for file content
	buffer = (unsigned char*)malloc(*fileSize);
	if (buffer == NULL) {
		fprintf(stderr, "Memory allocation failed\n");
		fclose(file);
		return NULL;
	}

	// Read file content into buffer
	size_t bytesRead = fread(buffer, 1, *fileSize, file);
	if (bytesRead != *fileSize) {
		fprintf(stderr, "Failed to read file: %s\n", filename);
		fclose(file);
		free(buffer);
		return NULL;
	}

	fclose(file);
	return buffer;
}

DWORD GetProcessIDByName(LPCWSTR processName)
{
	HANDLE hSnapshot;
	PROCESSENTRY32 processEntry;
	DWORD processID = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	processEntry.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnapshot, &processEntry)) {
		do {
			if (wcscmp(processEntry.szExeFile, processName) == 0) {
				processID = processEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &processEntry));
	}

	CloseHandle(hSnapshot);
	return processID;
}

#pragma region Manual Mapper
LPVOID GetFunction(LPCSTR dll, LPCSTR function)
{
	HMODULE module = GetModuleHandleA(dll);
	return module ? (LPVOID)GetProcAddress(module, function) : NULL;
}

typedef NTSTATUS(NTAPI* NT_NTCREATETHREADEX)(PHANDLE thread, ACCESS_MASK desiredAccess, LPVOID objectAttributes, HANDLE processHandle, LPVOID startAddress, LPVOID parameter, ULONG flags, SIZE_T stackZeroBits, SIZE_T sizeOfStackCommit, SIZE_T sizeOfStackReserve, LPVOID bytesBuffer);

NTSTATUS NTAPI SainanMapper_NtCreateThreadEx(PHANDLE thread, ACCESS_MASK desiredAccess, LPVOID objectAttributes, HANDLE processHandle, LPVOID startAddress, LPVOID parameter, ULONG flags, SIZE_T stackZeroBits, SIZE_T sizeOfStackCommit, SIZE_T sizeOfStackReserve, LPVOID bytesBuffer)
{
	return ((NT_NTCREATETHREADEX)GetFunction("ntdll.dll", "NtCreateThreadEx"))(thread, desiredAccess, objectAttributes, processHandle, startAddress, parameter, flags, stackZeroBits, sizeOfStackCommit, sizeOfStackReserve, bytesBuffer);
}

#include "Shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")
#include <winternl.h>

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

VOID MapSai(DWORD ProcessID)
{
	SIZE_T dllSize = 0;
	unsigned char* dll = readFile("C:\\Users\\C5\\Downloads\\Stand src\\Stand\\Stand\\Stand\\bin\\Debug\\Stand.dll", &dllSize);

	BOOL result = FALSE;
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessID);
	if (process)
	{
		DWORD entryPoint = GetExecutableFunction(dll, "MapSainan");
		if (entryPoint)
		{
			LPBYTE allocatedMemory = (LPBYTE)VirtualAllocEx(process, NULL, dllSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if (allocatedMemory)
			{
				if (WriteProcessMemory(process, allocatedMemory, dll, dllSize, NULL))
				{
					HANDLE thread = NULL;
					if (NT_SUCCESS(SainanMapper_NtCreateThreadEx(&thread, 0x1fffff, NULL, process, allocatedMemory + entryPoint, allocatedMemory, 0, 0, 0, 0, NULL)) && thread)
					{
						if (WaitForSingleObject(thread, 100) == WAIT_OBJECT_0)
						{
							DWORD exitCode;
							if (GetExitCodeThread(thread, &exitCode))
							{
								result = exitCode != 0;
							}

							if (!VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE))
							{
								result = FALSE;
							}
						}

						CloseHandle(thread);
					}
				}
			}
		}
		CloseHandle(process);
	}
}
#pragma endregion

#pragma region Shellcode Injector
VOID MapShellcode(DWORD ProcessID)
{
	SIZE_T shellCodeSize = 0;
	unsigned char* shellCode = readFile("C:\\Users\\C5\\Downloads\\Stand src\\Stand\\Stand\\Stand\\bin\\Debug\\shellcode.bin", &shellCodeSize);

	HANDLE hw = OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessID);
	void* base = VirtualAllocEx(hw, NULL, shellCodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	SIZE_T bytesWritten = 0;
	WriteProcessMemory(hw, base, shellCode, shellCodeSize, &bytesWritten);
	CreateRemoteThread(hw, NULL, NULL, (LPTHREAD_START_ROUTINE)base, NULL, 0, 0);
	CloseHandle(hw);
}
#pragma endregion

int main()
{
    DWORD ProcessID = GetProcessIDByName(L"GTA5.exe");
    
	MapSai(ProcessID);

	//MapShellcode(ProcessID);
}