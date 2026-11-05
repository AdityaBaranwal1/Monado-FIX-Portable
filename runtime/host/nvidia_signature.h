// [XCDE] T257: authenticate the interposer before loading executable code.
// Uses WinTrust + the primary embedded signer; compatible with Zig's older MinGW headers.
#pragma once
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <stdlib.h>
static bool xcde_nvidia_signature(const wchar_t *path) {
    HMODULE trust = LoadLibraryExW(L"wintrust.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE crypto = LoadLibraryExW(L"crypt32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!trust || !crypto) { if (trust) FreeLibrary(trust); if (crypto) FreeLibrary(crypto); return false; }
    typedef LONG (WINAPI *Verify)(HWND, GUID *, LPVOID);
    auto verify = (Verify)GetProcAddress(trust, "WinVerifyTrust");
    auto query = (decltype(&CryptQueryObject))GetProcAddress(crypto, "CryptQueryObject");
    auto get_param = (decltype(&CryptMsgGetParam))GetProcAddress(crypto, "CryptMsgGetParam");
    auto find_cert = (decltype(&CertFindCertificateInStore))GetProcAddress(crypto, "CertFindCertificateInStore");
    auto get_name = (decltype(&CertGetNameStringW))GetProcAddress(crypto, "CertGetNameStringW");
    auto free_cert = (decltype(&CertFreeCertificateContext))GetProcAddress(crypto, "CertFreeCertificateContext");
    auto close_store = (decltype(&CertCloseStore))GetProcAddress(crypto, "CertCloseStore");
    auto close_msg = (decltype(&CryptMsgClose))GetProcAddress(crypto, "CryptMsgClose");
    bool valid = false;
    if (verify && query && get_param && find_cert && get_name && free_cert && close_store && close_msg) {
        WINTRUST_FILE_INFO file = {}; file.cbStruct = sizeof file; file.pcwszFilePath = path;
        WINTRUST_DATA data = {}; data.cbStruct = sizeof data; data.dwUIChoice = WTD_UI_NONE;
        data.fdwRevocationChecks = WTD_REVOKE_NONE; data.dwUnionChoice = WTD_CHOICE_FILE;
        data.pFile = &file; data.dwStateAction = WTD_STATEACTION_VERIFY;
        GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
        LONG result = verify(NULL, &action, &data);
        if (result == ERROR_SUCCESS) {
            HCERTSTORE store = NULL; HCRYPTMSG message = NULL;
            if (query(CERT_QUERY_OBJECT_FILE, path, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY,
                      0, NULL, NULL, NULL, &store, &message, NULL)) {
                DWORD bytes = 0;
                if (get_param(message, CMSG_SIGNER_INFO_PARAM, 0, NULL, &bytes) && bytes && bytes <= 1024 * 1024) {
                    auto signer = (CMSG_SIGNER_INFO *)malloc(bytes);
                    if (signer && get_param(message, CMSG_SIGNER_INFO_PARAM, 0, signer, &bytes)) {
                        CERT_INFO info = {}; info.Issuer = signer->Issuer; info.SerialNumber = signer->SerialNumber;
                        PCCERT_CONTEXT certificate = find_cert(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, &info, NULL);
                        if (certificate) {
                            wchar_t name[256] = {};
                            if (get_name(certificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, name, 256) > 1)
                                valid = lstrcmpiW(name, L"NVIDIA Corporation") == 0;
                            free_cert(certificate);
                        }
                    }
                    free(signer);
                }
            }
            if (message) close_msg(message);
            if (store) close_store(store, 0);
        }
        data.dwStateAction = WTD_STATEACTION_CLOSE; verify(NULL, &action, &data);
    }
    FreeLibrary(crypto); FreeLibrary(trust); return valid;
}
