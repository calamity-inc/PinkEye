#pragma once

#include <ntifs.h>
#include <minwindef.h>
#include <ntstrsafe.h>

//#define DBG_PREFIX_ALL "InjectAll_"
//#define DBG_PREFIX DBG_PREFIX_ALL "Drv: "

#define DBG_PREFIX_ALL "[Output]"
#define DBG_PREFIX DBG_PREFIX_ALL ": "

//#define DbgPrintLine(s, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, s "\n", __VA_ARGS__)
//#define DbgPrintNoLine(s, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, DBG_PREFIX s, __VA_ARGS__)
#define DbgPrintLine(s, ...)

#define LIMIT_INJECTION_TO_PROC L"GTA5.exe"

#define STATIC_UNICODE_STRING(name, str) \
static const WCHAR label(__)[] = echo(L)str;\
static const UNICODE_STRING name = RTL_CONSTANT_STRINGW_(label(__))

#define STATIC_OBJECT_ATTRIBUTES(oa, name)\
	STATIC_UNICODE_STRING(label(m), name);\
	static OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, const_cast<PUNICODE_STRING>(&label(m)), OBJ_CASE_INSENSITIVE }

#define GCPFN_BUFF_SIZE (sizeof(UNICODE_STRING) + (MAX_PATH + 1) * sizeof(WCHAR))

enum IMAGE_LOAD_FLAGS_ENUM
{
	flImageNotifySet
} typedef IMAGE_LOAD_FLAGS;

enum SECTION_INFORMATION_CLASS_ENUM
{
	SectionBasicInformation,
	SectionImageInformation
} typedef SECTION_INFORMATION_CLASS;

struct SECTION_IMAGE_INFORMATION_STRUCT
{
	PVOID TransferAddress;
	ULONG ZeroBits;
	SIZE_T MaximumStackSize;
	SIZE_T CommittedStackSize;
	ULONG SubSystemType;
	union
	{
		struct s
		{
			USHORT SubSystemMinorVersion;
			USHORT SubSystemMajorVersion;
		};
		ULONG SubSystemVersion;
	};
	ULONG GpValue;
	USHORT ImageCharacteristics;
	USHORT DllCharacteristics;
	USHORT Machine;
	BOOLEAN ImageContainsCode;
	union
	{
		UCHAR ImageFlags;
		struct u
		{
			UCHAR ComPlusNativeReady : 1;
			UCHAR ComPlusILOnly : 1;
			UCHAR ImageDynamicallyRelocated : 1;
			UCHAR ImageMappedFlat : 1;
			UCHAR BaseBelow4gb : 1;
			UCHAR Reserved : 3;
		};
	};
	ULONG LoaderFlags;
	ULONG ImageFileSize;
	ULONG CheckSum;
} typedef SECTION_IMAGE_INFORMATION;

enum KAPC_ENVIRONMENT_ENUM
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment
} typedef KAPC_ENVIRONMENT;

typedef VOID __stdcall KNORMAL_ROUTINE(
	__in_opt PVOID NormalContext,
	__in_opt PVOID SystemArgument1,
	__in_opt PVOID SystemArgument2
);
typedef KNORMAL_ROUTINE* PKNORMAL_ROUTINE;

typedef VOID __stdcall KKERNEL_ROUTINE(
	__in struct _KAPC* Apc,
	__deref_inout_opt PKNORMAL_ROUTINE* NormalRoutine,
	__deref_inout_opt PVOID* NormalContext,
	__deref_inout_opt PVOID* SystemArgument1,
	__deref_inout_opt PVOID* SystemArgument2
);
typedef KKERNEL_ROUTINE* PKKERNEL_ROUTINE;

typedef VOID __stdcall KRUNDOWN_ROUTINE(
	__in struct _KAPC* Apc
);
typedef KRUNDOWN_ROUTINE* PKRUNDOWN_ROUTINE;

__declspec(dllimport) PIMAGE_NT_HEADERS RtlImageNtHeader(PVOID Base);
__declspec(dllimport) PVOID RtlImageDirectoryEntryToData(PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size);

__declspec(dllimport) BOOLEAN PsIsProcessBeingDebugged(PEPROCESS Process);

__declspec(dllimport) NTSTATUS ZwQueryInformationProcess
(
	IN HANDLE ProcessHandle,
	IN  PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

__declspec(dllimport) void KeInitializeApc(
	IN PKAPC Apc,
	IN PKTHREAD Thread,
	IN KAPC_ENVIRONMENT ApcIndex,
	IN PKKERNEL_ROUTINE KernelRoutine,
	IN PKRUNDOWN_ROUTINE RundownRoutine,
	IN PKNORMAL_ROUTINE NormalRoutine,
	IN ULONG ApcMode,
	IN PVOID NormalContext
);

__declspec(dllimport) BOOLEAN KeInsertQueueApc(
	IN PKAPC Apc,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2,
	IN ULONG PriorityIncrement
);

__declspec(dllimport) NTSTATUS ZwQuerySection(
	IN HANDLE SectionHandle,
	IN ULONG SectionInformationClass,
	OUT PVOID SectionInformation,
	IN ULONG SectionInformationLength,
	OUT PSIZE_T ResultLength OPTIONAL
);

__declspec(dllimport) NTSTATUS MmMapViewOfSection(
	IN PVOID SectionToMap,
	IN PEPROCESS Process,
	IN OUT PVOID* CapturedBase,
	IN ULONG_PTR ZeroBits,
	IN SIZE_T CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset,
	IN OUT PSIZE_T CapturedViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType,
	IN ULONG Protect
);

__declspec(dllimport) NTSTATUS MmUnmapViewOfSection(
	IN PEPROCESS Process,
	IN PVOID BaseAddress
);

__declspec(dllimport) BOOLEAN NTAPI KeTestAlertThread(IN KPROCESSOR_MODE AlertMode);