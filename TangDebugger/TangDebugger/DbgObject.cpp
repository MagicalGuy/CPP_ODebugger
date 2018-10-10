#include "stdafx.h"
#include "DbgObject.h"
#include <algorithm>
#include "Pectrl.h"
#include <psapi.h>
#include <TlHelp32.h>
#include"TangDebugger.h"
#include "HidePEB.h"

DbgObject::DbgObject()
	:m_imgBase()
	, m_oep()
	, m_pid()
	, m_tid()
	, m_hCurrProcess()
	, m_hCurrThread()
{
}


DbgObject::~DbgObject()
{
}

bool DbgObject::Open(const char* pszFile)
{
	if (pszFile == nullptr)
		return false;
	STARTUPINFOA         stcStartupInfo = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION  stcProcInfo = { 0 };     // 进程信息

												  /* 创建调试进程程 */
	BOOL bRet = FALSE;
	bRet = CreateProcessA(pszFile,                // 可执行模块路径
		NULL,                   // 命令行
		NULL,                   // 安全描述符
		NULL,                   // 线程属性是否可继承
		FALSE,                  // 是否从调用进程处继承了句柄
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,// 以调试的方式启动
		NULL,                   // 新进程的环境块
		NULL,                   // 新进程的当前工作路径（当前目录）
		&stcStartupInfo,        // 指定进程的主窗口特性
		&stcProcInfo             // 接收新进程的信息
	);
	if (bRet == FALSE)
		return false;


	//HidePEBDebug(stcProcInfo.hProcess);

	m_hCurrProcess = stcProcInfo.hProcess;
	Hprocess = stcProcInfo.hProcess;

	m_hCurrThread = stcProcInfo.hThread;
	Hthread = stcProcInfo.hThread;

	m_pid = stcProcInfo.dwProcessId;
	m_tid = stcProcInfo.dwThreadId;


	threadId = stcProcInfo.dwThreadId;


	return true;
}



//获取特权
bool getSeDebugPrivilge() {
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;

	//获取SEDEBUG特权的LUID
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	return true;
}


//附加进程
bool DbgObject::Open(const uint uPid){
	DebugSetProcessKillOnExit(FALSE);
	//getSeDebugPrivilge();
	BOOL bRet = DebugActiveProcess(uPid);

	//HANDLE	hProcess1 = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, uPid);
	//TCHAR	pszProcessPath[MAX_PATH + 2];
	//GetModuleFileNameEx(hProcess1, NULL, pszProcessPath, MAX_PATH + 2);

	//file_Name = pszProcessPath;

	//TerminateProcess(hProcess1,0);
	return bRet;
	//return true;
}

bool DbgObject::IsOpen()
{
	return m_hCurrProcess != 0;
}

void DbgObject::Close()
{
	TerminateProcess(m_hCurrProcess, 0);
	m_hCurrProcess = 0;
}


bool DbgObject::IsClose()
{
	return m_hCurrProcess == 0;
}

//读取内存
uint DbgObject::ReadMemory(uaddr  uAddress, pbyte pBuff, uint uSize)
{
	DWORD	dwRead = 0;
	ReadProcessMemory(m_hCurrProcess, (LPVOID)uAddress, pBuff, uSize, &dwRead);
	return dwRead;
}

//写入内存
uint DbgObject::WriteMemory(uaddr uAddress, const pbyte pBuff, uint uSize)
{
	DWORD	dwWrite = 0;
	WriteProcessMemory(m_hCurrProcess, (LPVOID)uAddress, pBuff, uSize, &dwWrite);
	return dwWrite;
}

bool DbgObject::GetRegInfo(CONTEXT& ct)
{
	return GetThreadContext(m_hCurrThread, &ct) == TRUE;
}

bool DbgObject::SetRegInfo(CONTEXT& ct)
{
	return SetThreadContext(m_hCurrThread, &ct) == TRUE;
}

void DbgObject::AddThread(HANDLE hThread)
{
	list<HANDLE>::iterator i = m_hThreadList.begin();
	for (; i != m_hThreadList.end(); ++i)
	{
		if (*i == hThread)
			return;
	}
	m_hThreadList.push_back(hThread);
}

void DbgObject::AddProcess(HANDLE hProcess, HANDLE hThread)
{
	AddThread(hThread);
	list<HANDLE>::iterator i = m_hProcessList.begin();
	for (; i != m_hProcessList.end(); ++i)
	{
		if (*i == hProcess)
			return;
	}
	m_hProcessList.push_back(hProcess);
}

bool DbgObject::RemoveThread(HANDLE hThread)
{
	list<HANDLE>::iterator i = m_hThreadList.begin();
	for (; i != m_hThreadList.end(); ++i)
	{
		if (*i == hThread)
		{
			m_hThreadList.erase(i);
			return true;
		}
	}
	return false;
}

bool DbgObject::RemoveProcess(HANDLE hProcess)
{
	list<HANDLE>::iterator i = m_hProcessList.begin();
	for (; i != m_hProcessList.end(); ++i)
	{
		if (*i == hProcess)
		{
			m_hProcessList.erase(i);
			return true;
		}
	}
	return false;
}



void DbgObject::GetModuleList(list<MODULEFULLINFO>& moduleList)
{
	// 枚举进程模块
	DWORD dwNeed = 0;
	EnumProcessModulesEx(m_hCurrProcess,
		nullptr,
		0,
		&dwNeed,
		LIST_MODULES_ALL);
	DWORD	dwModuleCount = dwNeed / sizeof(HMODULE);
	HMODULE *phModule = new HMODULE[dwModuleCount];
	EnumProcessModulesEx(m_hCurrProcess,
		phModule,
		dwNeed,
		&dwNeed,
		LIST_MODULES_ALL);

	MODULEINFO		moif = { 0 };
	char path[MAX_PATH];
	moduleList.resize(dwModuleCount);
	list<MODULEFULLINFO>::iterator itr = moduleList.begin();
	// 循环获取模块信息
	for (SIZE_T i = 0; i < dwModuleCount; ++i)
	{
		// 获取模块路径
		GetModuleFileNameExA(m_hCurrProcess,
			phModule[i],
			path,
			MAX_PATH);

		// 获取模块其他信息
		GetModuleInformation(m_hCurrProcess,
			phModule[i],
			&moif, sizeof(MODULEINFO));
		itr->name = PathFindFileNameA(path);
		itr->uStart = (LONG64)moif.lpBaseOfDll; // dll 基址
		itr->uSize = moif.SizeOfImage; // dll 大小
		++itr;
	}

	delete[] phModule;


	//HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_pid);
	//if (hSnapShot == INVALID_HANDLE_VALUE)
	//{
	//	//MessageBox(L"打开进程信息失败！");
	//	return;
	//}
	//MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
	//if (!Module32First(hSnapShot, &me))
	//{
	//	//MessageBox(L"获取模块信息失败！");
	//	return;
	//}
	//printf("模块名称      模块大小      模块路径\n");
	//do
	//{
	//	TCHAR* SizeBuf = new TCHAR[11];
	//	_stprintf_s(SizeBuf, 10, L"%d", me.modBaseSize);
	//	printf("%ws   %d    %ws", me.szModule, SizeBuf, me.szExePath);
	//} while (Module32Next(hSnapShot, &me));
	//return;
}
