#include <Windows.h>
#include <iostream>

typedef int (WINAPI* MessageBoxW_t)(HWND, LPCWSTR, LPCWSTR, UINT);
MessageBoxW_t OriginalMessageBoxW = nullptr;

int WINAPI HookMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    return OriginalMessageBoxW(hWnd, L"IAT HOOK success!", lpCaption, uType);
}

bool InstallIATHook(const char* dllName, const char* funName, void* hookFuncAddr) {
    HMODULE hModule = GetModuleHandleA(NULL);
    
    if (!hModule) return false;

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)((BYTE*)dosHeader + dosHeader->e_lfanew);
    PIMAGE_OPTIONAL_HEADER64 optionalHeader = &ntHeaders->OptionalHeader;
    IMAGE_DATA_DIRECTORY directory = optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + directory.VirtualAddress);

    while (importDescriptor->Name) {
        const char* iatDllName = (const char*)((BYTE*)hModule + importDescriptor->Name);
        if (_stricmp(dllName, iatDllName) == 0) {
            PIMAGE_THUNK_DATA64 pInt = (PIMAGE_THUNK_DATA64)((BYTE*)hModule + importDescriptor->OriginalFirstThunk);
            PIMAGE_THUNK_DATA64 pIat = (PIMAGE_THUNK_DATA64)((BYTE*)hModule + importDescriptor->FirstThunk);

            while (pInt->u1.AddressOfData) {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hModule + pInt->u1.AddressOfData);
                if (strcmp((const char*)pImportByName->Name, funName) == 0) {
                    // 保存原始地址
                    OriginalMessageBoxW = (MessageBoxW_t)pIat->u1.Function;

                    DWORD oldProtect;
                    VirtualProtect(&pIat->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
                    pIat->u1.Function = (DWORD_PTR)hookFuncAddr;
                    VirtualProtect(&pIat->u1.Function, sizeof(void*), oldProtect, &oldProtect);

                    return true;
                }
                pInt++;
                pIat++;
            }
        }
        importDescriptor++;
    }
    return false;
}

int main() {
    InstallIATHook("user32.dll", "MessageBoxW", HookMessageBoxW);
    MessageBoxW(NULL, L"欢迎加入从零开始学逆向！", L"@轩辕的编程宇宙", MB_OK);
    return 0;
}
