#include "DrvTypes.h"

BOOLEAN IsSuffixedUnicodeString(PCUNICODE_STRING FullName, PCUNICODE_STRING ShortName, BOOLEAN CaseInsensitive)
{
	if (FullName && ShortName && ShortName->Length <= FullName->Length)
	{
		UNICODE_STRING ustr = {
			ShortName->Length,
			ustr.Length,
			(PWSTR)RtlOffsetToPointer(FullName->Buffer, FullName->Length - ustr.Length)
		};

		return RtlEqualUnicodeString(&ustr, ShortName, CaseInsensitive);
	}

	return FALSE;
}

BOOLEAN IsMappedByLdrLoadDll(PCUNICODE_STRING ShortName)
{
	UNICODE_STRING Name;

	__try
	{
		PNT_TIB Teb = (PNT_TIB)PsGetCurrentThreadTeb();
		if (!Teb || !Teb->ArbitraryUserPointer)
		{
			return FALSE;
		}

		Name.Buffer = (PWSTR)Teb->ArbitraryUserPointer;

		ProbeForRead(Name.Buffer, sizeof(WCHAR), __alignof(WCHAR));

		Name.Length = (USHORT)wcsnlen(Name.Buffer, MAXSHORT);
		if (Name.Length == MAXSHORT)
		{
			return FALSE;
		}

		Name.Length *= sizeof(WCHAR);
		Name.MaximumLength = Name.Length;

		return IsSuffixedUnicodeString(&Name, ShortName, TRUE);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrintLine("#EXCEPTION: (0x%X) IsMappedByLdrLoadDll", GetExceptionCode());
	}

	return FALSE;
}


PCWSTR debugGetCurrentProcName(char* pBuff, size_t szcbLn, BOOL bFileNameOnly)
{
	if (!pBuff || szcbLn < (sizeof(UNICODE_STRING) + 1 * sizeof(WCHAR)) || szcbLn > MAXUSHORT)
	{
		ASSERT(NULL);
		return L"-";
	}

	UNICODE_STRING* puStr = (UNICODE_STRING*)pBuff;
	PWCH pWBuff = (PWCH)((BYTE*)pBuff + sizeof(UNICODE_STRING));
	puStr->Length = 0;
	puStr->MaximumLength = (USHORT)(szcbLn - sizeof(UNICODE_STRING));
	puStr->Buffer = pWBuff;

	ULONG uicbSzRet = 0;
	NTSTATUS status = ZwQueryInformationProcess(NtCurrentProcess(), ProcessImageFileName, puStr, (ULONG)szcbLn, &uicbSzRet);
	if (status == STATUS_SUCCESS)
	{
		*(WCHAR*)((BYTE*)pBuff + szcbLn - sizeof(WCHAR)) = 0;

		if (puStr->Length + sizeof(WCHAR) <= puStr->MaximumLength)
		{
			*(WCHAR*)((BYTE*)puStr->Buffer + puStr->Length) = 0;
		}

		if (bFileNameOnly)
		{
			WCHAR* pLastSlash = NULL;
			for (WCHAR* pS = pWBuff;; pS++)
			{
				WCHAR z = *pS;
				if (!z)
				{
					if (pLastSlash)
					{
						return pLastSlash + 1;
					}
					break;
				}
				else if (z == L'\\')
				{
					pLastSlash = pS;
				}
			}
		}
	}
	else
	{
		if (RtlStringCchPrintfW(pWBuff, (szcbLn - sizeof(UNICODE_STRING)) / sizeof(WCHAR), L"<Err:0x%x>", status) != STATUS_SUCCESS)
		{
			ASSERT(NULL);
			return L"?";
		}
	}

	return pWBuff;
}

BOOLEAN IsSpecificProcessW(HANDLE ProcessId, const WCHAR* ImageName, BOOLEAN bIsDebugged)
{
	ASSERT(ImageName);
	BOOLEAN bResult = FALSE;

	PEPROCESS Process;
	if (NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &Process)))
	{
		if (!bIsDebugged || PsIsProcessBeingDebugged(Process))
		{
			HANDLE hProc;
			if (ObOpenObjectByPointer(Process, OBJ_KERNEL_HANDLE, NULL, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &hProc) == STATUS_SUCCESS)
			{
				WCHAR buff[GCPFN_BUFF_SIZE];
				UNICODE_STRING* puStr = (UNICODE_STRING*)buff;
				PWCH pWBuff = (PWCH)((BYTE*)buff + sizeof(UNICODE_STRING));
				puStr->Length = 0;
				puStr->MaximumLength = (USHORT)(sizeof(buff) - sizeof(UNICODE_STRING));
				puStr->Buffer = pWBuff;

				if (ZwQueryInformationProcess(hProc, ProcessImageFileName, puStr, sizeof(buff), NULL) == STATUS_SUCCESS)
				{
					*(WCHAR*)((BYTE*)buff + sizeof(buff) - sizeof(WCHAR)) = 0;

					if (puStr->Length + sizeof(WCHAR) <= puStr->MaximumLength)
					{
						*(WCHAR*)((BYTE*)puStr->Buffer + puStr->Length) = 0;
					}

					WCHAR* pLastSlash = NULL;
					for (WCHAR* pS = pWBuff;; pS++)
					{
						WCHAR z = *pS;
						if (!z)
						{
							if (pLastSlash)
							{
								pWBuff = pLastSlash + 1;
							}
							break;
						}
						else if (z == L'\\')
						{
							pLastSlash = pS;
						}
					}

					if (_wcsicmp(ImageName, pWBuff) == 0)
					{
						bResult = TRUE;
					}
				}

				ZwClose(hProc);
			}
		}

		ObDereferenceObject(Process);
	}

	return bResult;
}