#include <Windows.h>
#include <psapi.h>
#include "detours.h"
#include <lib/MinHook 423d1e4/src/buffer.h>

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _NT_LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	LPVOID DllBase;
	LPVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} NT_LDR_DATA_TABLE_ENTRY, * PNT_LDR_DATA_TABLE_ENTRY;

typedef struct _NT_PEB_LDR_DATA
{
	DWORD Length;
	DWORD Initialized;
	LPVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	LPVOID EntryInProgress;
} NT_PEB_LDR_DATA, * PNT_PEB_LDR_DATA;

typedef struct _NT_IMAGE_RELOC
{
	WORD Offset : 12;
	WORD Type : 4;
} NT_IMAGE_RELOC, * PNT_IMAGE_RELOC;

typedef struct _NT_PEB
{
	BYTE InheritedAddressSpace;
	BYTE ReadImageFileExecOptions;
	BYTE BeingDebugged;
	BYTE SpareBool;
	LPVOID Mutant;
	LPVOID ImageBaseAddress;
	PNT_PEB_LDR_DATA Ldr;
	LPVOID ProcessParameters;
	LPVOID SubSystemData;
	LPVOID ProcessHeap;
	PRTL_CRITICAL_SECTION FastPebLock;
	LPVOID FastPebLockRoutine;
	LPVOID FastPebUnlockRoutine;
	DWORD EnvironmentUpdateCount;
	LPVOID KernelCallbackTable;
	DWORD SystemReserved;
	DWORD AtlThunkSListPtr32;
	LPVOID FreeList;
	DWORD TlsExpansionCounter;
	LPVOID TlsBitmap;
	DWORD TlsBitmapBits[2];
	LPVOID ReadOnlySharedMemoryBase;
	LPVOID ReadOnlySharedMemoryHeap;
	LPVOID ReadOnlyStaticServerData;
	LPVOID AnsiCodePageData;
	LPVOID OemCodePageData;
	LPVOID UnicodeCaseTableData;
	DWORD NumberOfProcessors;
	DWORD NtGlobalFlag;
	LARGE_INTEGER CriticalSectionTimeout;
	DWORD HeapSegmentReserve;
	DWORD HeapSegmentCommit;
	DWORD HeapDeCommitTotalFreeThreshold;
	DWORD HeapDeCommitFreeBlockThreshold;
	DWORD NumberOfHeaps;
	DWORD MaximumNumberOfHeaps;
	LPVOID ProcessHeaps;
	LPVOID GdiSharedHandleTable;
	LPVOID ProcessStarterHelper;
	DWORD GdiDCAttributeList;
	LPVOID LoaderLock;
	DWORD OSMajorVersion;
	DWORD OSMinorVersion;
	WORD OSBuildNumber;
	WORD OSCSDVersion;
	DWORD OSPlatformId;
	DWORD ImageSubsystem;
	DWORD ImageSubsystemMajorVersion;
	DWORD ImageSubsystemMinorVersion;
	DWORD ImageProcessAffinityMask;
	DWORD GdiHandleBuffer[34];
	LPVOID PostProcessInitRoutine;
	LPVOID TlsExpansionBitmap;
	DWORD TlsExpansionBitmapBits[32];
	DWORD SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	LPVOID ShimData;
	LPVOID AppCompatInfo;
	UNICODE_STRING CSDVersion;
	LPVOID ActivationContextData;
	LPVOID ProcessAssemblyStorageMap;
	LPVOID SystemDefaultActivationContextData;
	LPVOID SystemAssemblyStorageMap;
	DWORD MinimumStackCommit;
} NT_PEB, * PNT_PEB;

typedef DWORD(NTAPI* NT_NTFLUSHINSTRUCTIONCACHE)(HANDLE process, LPVOID baseAddress, ULONG size);
typedef HMODULE(WINAPI* NT_LOADLIBRARYA)(LPCSTR fileName);
typedef FARPROC(WINAPI* NT_GETPROCADDRESS)(HMODULE module, LPCSTR function);
typedef LPVOID(WINAPI* NT_VIRTUALALLOC)(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect);
typedef BOOL(WINAPI* NT_VIRTUALPROTECT)(LPVOID address, SIZE_T size, DWORD newProtect, PDWORD oldProtect);
typedef BOOL(WINAPI* NT_DLLMAIN)(HINSTANCE module, DWORD reason, LPVOID reserved);
typedef VOID(WINAPI* ENABLE_EXCEPTIONS)(void* dll);

