//#include <fltKernel.h>
//#include "Utils.h"
//#include "ObCallbacks.h"
//#include "IATHook.h"
//
//PDRIVER_OBJECT g_DriverObject = NULL;
//IMAGE_LOAD_FLAGS g_Flags;
//#define kMiniFiltersList_POOL_TAG 'iwik'
//
//BOOL hook_BattlEye_Once = FALSE;
//
//BOOL unloadLock = FALSE;
//
//BOOL hook_ObRegisterCallbacks_Once = FALSE;
//
////BOOL hook_FltRegisterFilter_Once = FALSE;
////PFLT_FILTER HookedFilterHandle = NULL;
//
//BOOL BE_OBCallbacksRegistered = FALSE;
//BOOL disabled_BEOBCallbacks = FALSE;
//BOOL copied_BEAltitude = FALSE;
//char BE_Altitude[MAX_PATH];
//
//OLD_CALLBACKS oldCallbacks = { 0 };
//
//void DisableObCallbacks(const char* Altitude)
//{
//	DisableBEObjectCallbacks(&oldCallbacks, Altitude);
//	DbgPrintLine("Disabled BEDaisy ObCallbacks");
//}
//
//void EnableObCallbacks(const char* Altitude)
//{
//	RestoreBEObjectCallbacks(&oldCallbacks, Altitude);
//	DbgPrintLine("Enabled BEDaisy ObCallbacks");
//}
//
////#define ExAllocatePoolWithTag_WhiteListSize 1000
////PVOID hook_ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag)
////{
////	static void* WhiteList[ExAllocatePoolWithTag_WhiteListSize] = { 0 };
////	static int size = 0;
////	void* ReturnAddress = _ReturnAddress();
////
////	for (int i = 0; i < size; i++)
////	{
////		if (WhiteList[i] == ReturnAddress)
////		{
////			return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
////		}
////	}
////	if (PoolType == 1 && NumberOfBytes == 24)
////	{
////		DbgPrintLine("ExAllocatePoolWithTag called from: 0x%p rejected!", ReturnAddress);
////
////		return NULL;
////	}
////	else
////	{
////		if (size < ExAllocatePoolWithTag_WhiteListSize)
////		{
////			WhiteList[size++] = ReturnAddress;
////			return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
////		}
////		else
////		{
////			DbgPrintLine("ExAllocatePoolWithTag WhiteList is full");
////
////			return NULL;
////		}
////	}
////}
////
////#define ExAllocatePool_WhiteListSize 1000
////PVOID hook_ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes)
////{
////	static void* WhiteList[ExAllocatePool_WhiteListSize] = { 0 };
////	static int size = 0;
////	void* ReturnAddress = _ReturnAddress();
////
////	for (int i = 0; i < size; i++)
////	{
////		if (WhiteList[i] == ReturnAddress)
////		{
////			return ExAllocatePool(PoolType, NumberOfBytes);
////		}
////	}
////	if (PoolType == 1 && NumberOfBytes == 24)
////	{
////		DbgPrintLine("ExAllocatePool called from: 0x%p rejected!", ReturnAddress);
////
////		return NULL;
////	}
////	else
////	{
////		if (size < ExAllocatePool_WhiteListSize)
////		{
////			WhiteList[size++] = ReturnAddress;
////			return ExAllocatePool(PoolType, NumberOfBytes);
////		}
////		else
////		{
////			DbgPrintLine("ExAllocatePool WhiteList is full");
////
////			return NULL;
////		}
////	}
////}
//
//NTSTATUS hook_ObRegisterCallbacks(_In_ POB_CALLBACK_REGISTRATION CallbackRegistration, _Outptr_ PVOID* RegistrationHandle)
//{
//	SIZE_T bufferSize = sizeof(BE_Altitude);
//
//	NTSTATUS copyStatus;
//	ULONG bytesWritten;
//
//	copyStatus = RtlUnicodeToMultiByteN(BE_Altitude, bufferSize, &bytesWritten, CallbackRegistration->Altitude.Buffer, CallbackRegistration->Altitude.Length);
//
//	if (NT_SUCCESS(copyStatus))
//	{
//		BE_Altitude[bytesWritten] = '\0';
//		copied_BEAltitude = TRUE;
//
//		DbgPrintLine("[INTERNAL] - BattlEye Altitude: %s", BE_Altitude);
//	}
//
//	NTSTATUS status = ObRegisterCallbacks(CallbackRegistration, RegistrationHandle);
//
//	if (NT_SUCCESS(status))
//	{
//		BE_OBCallbacksRegistered = TRUE;
//	}
//
//	return status;
//}
//
//typedef PVOID(__stdcall* typedef_MmGetSystemRoutineAddress)(PUNICODE_STRING SystemRoutineName);
//HANDLE g_hHook_MmGetSystemRoutineAddress = NULL;
//void* hook_MmGetSystemRoutineAddress(UNICODE_STRING* routine_name)
//{
//	/*if (wcsstr(routine_name->Buffer, L"ExAllocatePoolWithTag"))
//	{
//		DbgPrintLine("[Memory Tag Allocation ALERT] - REDIRECTING BEDaisy TO ->: 0x%p", hook_ExAllocatePoolWithTag);
//
//		return &hook_ExAllocatePoolWithTag;
//	}
//	else if (wcsstr(routine_name->Buffer, L"ExAllocatePool"))
//	{
//		DbgPrintLine("[Memory Allocation ALERT] - REDIRECTING BEDaisy TO ->: 0x%p", hook_ExAllocatePool);
//
//		return &hook_ExAllocatePool;
//	}
//	else if (wcsstr(routine_name->Buffer, L"ObRegisterCallbacks") && hook_ObRegisterCallbacks_Once == FALSE)
//	{
//		DbgPrintLine("[OB REGISTER ALERT] - REDIRECTING BEDaisy TO ->: 0x%p", hook_ObRegisterCallbacks);
//
//		hook_ObRegisterCallbacks_Once = TRUE;
//		return &hook_ObRegisterCallbacks;
//	}*/
//	if (wcsstr(routine_name->Buffer, L"ObRegisterCallbacks") && hook_ObRegisterCallbacks_Once == FALSE)
//	{
//		DbgPrintLine("[OB REGISTER ALERT] - REDIRECTING BEDaisy TO ->: 0x%p", hook_ObRegisterCallbacks);
//
//		hook_ObRegisterCallbacks_Once = TRUE;
//		return &hook_ObRegisterCallbacks;
//	}
//	else
//	{
//		DbgPrintLine("BEDaisy requested function name: %wZ", routine_name);
//
//		typedef_MmGetSystemRoutineAddress fnOrigin = (typedef_MmGetSystemRoutineAddress)GetIATHookOrign(g_hHook_MmGetSystemRoutineAddress);
//		return fnOrigin(routine_name);
//	}
//}
//
////NTSTATUS hook_FltStartFiltering(PFLT_FILTER Filter)
////{
////	if (Filter != HookedFilterHandle)
////	{
////		return FltStartFiltering(Filter);
////	}
////	else
////	{
////		DbgPrintLine("Prevented BEDaisy from starting FLT filter");
////		return STATUS_SUCCESS;
////	}
////}
////
////NTSTATUS hook_FltRegisterFilter(PDRIVER_OBJECT Driver, const FLT_REGISTRATION* Registration, PFLT_FILTER* RetFilter)
////{
////	DbgPrintLine("BEDaisy created a filter callback at address precallback->: 0x%p", Registration->OperationRegistration->PreOperation);
////
////	NTSTATUS status = FltRegisterFilter(Driver, Registration, &HookedFilterHandle);
////	*RetFilter = HookedFilterHandle;
////	return status;
////}
////
////typedef PVOID(FLTAPI* typedef_FltGetRoutineAddress)(PCSTR FltMgrRoutineName);
////HANDLE g_hHook_FltGetRoutineAddress = NULL;
////PVOID hook_FltGetRoutineAddress(PCSTR FltMgrRoutineName)
////{
////	DbgPrintLine("BEDaisy requested FLT function ->: %s", FltMgrRoutineName);
////
////	if (strcmp(FltMgrRoutineName, "FltRegisterFilter") == 0 && hook_FltRegisterFilter_Once == FALSE)
////	{
////		hook_FltRegisterFilter_Once = TRUE;
////		DbgPrintLine("[FLT_REGISTER ALERT] - REDIRECTING BEDaisy TO ->: 0x%p", hook_FltRegisterFilter);
////		return &hook_FltRegisterFilter;
////	}
////	else if (strcmp(FltMgrRoutineName, "FltStartFiltering") == 0)
////	{
////		DbgPrintLine("[FLT_CALLBACK START ALERT] - REDIRECTING BEDaisy TO ->: 0x%p", hook_FltStartFiltering);
////		return &hook_FltStartFiltering;
////	}
////	else
////	{
////		typedef_FltGetRoutineAddress fnOrigin = (typedef_FltGetRoutineAddress)GetIATHookOrign(g_hHook_FltGetRoutineAddress);
////		return fnOrigin(FltMgrRoutineName);
////	}
////}
//
//void OnLoadImage(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
//{
//	UNREFERENCED_PARAMETER(FullImageName);
//	UNREFERENCED_PARAMETER(ProcessId);
//	UNREFERENCED_PARAMETER(ImageInfo);
//
//	if ((ProcessId == NULL || ProcessId == (HANDLE)4) && FullImageName != NULL && wcsstr(FullImageName->Buffer, L"BEDaisy.sys"))
//	{
//		if (hook_BattlEye_Once == FALSE)
//		{
//			hook_BattlEye_Once = TRUE;
//
//			//IATHook(ImageInfo->ImageBase, "fltMgr.sys", "FltGetRoutineAddress", hook_FltGetRoutineAddress, &g_hHook_FltGetRoutineAddress);
//			IATHook(ImageInfo->ImageBase, "NtosKrnl.exe", "MmGetSystemRoutineAddress", hook_MmGetSystemRoutineAddress, &g_hHook_MmGetSystemRoutineAddress);
//		}
//	}
//	else
//	{
//		NTSTATUS status;
//
//		ASSERT(FullImageName);
//		ASSERT(ImageInfo);
//
//		UNICODE_STRING kernel32;
//		RtlInitUnicodeString(&kernel32, L"\\sfc_os.dll");
//
//		if (!ImageInfo->SystemModeImage && ProcessId == PsGetCurrentProcessId() && IsSuffixedUnicodeString(FullImageName, &kernel32, TRUE) && IsMappedByLdrLoadDll(&kernel32) && IsSpecificProcessW(ProcessId, LIMIT_INJECTION_TO_PROC, FALSE))
//		{
//			if (unloadLock == FALSE)
//			{
//				unloadLock = TRUE;
//
//				if (copied_BEAltitude == TRUE && BE_OBCallbacksRegistered == TRUE)
//				{
//					disabled_BEOBCallbacks = TRUE;
//					DisableObCallbacks(BE_Altitude);
//				}
//			}
//		}
//	}
//}
//
//BOOL kMiniFiltersList(PCWSTR targetMiniFilterName)
//{
//	UNICODE_STRING targetFilterName;
//	RtlInitUnicodeString(&targetFilterName, targetMiniFilterName);
//
//	NTSTATUS status;
//	ULONG i, j, k;
//	ULONG NumberFiltersReturned = 0;
//	PFLT_FILTER* FilterList = NULL;
//	ULONG BytesReturned = 0;
//	PFILTER_FULL_INFORMATION myFilterFullInformation = NULL;
//	PFLT_INSTANCE* InstanceList = NULL;
//	ULONG NumberInstancesReturned = 0;
//	PFLT_VOLUME RetVolume = NULL;
//	PVOID monCallBack, preCallBack, postCallBack;
//
//	BOOL filterFound = FALSE;
//
//	status = FltEnumerateFilters(NULL, 0, &NumberFiltersReturned);
//	if ((status == STATUS_BUFFER_TOO_SMALL) && (NumberFiltersReturned > 0))
//	{
//		FilterList = (PFLT_FILTER*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PFLT_FILTER) * NumberFiltersReturned, kMiniFiltersList_POOL_TAG);
//		if (FilterList != NULL)
//		{
//			status = FltEnumerateFilters(FilterList, sizeof(PFLT_FILTER) * NumberFiltersReturned, &NumberFiltersReturned);
//			for (i = 0; (i < NumberFiltersReturned) && NT_SUCCESS(status); i++)
//			{
//				status = FltGetFilterInformation(FilterList[i], FilterFullInformation, NULL, 0, &BytesReturned);
//				if ((status == STATUS_BUFFER_TOO_SMALL) && (BytesReturned > 0))
//				{
//					myFilterFullInformation = (PFILTER_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, BytesReturned, kMiniFiltersList_POOL_TAG);
//					if (myFilterFullInformation != NULL)
//					{
//						status = FltGetFilterInformation(FilterList[i], FilterFullInformation, myFilterFullInformation, BytesReturned, &BytesReturned);
//						if (NT_SUCCESS(status))
//						{
//							UNICODE_STRING FilterName;
//							FilterName.Length = (USHORT)myFilterFullInformation->FilterNameLength;
//							FilterName.MaximumLength = (USHORT)myFilterFullInformation->FilterNameLength;
//							FilterName.Buffer = myFilterFullInformation->FilterNameBuffer;
//
//							DbgPrintLine("Filter Driver Name: %wZ\n", &FilterName);
//
//							if (RtlCompareUnicodeString(&FilterName, &targetFilterName, FALSE) == 0)
//							{
//								DbgPrintLine("Found BattlEye!\n");
//								filterFound = TRUE;
//							}
//						}
//						ExFreePoolWithTag(myFilterFullInformation, kMiniFiltersList_POOL_TAG);
//					}
//				}
//				FltObjectDereference(FilterList[i]);
//			}
//			ExFreePoolWithTag(FilterList, kMiniFiltersList_POOL_TAG);
//		}
//	}
//	return filterFound;
//}
//
//void NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
//{
//	DbgPrintLine("BattlEye Altitude: %s", BE_Altitude);
//
//	BOOL filterFound = kMiniFiltersList(L"BEDaisy");
//
//	/*if (g_hHook_FltGetRoutineAddress != NULL && filterFound == TRUE)
//	{
//		if (HookedFilterHandle != NULL)
//		{
//			FltStartFiltering(HookedFilterHandle);
//		}
//		UnIATHook(g_hHook_FltGetRoutineAddress);
//	}*/
//	if (g_hHook_MmGetSystemRoutineAddress != NULL && filterFound == TRUE)
//	{
//		UnIATHook(g_hHook_MmGetSystemRoutineAddress);
//	}
//	if (disabled_BEOBCallbacks == TRUE)
//	{
//		EnableObCallbacks(BE_Altitude);
//	}
//
//	NTSTATUS status = STATUS_SUCCESS;
//	if (_bittestandreset((LONG*)&g_Flags, flImageNotifySet))
//	{
//		status = PsRemoveLoadImageNotifyRoutine(OnLoadImage);
//		if (!NT_SUCCESS(status))
//		{
//			DbgPrintLine("CRITICAL: (0x%X) PsRemoveLoadImageNotifyRoutine", status);
//		}
//	}
//
//	DbgPrintLine("DriverUnload(0x%p), status=0x%x", DriverObject, status);
//}
//
//NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
//{
//	UNREFERENCED_PARAMETER(DriverObject);
//	UNREFERENCED_PARAMETER(RegistryPath);
//
//	DbgPrintLine("DriverLoad(0x%p, %wZ)", DriverObject, RegistryPath);
//
//	g_DriverObject = DriverObject;
//
//	DriverObject->DriverUnload = DriverUnload;
//
//	NTSTATUS status = PsSetLoadImageNotifyRoutine(OnLoadImage);
//	if (NT_SUCCESS(status))
//	{
//		_bittestandset((LONG*)&g_Flags, flImageNotifySet);
//	}
//	else
//	{
//		DbgPrintLine("CRITICAL: (0x%X) PsSetLoadImageNotifyRoutine", status);
//	}
//
//	return status;
//}

