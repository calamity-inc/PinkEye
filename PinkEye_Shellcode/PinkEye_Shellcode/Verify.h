#include <Windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
#include <sfc.h>
#include <wchar.h>
#include <intrin.h>

typedef NTSTATUS(NTAPI* typedef_NtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
static typedef_NtCreateFile OriginalNtCreateFile;

typedef struct
{
    wchar_t HashFinalCert[MAX_PATH];
    wchar_t SubjectName[MAX_PATH];
} SignResult;

wchar_t* ExtractStringFromCertificate(PCCERT_CONTEXT pCertificate, DWORD dwType, DWORD dwFlags)
{
    DWORD size = CertGetNameStringW(pCertificate, dwType, dwFlags, NULL, NULL, 0);
    if (size)
    {
        wchar_t* buff = (wchar_t*)malloc(size * sizeof(wchar_t));
        if (buff)
        {
            CertGetNameStringW(pCertificate, dwType, dwFlags, NULL, buff, size);
            return buff;
        }
    }
    return NULL;
}

void GetSignerInfo(HANDLE hWVTStateData, SignResult* pSignResult)
{
    CRYPT_PROVIDER_DATA* pProvData = WTHelperProvDataFromStateData(hWVTStateData);
    if (pProvData != NULL)
    {
        int idxSigner = 0;
        CRYPT_PROVIDER_SGNR* pCPSigner = WTHelperGetProvSignerFromChain(pProvData, idxSigner, FALSE, 0);
        if (pCPSigner != NULL)
        {
            PCCERT_CONTEXT pCertificate = CertDuplicateCertificateContext(pCPSigner->pasCertChain->pCert);
            if (pCertificate != NULL)
            {
                DWORD size = 0;
                CertGetCertificateContextProperty(pCertificate, CERT_HASH_PROP_ID, NULL, &size);
                if (size != 0)
                {
                    BYTE* buff = (BYTE*)malloc(size);
                    if (buff)
                    {
                        if (CertGetCertificateContextProperty(pCertificate, CERT_HASH_PROP_ID, buff, &size))
                        {
                            for (DWORD i = 0; i < size; ++i)
                            {
                                swprintf(&pSignResult->HashFinalCert[i * 2], 3, L"%02X", buff[i]);
                            }
                        }
                        free(buff);
                    }
                }

                wchar_t* subjectName = ExtractStringFromCertificate(pCertificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0);
                if (subjectName)
                {
                    wcscpy(pSignResult->SubjectName, subjectName);
                    free(subjectName);
                }

                CertFreeCertificateContext(pCertificate);
            }
        }
    }
}

BOOL FileExists(LPCWSTR file)
{
    DWORD attr = GetFileAttributesW(file);
    return attr != INVALID_FILE_ATTRIBUTES && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

__declspec(noinline) BOOL VerifyCustomSignature(LPCWSTR lpFileName, SignResult* pSignResult)
{
    if (FileExists(lpFileName) == FALSE)
    {
        return FALSE;
    }

    BOOL bRet = FALSE;
    BOOL bIsVerified = FALSE;
    WINTRUST_DATA wd = { 0 };
    WINTRUST_FILE_INFO wfi = { 0 };
    WINTRUST_CATALOG_INFO wci = { 0 };
    CATALOG_INFO catalogInfo = { 0 };
    WCHAR pszMemberTag[260] = { 0 };
    HCATINFO hCatInfoContext = NULL;
    LONG iResult = 0;
    DRIVER_VER_INFO verInfo = { 0 };
    WINTRUST_SIGNATURE_SETTINGS signSettings = { 0 };
    HCATADMIN hCatAdmin = NULL;

    HANDLE hFile = NULL;

    IO_STATUS_BLOCK ioStatusBlock;

    UNICODE_STRING fileName;
    RtlInitUnicodeString(&fileName, lpFileName);

    OBJECT_ATTRIBUTES objectAttributes;
    InitializeObjectAttributes(&objectAttributes, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    ULONG desiredAccess = FILE_READ_ATTRIBUTES | FILE_READ_DATA | STANDARD_RIGHTS_READ;
    ULONG shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    ULONG createDisposition = FILE_OPEN;
    ULONG options = FILE_NON_DIRECTORY_FILE;

    NTSTATUS status = OriginalNtCreateFile(&hFile, desiredAccess, &objectAttributes, &ioStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, shareAccess, createDisposition, options, NULL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    GUID DriverGuid = DRIVER_ACTION_VERIFY;
    GUID VerifyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    CryptCATAdminAcquireContext2(&hCatAdmin, &DriverGuid, BCRYPT_SHA256_ALGORITHM, NULL, 0);

    if (hCatAdmin == NULL)
    {
        if (!CryptCATAdminAcquireContext(&hCatAdmin, &DriverGuid, 0))
        {
            CloseHandle(hFile);
            return FALSE;
        }
    }

    DWORD dwHashSize = 0;
    BYTE* fileHash = NULL;
    bRet = CryptCATAdminCalcHashFromFileHandle2(hCatAdmin, hFile, &dwHashSize, NULL, 0);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        fileHash = (BYTE*)malloc(dwHashSize);
        if (fileHash)
        {
            bRet = CryptCATAdminCalcHashFromFileHandle2(hCatAdmin, hFile, &dwHashSize, fileHash, 0);
        }
    }

    if (!bRet || dwHashSize == 0)
    {
        bRet = CryptCATAdminCalcHashFromFileHandle(hFile, &dwHashSize, NULL, 0);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            fileHash = (BYTE*)malloc(dwHashSize);
            if (fileHash)
            {
                bRet = CryptCATAdminCalcHashFromFileHandle(hFile, &dwHashSize, fileHash, 0);
            }
        }

        if (!bRet)
        {
            goto Finalize;
        }
    }

    hCatInfoContext = CryptCATAdminEnumCatalogFromHash(hCatAdmin, fileHash, dwHashSize, 0, NULL);

    if (hCatInfoContext != NULL)
    {
        catalogInfo.cbStruct = sizeof(CATALOG_INFO);
        if (!CryptCATCatalogInfoFromContext(hCatInfoContext, &catalogInfo, 0))
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfoContext, 0);
            hCatInfoContext = NULL;
        }
    }

    wd.cbStruct = sizeof(WINTRUST_DATA);
    wd.dwUIChoice = WTD_UI_NONE;
    wd.dwStateAction = WTD_STATEACTION_VERIFY;
    wd.fdwRevocationChecks = WTD_REVOKE_NONE;
    wd.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
    wd.hWVTStateData = NULL;
    wd.pwszURLReference = NULL;

    if (hCatInfoContext != NULL)
    {
        verInfo.cbStruct = sizeof(DRIVER_VER_INFO);
        wd.pPolicyCallbackData = &verInfo;
        wd.dwUnionChoice = WTD_CHOICE_CATALOG;
        wd.pCatalog = &wci;
        wd.dwUIContext = WTD_UICONTEXT_EXECUTE;
        wci.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
        wci.pcwszCatalogFilePath = catalogInfo.wszCatalogFile;
        wci.pcwszMemberTag = pszMemberTag;
        wci.pcwszMemberFilePath = lpFileName;
        wci.hMemberFile = hFile;
        wci.pbCalculatedFileHash = fileHash;
        wci.cbCalculatedFileHash = dwHashSize;
        wci.hCatAdmin = hCatAdmin;
    }
    else
    {
        wd.dwUnionChoice = WTD_CHOICE_FILE;
        wd.pFile = &wfi;

        wfi.cbStruct = sizeof(WINTRUST_FILE_INFO);
        wfi.pcwszFilePath = NULL;
        wfi.hFile = hFile;
        wfi.pgKnownSubject = NULL;

        signSettings.cbStruct = sizeof(signSettings);
        signSettings.pCryptoPolicy = NULL;
        signSettings.dwFlags = WSS_GET_SECONDARY_SIG_COUNT;
        wd.pSignatureSettings = &signSettings;
    }

    iResult = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &VerifyGuid, &wd);
    bIsVerified = TRUE;
    if (pSignResult != NULL)
    {
        if (iResult == 0 || iResult == CERT_E_UNTRUSTEDROOT || iResult == CERT_E_EXPIRED || iResult == TRUST_E_BAD_DIGEST)
        {
            GetSignerInfo(wd.hWVTStateData, pSignResult);
        }
    }

Finalize:

    if (hCatAdmin && hCatInfoContext)
    {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfoContext, 0);
    }

    if (bIsVerified)
    {
        wd.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &VerifyGuid, &wd);
    }

    if (hCatAdmin)
    {
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    CloseHandle(hFile);

    if (fileHash)
    {
        free(fileHash);
    }

    return iResult == 0 || iResult == CERT_E_UNTRUSTEDROOT || iResult == CERT_E_EXPIRED || iResult == TRUST_E_BAD_DIGEST;
}