DWORD SectionCharacteristicsToProtection(DWORD characteristics)
{
	if ((characteristics & IMAGE_SCN_MEM_EXECUTE) && (characteristics & IMAGE_SCN_MEM_READ) && (characteristics & IMAGE_SCN_MEM_WRITE))
	{
		return PAGE_EXECUTE_READWRITE;
	}
	else if ((characteristics & IMAGE_SCN_MEM_EXECUTE) && (characteristics & IMAGE_SCN_MEM_READ))
	{
		return PAGE_EXECUTE_READ;
	}
	else if ((characteristics & IMAGE_SCN_MEM_EXECUTE) && (characteristics & IMAGE_SCN_MEM_WRITE))
	{
		return PAGE_EXECUTE_WRITECOPY;
	}
	else if ((characteristics & IMAGE_SCN_MEM_READ) && (characteristics & IMAGE_SCN_MEM_WRITE))
	{
		return PAGE_READWRITE;
	}
	else if (characteristics & IMAGE_SCN_MEM_EXECUTE)
	{
		return PAGE_EXECUTE;
	}
	else if (characteristics & IMAGE_SCN_MEM_READ)
	{
		return PAGE_READONLY;
	}
	else if (characteristics & IMAGE_SCN_MEM_WRITE)
	{
		return PAGE_WRITECOPY;
	}
	else
	{
		return PAGE_NOACCESS;
	}
}

#define i_memcpy(dest, src, count) __movsb((LPBYTE)(dest), (unsigned char*)(src), (SIZE_T)(count))

#define RVA(type, base_addr, rva) (type)((ULONG_PTR) base_addr + rva)

static LPVOID PebGetProcAddress(DWORD moduleHash, DWORD functionHash)
{
#ifdef _WIN64
	PNT_PEB_LDR_DATA peb = (PNT_PEB_LDR_DATA)((PNT_PEB)__readgsqword(0x60))->Ldr;
#else
	PNT_PEB_LDR_DATA peb = (PNT_PEB_LDR_DATA)((PNT_PEB)__readfsdword(0x30))->Ldr;
#endif

	PNT_LDR_DATA_TABLE_ENTRY firstPebEntry = (PNT_LDR_DATA_TABLE_ENTRY)peb->InMemoryOrderModuleList.Flink;
	PNT_LDR_DATA_TABLE_ENTRY pebEntry = firstPebEntry;
	do
	{
		DWORD entryHash = 0;
		if (pebEntry->BaseDllName.Buffer)
		{
			for (USHORT i = 0; i < pebEntry->BaseDllName.Length; i++)
			{
				CHAR c = ((LPCSTR)pebEntry->BaseDllName.Buffer)[i];
				entryHash = _rotr(entryHash, 13) + (c >= 'a' ? c - 0x20 : c);
			}
		}

		if (entryHash == moduleHash)
		{
			LPBYTE dllBase = (LPBYTE)pebEntry->DllBase;
			PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(dllBase + ((PIMAGE_DOS_HEADER)dllBase)->e_lfanew);
			PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)(dllBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
			LPDWORD nameDirectory = (LPDWORD)(dllBase + exportDirectory->AddressOfNames);
			LPWORD nameOrdinalDirectory = (LPWORD)(dllBase + exportDirectory->AddressOfNameOrdinals);

			for (DWORD i = 0; i < exportDirectory->NumberOfNames; i++, nameDirectory++, nameOrdinalDirectory++)
			{
				DWORD hash = 0;
				for (LPCSTR currentFunctionName = (LPCSTR)(dllBase + *nameDirectory); *currentFunctionName; currentFunctionName++)
				{
					hash = _rotr(hash, 13) + *currentFunctionName;
				}

				if (hash == functionHash)
				{
					return dllBase + *(LPDWORD)(dllBase + exportDirectory->AddressOfFunctions + *nameOrdinalDirectory * sizeof(DWORD));
				}
			}

			return NULL;
		}
	} while ((pebEntry = (PNT_LDR_DATA_TABLE_ENTRY)pebEntry->InMemoryOrderModuleList.Flink) != firstPebEntry);

	return NULL;
}