#include <fltKernel.h>
#include "Utils.h"
#include "ObCallbacks.h"

PDRIVER_OBJECT g_DriverObject = NULL;

OLD_CALLBACKS oldCallbacks = { 0 };

BOOL disabledBECallbacks = FALSE;

void DisableObCallbacks(const char* Altitude)
{
	DisableBEObjectCallbacks(&oldCallbacks, Altitude);
	DbgPrintLine("Disabled BEDaisy ObCallbacks");
}

void EnableObCallbacks(const char* Altitude)
{
	RestoreBEObjectCallbacks(&oldCallbacks, Altitude);
	DbgPrintLine("Enabled BEDaisy ObCallbacks");
}

#define kMiniFiltersList_POOL_TAG 'iwik'

BOOL kMiniFiltersList(PCWSTR targetMiniFilterName)
{
	UNICODE_STRING targetFilterName;
	RtlInitUnicodeString(&targetFilterName, targetMiniFilterName);

	NTSTATUS status;
	ULONG i, j, k;
	ULONG NumberFiltersReturned = 0;
	PFLT_FILTER* FilterList = NULL;
	ULONG BytesReturned = 0;
	PFILTER_FULL_INFORMATION myFilterFullInformation = NULL;
	PFLT_INSTANCE* InstanceList = NULL;
	ULONG NumberInstancesReturned = 0;
	PFLT_VOLUME RetVolume = NULL;
	PVOID monCallBack, preCallBack, postCallBack;

	BOOL filterFound = FALSE;

	status = FltEnumerateFilters(NULL, 0, &NumberFiltersReturned);
	if ((status == STATUS_BUFFER_TOO_SMALL) && (NumberFiltersReturned > 0))
	{
		FilterList = (PFLT_FILTER*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PFLT_FILTER) * NumberFiltersReturned, kMiniFiltersList_POOL_TAG);
		if (FilterList != NULL)
		{
			status = FltEnumerateFilters(FilterList, sizeof(PFLT_FILTER) * NumberFiltersReturned, &NumberFiltersReturned);
			for (i = 0; (i < NumberFiltersReturned) && NT_SUCCESS(status); i++)
			{
				status = FltGetFilterInformation(FilterList[i], FilterFullInformation, NULL, 0, &BytesReturned);
				if ((status == STATUS_BUFFER_TOO_SMALL) && (BytesReturned > 0))
				{
					myFilterFullInformation = (PFILTER_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, BytesReturned, kMiniFiltersList_POOL_TAG);
					if (myFilterFullInformation != NULL)
					{
						status = FltGetFilterInformation(FilterList[i], FilterFullInformation, myFilterFullInformation, BytesReturned, &BytesReturned);
						if (NT_SUCCESS(status))
						{
							UNICODE_STRING FilterName;
							FilterName.Length = (USHORT)myFilterFullInformation->FilterNameLength;
							FilterName.MaximumLength = (USHORT)myFilterFullInformation->FilterNameLength;
							FilterName.Buffer = myFilterFullInformation->FilterNameBuffer;

							DbgPrintLine("Filter Driver Name: %wZ\n", &FilterName);

							if (RtlCompareUnicodeString(&FilterName, &targetFilterName, FALSE) == 0)
							{
								DbgPrintLine("Found BattlEye!\n");
								filterFound = TRUE;
							}
						}
						ExFreePoolWithTag(myFilterFullInformation, kMiniFiltersList_POOL_TAG);
					}
				}
				FltObjectDereference(FilterList[i]);
			}
			ExFreePoolWithTag(FilterList, kMiniFiltersList_POOL_TAG);
		}
	}
	return filterFound;
}

void NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	BOOL filterFound = kMiniFiltersList(L"BEDaisy");

	if (filterFound == TRUE && disabledBECallbacks == TRUE)
	{
		EnableObCallbacks("363220");
	}

	DbgPrintLine("DriverUnload(0x%p), status=0x%x", DriverObject, status);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrintLine("DriverLoad(0x%p, %wZ)", DriverObject, RegistryPath);

	g_DriverObject = DriverObject;

	DriverObject->DriverUnload = DriverUnload;

	BOOL filterFound = kMiniFiltersList(L"BEDaisy");

	if (filterFound == TRUE)
	{
		DisableObCallbacks("363220");
		disabledBECallbacks = TRUE;
	}

	return STATUS_SUCCESS;
}