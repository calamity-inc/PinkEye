#include <ntifs.h>
#include <windef.h>

typedef unsigned __int64 QWORD;

typedef struct _OLD_CALLBACKS {
	QWORD PreOperationProc;
	QWORD PostOperationProc;
	QWORD PreOperationThread;
	QWORD PostOperationThread;
} OLD_CALLBACKS, * POLD_CALLBACKS;

typedef struct _CALLBACK_ENTRY {
	WORD Version;
	WORD OperationRegistrationCount;
	DWORD unk1;
	PVOID RegistrationContext;
	UNICODE_STRING Altitude;
} CALLBACK_ENTRY, * PCALLBACK_ENTRY;

typedef struct _CALLBACK_ENTRY_ITEM {
	LIST_ENTRY CallbackList;
	OB_OPERATION Operations;
	DWORD Active;
	CALLBACK_ENTRY* CallbackEntry;
	PVOID ObjectType;
	POB_PRE_OPERATION_CALLBACK PreOperation;
	POB_POST_OPERATION_CALLBACK PostOperation;
	QWORD unk1;
} CALLBACK_ENTRY_ITEM, * PCALLBACK_ENTRY_ITEM;

OB_PREOP_CALLBACK_STATUS DummyObjectPreCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	return(OB_PREOP_SUCCESS);
}

VOID DummyObjectPostCallback(PVOID RegistrationContext, POB_POST_OPERATION_INFORMATION OperationInformation)
{
	return;
}

