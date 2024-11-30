#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef BOOL(WINAPI* HWriteFile)(_In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped);

HWriteFile targetaddress = nullptr;

BOOL HookWriteFile(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
) {
    char str[] = "Xhani!!!";
    size_t len = strlen(str) + nNumberOfBytesToWrite;
    char* buffer = (char*)malloc(len + 1);
    if (buffer == NULL) return FALSE;

    memcpy(buffer, lpBuffer, nNumberOfBytesToWrite);
    memcpy(buffer + nNumberOfBytesToWrite, str, strlen(str) + 1); // 修改为+1，正确拼接

    BOOL result = targetaddress(hFile, buffer, len, lpNumberOfBytesWritten, lpOverlapped);
    free(buffer); // 释放内存
    return result;
}

BOOL iathookWriteFile(const char* dllname, const char* function, void* hookaddress) {
    HMODULE hmodule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER pDosheader = (PIMAGE_DOS_HEADER)hmodule;
    PIMAGE_NT_HEADERS64 pNtheader = (PIMAGE_NT_HEADERS64)(pDosheader->e_lfanew + (BYTE*)hmodule);
    PIMAGE_OPTIONAL_HEADER64 pOptionhead = &pNtheader->OptionalHeader;
    IMAGE_DATA_DIRECTORY directory = pOptionhead->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hmodule + directory.VirtualAddress);

    while (importDescriptor->Name) {
        const char* iatdllname = (const char*)((BYTE*)hmodule + importDescriptor->Name);
        if (_stricmp(iatdllname, dllname) == 0) {
            PIMAGE_THUNK_DATA64 pInt = (PIMAGE_THUNK_DATA64)((BYTE*)hmodule + importDescriptor->OriginalFirstThunk);
            PIMAGE_THUNK_DATA64 pIat = (PIMAGE_THUNK_DATA64)((BYTE*)hmodule + importDescriptor->FirstThunk);
            while (pInt->u1.AddressOfData) {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hmodule + pInt->u1.AddressOfData);
                if (_stricmp((const char*)pImportByName->Name, function) == 0) {
                    // 保存原始地址
                    targetaddress = (HWriteFile)pIat->u1.Function;

                    DWORD oldProtect;
                    VirtualProtect(&pIat->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
                    pIat->u1.Function = (DWORD_PTR)hookaddress;
                    VirtualProtect(&pIat->u1.Function, sizeof(void*), oldProtect, &oldProtect);

                    return true;
                }
                pInt++;
                pIat++;
            }
            importDescriptor++;
        }
        return FALSE;
    }

}


int main() {
    HANDLE hFile = ::CreateFile("exp.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to create file.\n");
        return 1; // 处理错误
    }

    char str[] = "hello_world";
    DWORD byteswriten;
    if (iathookWriteFile("KERNEL32.dll", "WriteFile", HookWriteFile)) {
        WriteFile(hFile, str, strlen(str), &byteswriten, NULL);
    }
    else {
        printf("Failed to hook WriteFile.\n");
    }
    getchar();
    return 0;
}
