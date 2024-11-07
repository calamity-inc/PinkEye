#include <Windows.h>

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

EXTERN_C __declspec(dllexport) BOOL WINAPI MapSainan(LPBYTE dllBase)
{
	NT_NTFLUSHINSTRUCTIONCACHE ntFlushInstructionCache = (NT_NTFLUSHINSTRUCTIONCACHE)PebGetProcAddress(0x3cfa685d, 0x534c0ab8);
	NT_LOADLIBRARYA loadLibraryA = (NT_LOADLIBRARYA)PebGetProcAddress(0x6a4abc5b, 0xec0e4e8e);
	NT_GETPROCADDRESS getProcAddress = (NT_GETPROCADDRESS)PebGetProcAddress(0x6a4abc5b, 0x7c0dfcaa);
	NT_VIRTUALALLOC virtualAlloc = (NT_VIRTUALALLOC)PebGetProcAddress(0x6a4abc5b, 0x91afca54);
	NT_VIRTUALPROTECT virtualProtect = (NT_VIRTUALPROTECT)PebGetProcAddress(0x6a4abc5b, 0x7946c61b);

	if (ntFlushInstructionCache && loadLibraryA && getProcAddress && virtualAlloc && virtualProtect)
	{
		PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(dllBase + ((PIMAGE_DOS_HEADER)dllBase)->e_lfanew);

		LPBYTE allocatedMemory = (LPBYTE)virtualAlloc(NULL, ntHeaders->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
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

			return dllMain((HINSTANCE)allocatedMemory, DLL_PROCESS_ATTACH, NULL);
		}
	}

	return FALSE;
}