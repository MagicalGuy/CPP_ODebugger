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
	PROCESS_INFORMATION  stcProcInfo = { 0 };     // ������Ϣ

												  /* �������Խ��̳� */
	BOOL bRet = FALSE;
	bRet = CreateProcessA(pszFile,                // ��ִ��ģ��·��
		NULL,                   // ������
		NULL,                   // ��ȫ������
		NULL,                   // �߳������Ƿ�ɼ̳�
		FALSE,                  // �Ƿ�ӵ��ý��̴��̳��˾��
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,// �Ե��Եķ�ʽ����
		NULL,                   // �½��̵Ļ�����
		NULL,                   // �½��̵ĵ�ǰ����·������ǰĿ¼��
		&stcStartupInfo,        // ָ�����̵�����������
		&stcProcInfo             // �����½��̵���Ϣ
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



//��ȡ��Ȩ
bool getSeDebugPrivilge() {
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;

	//��ȡSEDEBUG��Ȩ��LUID
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	return true;
}


//���ӽ���
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

//��ȡ�ڴ�
uint DbgObject::ReadMemory(uaddr  uAddress, pbyte pBuff, uint uSize)
{
	DWORD	dwRead = 0;
	ReadProcessMemory(m_hCurrProcess, (LPVOID)uAddress, pBuff, uSize, &dwRead);
	return dwRead;
}

//д���ڴ�
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
	// ö�ٽ���ģ��
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
	// ѭ����ȡģ����Ϣ
	for (SIZE_T i = 0; i < dwModuleCount; ++i)
	{
		// ��ȡģ��·��
		GetModuleFileNameExA(m_hCurrProcess,
			phModule[i],
			path,
			MAX_PATH);

		// ��ȡģ��������Ϣ
		GetModuleInformation(m_hCurrProcess,
			phModule[i],
			&moif, sizeof(MODULEINFO));
		itr->name = PathFindFileNameA(path);
		itr->uStart = (LONG64)moif.lpBaseOfDll; // dll ��ַ
		itr->uSize = moif.SizeOfImage; // dll ��С
		++itr;
	}

	delete[] phModule;


	//HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_pid);
	//if (hSnapShot == INVALID_HANDLE_VALUE)
	//{
	//	//MessageBox(L"�򿪽�����Ϣʧ�ܣ�");
	//	return;
	//}
	//MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
	//if (!Module32First(hSnapShot, &me))
	//{
	//	//MessageBox(L"��ȡģ����Ϣʧ�ܣ�");
	//	return;
	//}
	//printf("ģ������      ģ���С      ģ��·��\n");
	//do
	//{
	//	TCHAR* SizeBuf = new TCHAR[11];
	//	_stprintf_s(SizeBuf, 10, L"%d", me.modBaseSize);
	//	printf("%ws   %d    %ws", me.szModule, SizeBuf, me.szExePath);
	//} while (Module32Next(hSnapShot, &me));
	//return;
}
