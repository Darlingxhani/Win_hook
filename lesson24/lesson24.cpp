// lesson24.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>

int
WINAPI
HookMessageBoxW(
				__in_opt HWND hWnd,
				__in_opt LPCSTR lpText,
				__in_opt LPCSTR lpCaption,
				__in UINT uType)
{
	MessageBoxA(hWnd, "IAT HOOK success!", "IAT HOOK", MB_OK);

	return 0;
}


bool InstallIATHook(const char* dllName, const char* funName, void* hookFuncAddr) {


	// 第一步：定位IAT表项位置

	// 1.1 定位当前PE文件的导入表
	HMODULE hModule = GetModuleHandleA(NULL);

	PIMAGE_DOS_HEADER ptrDosHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS32 ptrNtHeader = (PIMAGE_NT_HEADERS32)((DWORD)hModule + (DWORD)ptrDosHeader->e_lfanew);
	PIMAGE_OPTIONAL_HEADER32 ptrOptionHeader = &ptrNtHeader->OptionalHeader;
	IMAGE_DATA_DIRECTORY directory = ptrOptionHeader->DataDirectory[1];
	PIMAGE_IMPORT_DESCRIPTOR pImportDescripor = PIMAGE_IMPORT_DESCRIPTOR((DWORD)hModule + (DWORD)directory.VirtualAddress);

	while (pImportDescripor->Name) {

		const char* iatDllName = (const char*)((DWORD)hModule + (DWORD)pImportDescripor->Name);
		if (stricmp(dllName, iatDllName) == 0) {

			// 已经定位到对应的导入表描述符了

			PIMAGE_THUNK_DATA pInt = (PIMAGE_THUNK_DATA)((DWORD)hModule + (DWORD)pImportDescripor->OriginalFirstThunk);
			PIMAGE_THUNK_DATA pIat = (PIMAGE_THUNK_DATA)((DWORD)hModule + (DWORD)pImportDescripor->FirstThunk);

			while (pInt->u1.Function) {

				PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD)hModule + (DWORD)pInt->u1.Function);
				if (strcmp((const char*)pImportByName->Name, funName) == 0 ) {


					DWORD* targetFunAddrPtr = (DWORD*)pIat;

					DWORD oldProtect = 0;
					VirtualProtect(targetFunAddrPtr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
					*targetFunAddrPtr = (DWORD)hookFuncAddr;
					VirtualProtect(targetFunAddrPtr, 4, oldProtect, &oldProtect);

					return true;
				}

				pInt++;
				pIat++;
			}

		}


		pImportDescripor++;
	}


	return false;
}



int _tmain(int argc, _TCHAR* argv[])
{
	InstallIATHook("user32.dll", "MessageBoxW", HookMessageBoxW);
	MessageBoxW(NULL, L"欢迎加入从零开始学逆向！", L"@轩辕的编程宇宙", MB_OK);

	return 0;
}

