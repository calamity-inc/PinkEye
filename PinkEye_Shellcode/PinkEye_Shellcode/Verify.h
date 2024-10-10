#include <Windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
#include <sfc.h>
#include <wchar.h>
#include <intrin.h>

typedef LONG(WINAPI* typedef_WinVerifyTrust)(HWND hwnd, GUID* pgActionID, LPVOID pWVTData);
static typedef_WinVerifyTrust OriginalWinVerifyTrust;

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