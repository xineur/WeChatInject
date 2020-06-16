// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <tchar.h>
#include <TlHelp32.h>
#include <direct.h>
#include <string.h>
#include <atlstr.h>

void CharToTchar(const char * _char, TCHAR * tchar)
{
	int iLength;
	iLength = MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, tchar, iLength);
}

extern "C" __declspec(dllexport) int Inject(char* dllFileName);

int Inject(char* dllFileName)
{
	// 需要被注入的dll
	//TCHAR DllFileName[] = TEXT("");
	//TCHAR DirName[] = TEXT("");
	// 微信id
	DWORD pid = 0;
	//char DllFileName[MAX_PATH] = "E:\\c++\\demoIndejt\\InjectWeChat.dll";

	/*TCHAR DirName[MAX_PATH] = TEXT("");
	//char dllNamePath[256] = "";
	TCHAR DllFileName[MAX_PATH] = TEXT("");
	GetCurrentDirectoryW(sizeof(DirName), DirName);//
	wcscat_s(DllFileName, DirName);
	wcscat_s(DllFileName, TEXT("\\"));
	wcscat_s(DllFileName, TEXT("InjectWeChat.dll"));
	*/
	const char* DllFileName = "aaaa";
	TCHAR a[MAX_PATH] = TEXT("");
	CharToTchar(dllFileName, a);
	MessageBox(NULL, LPCWSTR(dllFileName), TEXT("aaa"), MB_OK);

	return 0;
	// 需要被注入的dll长度
	//DWORD strSize = strlen(DllFileName) + 1;
	DWORD strSize = sizeof(DllFileName) + 1;

	// 1> 遍历进程找到微信进程
	HANDLE wxHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32  processentry32 = { sizeof(processentry32) };
	processentry32.dwSize = sizeof(PROCESSENTRY32);
	BOOL next = Process32Next(wxHandle, &processentry32);
	while (next == TRUE)
	{
		// if (wcscmp(processentry32.szExeFile, L"WeChat.exe") == 0) {
		if (wcscmp(processentry32.szExeFile, L"YoudaoDict.exe") == 0) {
			pid = processentry32.th32ProcessID;
			break;
		}
		next = Process32Next(wxHandle, &processentry32);
	}
	if (pid == 0) {
		return 1;
	}
	// 2> 打开微信进程,获得HANDLE
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	if (hProcess == NULL) {
		return 2;
	}
	// 3> 在微信进程中为DLL文件路径字符串申请内存空间（VirtualAllocEx）
	LPVOID allocAddress = VirtualAllocEx(hProcess, NULL, strSize, MEM_COMMIT, PAGE_READWRITE);
	if (allocAddress == NULL) {
		return 3;
	}
	//4)	把DLL文件路径字符串写入到申请的内存中（WriteProcessMemory）
	BOOL result = WriteProcessMemory(hProcess, allocAddress, LPCVOID(DllFileName), strSize, NULL);
	if (result == FALSE)
	{
		return 4;
	}
	//5)	从Kernel32.dll中获取LoadLibraryA的函数地址（GetModuleHandle、GetProcAddress）
	HMODULE hMODULE = GetModuleHandle(L"Kernel32.dll");
	LPTHREAD_START_ROUTINE fARPROC = (PTHREAD_START_ROUTINE)GetProcAddress(hMODULE, "LoadLibraryA");
	if (NULL == fARPROC)
	{
		return 5;
	}
	//6)	在微信中启动内存中指定了文件名路径的DLL（CreateRemoteThread）。
	//也就是调用DLL中的DllMain（以DLL_PROCESS_ATTACH为参数）。
	HANDLE hANDLE = CreateRemoteThread(hProcess, NULL, 0, fARPROC, allocAddress, 0, NULL);
	if (NULL == hANDLE)
	{
		return 6;
	}
	return 0;
}

/*
错误码
1: 没有找到微信进程
2: 打开微信进程失败
3: 分配内存空间失败
4: 写入内存失败
5: 查找LoadLibraryA失败
6: 启动远程线程失败
*/