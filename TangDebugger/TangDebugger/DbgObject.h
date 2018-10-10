#pragma once

#include <windows.h>
#include <list>
using std::list;
#include <map>
using std::map;
using std::pair;
#include <atlstr.h>

typedef unsigned int	 uint;
typedef unsigned int     uaddr;
typedef unsigned char	 byte, *pbyte;

typedef struct MODULEFULLINFO
{
	CStringA name;
	LONG64	 uStart;
	LONG32	 uSize;
}MODULEFULLINFO, *PMODULEFULLINFO;

//���Զ���
class DbgObject
{

public:
	DbgObject();
	~DbgObject();

public:
	uaddr			m_imgBase;		// ���ػ�ַ
	uint			m_oep;			// ������ڵ�ַ
	uint			m_pid;			// ����id
	uint			m_tid;			// �߳�id
	HANDLE			m_hCurrProcess;	// ��ǰ�����쳣�Ľ��̾��
	HANDLE			m_hCurrThread;	// ��ǰ�����쳣���߳̾��
	list<HANDLE>	m_hProcessList;	// ���̾����
	list<HANDLE>	m_hThreadList;	// �߳̾����
public:
	bool Open(const char* pszFile);
	bool Open(const uint  uPid);
	bool IsOpen();
	bool IsClose();
	void Close();

	// ��ȡ�ڴ�
	uint ReadMemory(uaddr  uAddress, pbyte pBuff, uint uSize);
	// д���ڴ�
	uint WriteMemory(uaddr uAddress, const pbyte pBuff, uint uSize);

	// ��ȡ�Ĵ�����Ϣ
	bool GetRegInfo(CONTEXT& ct);
	// ���üĴ�����Ϣ
	bool SetRegInfo(CONTEXT& ct);

	// ����µ��߳�
	void AddThread(HANDLE hThread);
	// ����µĽ���
	void AddProcess(HANDLE hProcess, HANDLE hThread);
	// ���߳��б�ɾ��һ���߳�
	bool RemoveThread(HANDLE hThread);
	// �ӽ����б�ɾ��һ������
	bool RemoveProcess(HANDLE hProcess);

	// ��ȡģ���б�
	void GetModuleList(list<MODULEFULLINFO>& moduleList);
};