QWORD GetCallbackListOffset(void)
{
	POBJECT_TYPE procType = *PsProcessType;

	__try
	{
		if (procType && MmIsAddressValid((void*)procType))
		{
			for (int i = 0xF8; i > 0; i -= 8)
			{
				QWORD first = *(QWORD*)((QWORD)procType + i), second = *(QWORD*)((QWORD)procType + (i + 8));
				if (first && MmIsAddressValid((void*)first) && second && MmIsAddressValid((void*)second))
				{
					QWORD test1First = *(QWORD*)(first + 0x0), test1Second = *(QWORD*)(first + 0x8);
					if (test1First && MmIsAddressValid((void*)test1First) && test1Second && MmIsAddressValid((void*)test1Second))
					{
						QWORD testObjectType = *(QWORD*)(first + 0x20);
						if (testObjectType == (QWORD)procType)
						{
							return((QWORD)i);
						}
					}
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return(0);
	}
}

void DisableBEObjectCallbacks(POLD_CALLBACKS oldCallbacks, const char* Alt_Var)
{
	POBJECT_TYPE procType = *PsProcessType;
	if (procType && MmIsAddressValid((void*)procType))
	{
		__try
		{
			QWORD callbackListOffset = GetCallbackListOffset();
			if (callbackListOffset && MmIsAddressValid((void*)((QWORD)procType + callbackListOffset)))
			{
				LIST_ENTRY* callbackList = (LIST_ENTRY*)((QWORD)procType + callbackListOffset);
				if (callbackList->Flink && MmIsAddressValid((void*)callbackList->Flink))
				{
					CALLBACK_ENTRY_ITEM* firstCallback = (CALLBACK_ENTRY_ITEM*)callbackList->Flink;
					CALLBACK_ENTRY_ITEM* curCallback = firstCallback;

					do
					{
						if (curCallback && MmIsAddressValid((void*)curCallback) && MmIsAddressValid((void*)curCallback->CallbackEntry))
						{
							ANSI_STRING altitudeAnsi = { 0 };
							UNICODE_STRING altitudeUni = curCallback->CallbackEntry->Altitude;
							RtlUnicodeStringToAnsiString(&altitudeAnsi, &altitudeUni, 1);

							if (!strcmp(altitudeAnsi.Buffer, Alt_Var)) //BattlEye should be "363220".
							{
								if (curCallback->PreOperation)
								{
									oldCallbacks->PreOperationProc = (QWORD)curCallback->PreOperation;
									curCallback->PreOperation = DummyObjectPreCallback;
								}
								if (curCallback->PostOperation)
								{
									oldCallbacks->PostOperationProc = (QWORD)curCallback->PostOperation;
									curCallback->PostOperation = DummyObjectPostCallback;
								}
								RtlFreeAnsiString(&altitudeAnsi);
								break;
							}

							RtlFreeAnsiString(&altitudeAnsi);
						}

						curCallback = (CALLBACK_ENTRY_ITEM*)curCallback->CallbackList.Flink;
					} while (curCallback != firstCallback);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return;
		}
	}

	POBJECT_TYPE threadType = *PsThreadType;
	if (threadType && MmIsAddressValid((void*)threadType))
	{
		__try
		{
			QWORD callbackListOffset = GetCallbackListOffset();
			if (callbackListOffset && MmIsAddressValid((void*)((QWORD)threadType + callbackListOffset)))
			{
				LIST_ENTRY* callbackList = (LIST_ENTRY*)((QWORD)threadType + callbackListOffset);
				if (callbackList->Flink && MmIsAddressValid((void*)callbackList->Flink))
				{
					CALLBACK_ENTRY_ITEM* firstCallback = (CALLBACK_ENTRY_ITEM*)callbackList->Flink;
					CALLBACK_ENTRY_ITEM* curCallback = firstCallback;

					do
					{
						if (curCallback && MmIsAddressValid((void*)curCallback) && MmIsAddressValid((void*)curCallback->CallbackEntry))
						{
							ANSI_STRING altitudeAnsi = { 0 };
							UNICODE_STRING altitudeUni = curCallback->CallbackEntry->Altitude;
							RtlUnicodeStringToAnsiString(&altitudeAnsi, &altitudeUni, 1);

							if (!strcmp(altitudeAnsi.Buffer, Alt_Var)) //BattlEye should be "363220".
							{
								if (curCallback->PreOperation)
								{
									oldCallbacks->PreOperationThread = (QWORD)curCallback->PreOperation;
									curCallback->PreOperation = DummyObjectPreCallback;
								}
								if (curCallback->PostOperation)
								{
									oldCallbacks->PostOperationThread = (QWORD)curCallback->PostOperation;
									curCallback->PostOperation = DummyObjectPostCallback;
								}
								RtlFreeAnsiString(&altitudeAnsi);
								break;
							}

							RtlFreeAnsiString(&altitudeAnsi);
						}

						curCallback = (CALLBACK_ENTRY_ITEM*)curCallback->CallbackList.Flink;
					} while (curCallback != firstCallback);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return;
		}
	}
}

void RestoreBEObjectCallbacks(POLD_CALLBACKS oldCallbacks, const char* Alt_Var)
{
	POBJECT_TYPE procType = *PsProcessType;
	if (procType && MmIsAddressValid((void*)procType))
	{
		__try
		{
			QWORD callbackListOffset = GetCallbackListOffset();
			if (callbackListOffset && MmIsAddressValid((void*)((QWORD)procType + callbackListOffset)))
			{
				LIST_ENTRY* callbackList = (LIST_ENTRY*)((QWORD)procType + callbackListOffset);
				if (callbackList->Flink && MmIsAddressValid((void*)callbackList->Flink))
				{
					CALLBACK_ENTRY_ITEM* firstCallback = (CALLBACK_ENTRY_ITEM*)callbackList->Flink;
					CALLBACK_ENTRY_ITEM* curCallback = firstCallback;

					do
					{
						if (curCallback && MmIsAddressValid((void*)curCallback) && MmIsAddressValid((void*)curCallback->CallbackEntry))
						{
							ANSI_STRING altitudeAnsi = { 0 };
							UNICODE_STRING altitudeUni = curCallback->CallbackEntry->Altitude;
							RtlUnicodeStringToAnsiString(&altitudeAnsi, &altitudeUni, 1);

							if (!strcmp(altitudeAnsi.Buffer, Alt_Var)) //BattlEye should be "363220".
							{
								if (curCallback->PreOperation && oldCallbacks->PreOperationProc)
								{
									curCallback->PreOperation = (POB_PRE_OPERATION_CALLBACK)oldCallbacks->PreOperationProc;
								}
								if (curCallback->PostOperation && oldCallbacks->PostOperationProc)
								{
									curCallback->PostOperation = (POB_POST_OPERATION_CALLBACK)oldCallbacks->PostOperationProc;
								}
								RtlFreeAnsiString(&altitudeAnsi);
								break;
							}

							RtlFreeAnsiString(&altitudeAnsi);
						}

						curCallback = (CALLBACK_ENTRY_ITEM*)curCallback->CallbackList.Flink;
					} while (curCallback != firstCallback);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return;
		}
	}

	POBJECT_TYPE threadType = *PsThreadType;
	if (threadType && MmIsAddressValid((void*)threadType))
	{
		__try
		{
			QWORD callbackListOffset = GetCallbackListOffset();
			if (callbackListOffset && MmIsAddressValid((void*)((QWORD)threadType + callbackListOffset)))
			{
				LIST_ENTRY* callbackList = (LIST_ENTRY*)((QWORD)threadType + callbackListOffset);
				if (callbackList->Flink && MmIsAddressValid((void*)callbackList->Flink))
				{
					CALLBACK_ENTRY_ITEM* firstCallback = (CALLBACK_ENTRY_ITEM*)callbackList->Flink;
					CALLBACK_ENTRY_ITEM* curCallback = firstCallback;

					do
					{
						if (curCallback && MmIsAddressValid((void*)curCallback) && MmIsAddressValid((void*)curCallback->CallbackEntry))
						{
							ANSI_STRING altitudeAnsi = { 0 };
							UNICODE_STRING altitudeUni = curCallback->CallbackEntry->Altitude;
							RtlUnicodeStringToAnsiString(&altitudeAnsi, &altitudeUni, 1);

							if (!strcmp(altitudeAnsi.Buffer, Alt_Var)) //BattlEye should be "363220".
							{
								if (curCallback->PreOperation && oldCallbacks->PreOperationThread)
								{
									curCallback->PreOperation = (POB_PRE_OPERATION_CALLBACK)oldCallbacks->PreOperationThread;
								}
								if (curCallback->PostOperation && oldCallbacks->PostOperationThread)
								{
									curCallback->PostOperation = (POB_POST_OPERATION_CALLBACK)oldCallbacks->PostOperationThread;
								}
								RtlFreeAnsiString(&altitudeAnsi);
								break;
							}

							RtlFreeAnsiString(&altitudeAnsi);
						}

						curCallback = (CALLBACK_ENTRY_ITEM*)curCallback->CallbackList.Flink;
					} while (curCallback != firstCallback);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return;
		}
	}
}