BOOL EnableExceptions(DWORD64 moduleBase)
{
	PIMAGE_DOS_HEADER pDOSHeader;
	PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_OPTIONAL_HEADER pOptHeader;

	pDOSHeader = (PIMAGE_DOS_HEADER)moduleBase;
	if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return FALSE;
	}

	pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDOSHeader + pDOSHeader->e_lfanew);
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		return FALSE;
	}

	pOptHeader = (PIMAGE_OPTIONAL_HEADER)&pNTHeader->OptionalHeader;
	if (pOptHeader->Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
	{
		return FALSE;
	}

	PRUNTIME_FUNCTION pFunctionTable = (PRUNTIME_FUNCTION)((DWORD64)pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress + moduleBase);

	DWORD sizeFunctionTable = (pOptHeader->DataDirectory[3].Size / (DWORD)sizeof(RUNTIME_FUNCTION));

	BOOL success = RtlAddFunctionTable(pFunctionTable, sizeFunctionTable, moduleBase); //winnt.h

	return success;
}

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

typedef struct __LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID      DllBase;
	PVOID      EntryPoint;
	ULONG      SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG      Flags;
	SHORT      LoadCount;
	SHORT      TlsIndex;
	LIST_ENTRY HashLinks;
	PVOID      SectionPointer;
	ULONG      CheckSum;
	ULONG      TimeDateStamp;
	PVOID      LoadedImports;
	PVOID      EntryPointActivationContext;
	PVOID      PatchInformation;
} ___LDR_DATA_TABLE_ENTRY_BASE, * _PLDR_DATA_TABLE_ENTRY_BASE;

typedef LONG(NTAPI* typedef_LdrpHandleTlsData)(___LDR_DATA_TABLE_ENTRY_BASE* ModuleEntry);

void RegisterCustomLdrEntry(HMODULE hModule)
{
	___LDR_DATA_TABLE_ENTRY_BASE LdrpEntryBase;
	memset(&LdrpEntryBase, 0, sizeof(LdrpEntryBase));

	LdrpEntryBase.DllBase = (PVOID)(hModule);

	uintptr_t functionAddress = ScanPattern("ntdll.dll", "\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x33\xC4\x48\x89\x84\x24\x00\x00\x00\x00\x48\x8B\xC1\x48\x89\x4C\x24\x00\x48\x89\x8C\x24\x00\x00\x00\x00\x33\xDB\x39\x1D\x00\x00\x00\x00\x74\x33\x44\x8D\x43\x09\x48\x8D\x4C\x24\x00\x48\x89\x4C\x24\x00\x4C\x8D\x4C\x24\x00\xB2\x01\x48\x8B\x48\x30\xE8\x00\x00\x00\x00\x48\x8B\x4C\x24\x00\x85\xC0\x48\x0F\x48\xCB\x48\x89\x4C\x24\x00\x48\x85\xC9\x75\x31\x33\xC0\x48\x8B\x8C\x24\x00\x00\x00\x00\x48\x33\xCC\xE8\x00\x00\x00\x00\x4C\x8D\x9C\x24\x00\x00\x00\x00\x49\x8B\x5B\x28\x49\x8B\x73\x30\x49\x8B\x7B\x38\x49\x8B\xE3\x41\x5F\x41\x5E\x41\x5C\xC3", "xxxx?xxxx?xxxx?xxxxxxxxx????xxx????xxxxxxx????xxxxxxx?xxxx????xxxx????xxxxxxxxxx?xxxx?xxxx?xxxxxxx????xxxx?xxxxxxxxxx?xxxxxxxxxxx????xxxx????xxxx????xxxxxxxxxxxxxxxxxxxxxx");

	if (functionAddress == NULL)
	{
		functionAddress = ScanPattern("ntdll.dll", "\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x33\xC4\x48\x89\x84\x24\x00\x00\x00\x00\x4C\x8B\xE9\x48\x89\x8C\x24\x00\x00\x00\x00\x48\x89\x8C\x24\x00\x00\x00\x00\x33\xDB\x39\x1D\x00\x00\x00\x00\x74\x37\x44\x8D\x43\x09\x44\x39\x81\x00\x00\x00\x00\x74\x2A\x48\x8D\x44\x24\x00\x48\x89\x44\x24\x00\x4C\x8D\x4C\x24\x00\xB2\x01\x48\x8B\x49\x30\xE8\x00\x00\x00\x00\x4C\x8B\x7C\x24\x00\x85\xC0\x4C\x0F\x48\xFB\x4D\x85\xFF\x75\x31\x33\xC0\x48\x8B\x8C\x24\x00\x00\x00\x00\x48\x33\xCC\xE8\x00\x00\x00\x00\x4C\x8D\x9C\x24\x00\x00\x00\x00\x49\x8B\x5B\x28\x49\x8B\x73\x30\x49\x8B\x7B\x38\x49\x8B\xE3\x41\x5F\x41\x5E\x41\x5D\xC3", "xxxx?xxxx?xxxx?xxxxxxxxx????xxx????xxxxxxx????xxxxxxx????xxxx????xxxx????xxxxxxxxx????xxxxxx?xxxx?xxxx?xxxxxxx????xxxx?xxxxxxxxxxxxxxxxx????xxxx????xxxx????xxxxxxxxxxxxxxxxxxxxxx");
	}

	if (functionAddress == NULL)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}

	((typedef_LdrpHandleTlsData)functionAddress)(&LdrpEntryBase);
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

