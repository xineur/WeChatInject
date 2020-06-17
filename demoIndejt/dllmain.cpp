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

wchar_t * char2wchar(const char* cchar)
{
	wchar_t *m_wchar;
	int len = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
	m_wchar = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), m_wchar, len);
	m_wchar[len] = '\0';
	return m_wchar;
}

extern "C" __declspec(dllexport) int Inject(char* dllFileName, char* exeName);

int Inject(char* dllFileName, char* exeName)
{
	// 需要被注入的dll
	char DllFileName[MAX_PATH];
	// 需要被注入的应用名称
	wchar_t ExeName[MAX_PATH];
	// 微信id
	DWORD pid = 0;
	// 获取dll的路径
	strcpy_s(DllFileName, dllFileName);
	// 获取应用名称
	wcscpy_s(ExeName, char2wchar(exeName));
	// 需要被注入的dll长度
	//DWORD strSize = strlen(DllFileName) + 1;
	DWORD strSize = sizeof(DllFileName) + 1;

	// 1> 遍历进程找到指定进程
	HANDLE wxHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32  processentry32 = { sizeof(processentry32) };
	processentry32.dwSize = sizeof(PROCESSENTRY32);
	BOOL next = Process32Next(wxHandle, &processentry32);
	while (next == TRUE)
	{
		// if (wcscmp(processentry32.szExeFile, L"WeChat.exe") == 0) {
		if (wcscmp(processentry32.szExeFile, ExeName) == 0) {
			pid = processentry32.th32ProcessID;
			break;
		}
		next = Process32Next(wxHandle, &processentry32);
	}
	if (pid == 0) {
		return 1;
	}
	// 2> 打开指定进程,获得HANDLE
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	if (hProcess == NULL) {
		return 2;
	}
	// 3> 在指定进程中为DLL文件路径字符串申请内存空间（VirtualAllocEx）
	LPVOID allocAddress = VirtualAllocEx(hProcess, NULL, strSize, MEM_COMMIT, PAGE_READWRITE);
	if (allocAddress == NULL) {
		return 3;
	}
	// 4> 把DLL文件路径字符串写入到申请的内存中（WriteProcessMemory）
	BOOL result = WriteProcessMemory(hProcess, allocAddress, DllFileName, strSize, NULL);
	if (result == FALSE)
	{
		return 4;
	}
	// 5> 从Kernel32.dll中获取LoadLibraryA的函数地址（GetModuleHandle、GetProcAddress）
	HMODULE hMODULE = GetModuleHandle(L"Kernel32.dll");
	LPTHREAD_START_ROUTINE fARPROC = (PTHREAD_START_ROUTINE)GetProcAddress(hMODULE, "LoadLibraryA");
	if (NULL == fARPROC)
	{
		return 5;
	}
	// 6> 在指定应用中启动内存中指定了文件名路径的DLL（CreateRemoteThread）。
	// 也就是调用DLL中的DllMain（以DLL_PROCESS_ATTACH为参数）。
	HANDLE hANDLE = CreateRemoteThread(hProcess, NULL, 0, fARPROC, allocAddress, 0, NULL);
	if (NULL == hANDLE)
	{
		return 6;
	}
	

	// 将注入信息已弹窗的信息通知用户,防止恶意注入
	char SendMg[MAX_PATH];
	strcpy_s(SendMg, "您的");
	strcat_s(SendMg, exeName);
	strcat_s(SendMg, "应用,被");
	strcat_s(SendMg, dllFileName);
	strcat_s(SendMg, "注入");
	TCHAR message[MAX_PATH] = TEXT("");
	CharToTchar(SendMg, message);
	MessageBox(NULL, message, TEXT("注入提示"), MB_OK);
	return 0;
}

/*
错误码
1: 没有找到进程
2: 打开进程失败
3: 分配内存空间失败
4: 写入内存失败
5: 查找LoadLibraryA失败
6: 启动远程线程失败
*/