LPVOID ntdllBaseAddress = NULL;
LPVOID allocatedMemoryAddress = NULL;
SIZE_T allocatedMemorySize = 0;

BOOLEAN CheckReturnAddressBounds(ULONG_PTR Rip, ULONG_PTR BaseAddress, DWORD ModuleSize)
{
	return (Rip > BaseAddress) && (Rip < (BaseAddress + ModuleSize));
}

typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation
} MEMORY_INFORMATION_CLASS;
typedef LONG(NTAPI* typedef_NtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);
static typedef_NtQueryVirtualMemory OriginalNtQueryVirtualMemory;
static LONG NTAPI HookedNtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength)
{
	if (allocatedMemoryAddress != NULL && allocatedMemorySize != 0 && CheckReturnAddressBounds((uintptr_t)BaseAddress, (uintptr_t)allocatedMemoryAddress, allocatedMemorySize) == TRUE)
	{
		BaseAddress = ntdllBaseAddress;
	}
	return OriginalNtQueryVirtualMemory(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
}

#define NT_SUCCESS(Status) (((LONG)(Status)) >= 0)
typedef LONG(NTAPI* typedef_NtResumeThread)(HANDLE, PULONG);
static typedef_NtResumeThread OriginalNtResumeThread;
static LONG NTAPI HookedNtResumeThread(HANDLE ThreadHandle, PULONG SuspendCount)
{
	ULONG localSuspendCount;
	LONG status = OriginalNtResumeThread(ThreadHandle, &localSuspendCount);
	if (NT_SUCCESS(status))
	{
		if (SuspendCount != NULL)
		{
			if (localSuspendCount != (ULONG)-1)
			{
				*SuspendCount = (ULONG)-1;
			}
			else
			{
				*SuspendCount = localSuspendCount;
			}
		}
	}
	else
	{
		if (SuspendCount != NULL)
		{
			*SuspendCount = localSuspendCount;
		}
	}
	return status;
}

SIZE_T VirtualQueryWrapper(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength)
{
	SIZE_T returnLength;
	LONG status = OriginalNtQueryVirtualMemory(GetCurrentProcess(), (PVOID)lpAddress, MemoryBasicInformation, lpBuffer, dwLength, &returnLength);
	if (status != 0)
	{
		return 0;
	}
	return returnLength;
}

#define PAGE_EXECUTE_FLAGS (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
typedef BOOL(__stdcall* typedef_IsExecutableAddress)(LPVOID pAddress);
static typedef_IsExecutableAddress OriginalIsExecutableAddress;
static BOOL __stdcall HookedIsExecutableAddress(LPVOID pAddress)
{
	MEMORY_BASIC_INFORMATION mi;
	VirtualQueryWrapper(pAddress, &mi, sizeof(mi));
	return (mi.State == MEM_COMMIT && (mi.Protect & PAGE_EXECUTE_FLAGS));
}

void PostMappingInit(HMODULE hModule, PVOID lpArg)
{
	VirtualFree(lpArg, 0, MEM_RELEASE);
	EnableExceptions((DWORD64)hModule);
	RegisterCustomLdrEntry(hModule);

	PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(((DWORD_PTR)hModule) + ((PIMAGE_DOS_HEADER)((DWORD_PTR)hModule))->e_lfanew);
	ntdllBaseAddress = (LPVOID)GetModuleHandleA("ntdll.dll");
	allocatedMemoryAddress = (LPVOID)hModule;
	allocatedMemorySize = (SIZE_T)ntHeaders->OptionalHeader.SizeOfImage;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	InstallHook("ntdll.dll", "NtQueryVirtualMemory", (LPVOID*)&OriginalNtQueryVirtualMemory, HookedNtQueryVirtualMemory);
	InstallHook("ntdll.dll", "NtResumeThread", (LPVOID*)&OriginalNtResumeThread, HookedNtResumeThread);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	pInstallHook((LPVOID)IsExecutableAddress, (LPVOID*)&OriginalIsExecutableAddress, HookedIsExecutableAddress);
	DetourTransactionCommit();
}

void BlockThread()
{
	while (TRUE)
	{
		Sleep(INFINITE);
	}
}

#pragma optimize("", off)
EXTERN_C __declspec(dllexport) BOOL WINAPI MapSainan(LPBYTE dllBase, LPBYTE allocatedMemory)
{
	dllBase = (LPBYTE)0xC0DEC0DEC0DEC0DE;
	allocatedMemory = (LPBYTE)0xC0DEC0DEC0DEC0DE;

	NT_NTFLUSHINSTRUCTIONCACHE ntFlushInstructionCache = (NT_NTFLUSHINSTRUCTIONCACHE)PebGetProcAddress(0x3cfa685d, 0x534c0ab8);
	NT_LOADLIBRARYA loadLibraryA = (NT_LOADLIBRARYA)PebGetProcAddress(0x6a4abc5b, 0xec0e4e8e);
	NT_GETPROCADDRESS getProcAddress = (NT_GETPROCADDRESS)PebGetProcAddress(0x6a4abc5b, 0x7c0dfcaa);
	NT_VIRTUALALLOC virtualAlloc = (NT_VIRTUALALLOC)PebGetProcAddress(0x6a4abc5b, 0x91afca54);
	NT_VIRTUALPROTECT virtualProtect = (NT_VIRTUALPROTECT)PebGetProcAddress(0x6a4abc5b, 0x7946c61b);
	
	if (ntFlushInstructionCache && loadLibraryA && getProcAddress && virtualAlloc && virtualProtect)
	{
		PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(dllBase + ((PIMAGE_DOS_HEADER)dllBase)->e_lfanew);

		if (allocatedMemory)
		{
			i_memcpy(allocatedMemory, dllBase, ntHeaders->OptionalHeader.SizeOfHeaders);

			DWORD oldProtect;
			if (!virtualProtect(allocatedMemory, ntHeaders->OptionalHeader.SizeOfHeaders, PAGE_READONLY, &oldProtect))
			{
				return FALSE;
			}

			PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)((LPBYTE)&ntHeaders->OptionalHeader + ntHeaders->FileHeader.SizeOfOptionalHeader);
			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				i_memcpy(allocatedMemory + sections[i].VirtualAddress, dllBase + sections[i].PointerToRawData, sections[i].SizeOfRawData);
			}

			PIMAGE_DATA_DIRECTORY importDirectory = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
			if (importDirectory->Size)
			{
				for (PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(allocatedMemory + importDirectory->VirtualAddress); importDescriptor->Name; importDescriptor++)
				{
					LPBYTE module = (LPBYTE)loadLibraryA((LPCSTR)(allocatedMemory + importDescriptor->Name));
					if (module)
					{
						PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)(allocatedMemory + importDescriptor->OriginalFirstThunk);
						PUINT_PTR importAddressTable = (PUINT_PTR)(allocatedMemory + importDescriptor->FirstThunk);

						while (*importAddressTable)
						{
							if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
							{
								PIMAGE_NT_HEADERS moduleNtHeaders = (PIMAGE_NT_HEADERS)(module + ((PIMAGE_DOS_HEADER)module)->e_lfanew);
								PIMAGE_EXPORT_DIRECTORY moduleExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(module + moduleNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
								*importAddressTable = (UINT_PTR)(module + *(LPDWORD)(module + moduleExportDirectory->AddressOfFunctions + (IMAGE_ORDINAL(thunk->u1.Ordinal) - moduleExportDirectory->Base) * sizeof(DWORD)));
							}
							else
							{
								importDirectory = (PIMAGE_DATA_DIRECTORY)(allocatedMemory + *importAddressTable);
								*importAddressTable = (UINT_PTR)getProcAddress((HMODULE)module, (LPCSTR)((PIMAGE_IMPORT_BY_NAME)importDirectory)->Name);
							}

							thunk = (PIMAGE_THUNK_DATA)((LPBYTE)thunk + sizeof(UINT_PTR));
							importAddressTable = (PUINT_PTR)((LPBYTE)importAddressTable + sizeof(UINT_PTR));
						}
					}
				}
			}

			PIMAGE_DATA_DIRECTORY relocationDirectory = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
			if (relocationDirectory->Size)
			{
				UINT_PTR imageBase = (UINT_PTR)(allocatedMemory - ntHeaders->OptionalHeader.ImageBase);

				for (PIMAGE_BASE_RELOCATION baseRelocation = (PIMAGE_BASE_RELOCATION)(allocatedMemory + relocationDirectory->VirtualAddress); baseRelocation->SizeOfBlock; baseRelocation = (PIMAGE_BASE_RELOCATION)((LPBYTE)baseRelocation + baseRelocation->SizeOfBlock))
				{
					LPBYTE relocationAddress = allocatedMemory + baseRelocation->VirtualAddress;
					PNT_IMAGE_RELOC relocations = (PNT_IMAGE_RELOC)((LPBYTE)baseRelocation + sizeof(IMAGE_BASE_RELOCATION));

					for (UINT_PTR i = 0; i < (baseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(NT_IMAGE_RELOC); i++)
					{
						if (relocations[i].Type == IMAGE_REL_BASED_DIR64)
						{
							*(PUINT_PTR)(relocationAddress + relocations[i].Offset) += imageBase;
						}
						else if (relocations[i].Type == IMAGE_REL_BASED_HIGHLOW)
						{
							*(LPDWORD)(relocationAddress + relocations[i].Offset) += (DWORD)imageBase;
						}
						else if (relocations[i].Type == IMAGE_REL_BASED_HIGH)
						{
							*(LPWORD)(relocationAddress + relocations[i].Offset) += HIWORD(imageBase);
						}
						else if (relocations[i].Type == IMAGE_REL_BASED_LOW)
						{
							*(LPWORD)(relocationAddress + relocations[i].Offset) += LOWORD(imageBase);
						}
					}
				}
			}

			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				if (!virtualProtect(allocatedMemory + sections[i].VirtualAddress, i == ntHeaders->FileHeader.NumberOfSections - 1 ? ntHeaders->OptionalHeader.SizeOfImage - sections[i].VirtualAddress : sections[i + 1].VirtualAddress - sections[i].VirtualAddress, SectionCharacteristicsToProtection(sections[i].Characteristics), &oldProtect))
				{
					return FALSE;
				}
			}

			PIMAGE_DATA_DIRECTORY pDataDir = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
			if (pDataDir->Size)
			{
				PIMAGE_TLS_DIRECTORY pTlsDir = RVA(PIMAGE_TLS_DIRECTORY, allocatedMemory, pDataDir->VirtualAddress);
				PIMAGE_TLS_CALLBACK* ppCallback = (PIMAGE_TLS_CALLBACK*)(pTlsDir->AddressOfCallBacks);
				for (; *ppCallback; ppCallback++)
				{
					(*ppCallback)((LPVOID)allocatedMemory, DLL_PROCESS_ATTACH, NULL);
				}
			}
			
			NT_DLLMAIN dllMain = (NT_DLLMAIN)(allocatedMemory + ntHeaders->OptionalHeader.AddressOfEntryPoint);

			ntFlushInstructionCache(INVALID_HANDLE_VALUE, NULL, 0);

			return dllMain((HINSTANCE)allocatedMemory, DLL_PROCESS_ATTACH, dllBase);
		}
	}

	return FALSE;
}
#pragma optimize("", on)