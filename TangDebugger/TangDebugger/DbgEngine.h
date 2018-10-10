#pragma once
#include "DbgObject.h"
#include "BreakpointEngine.h"
class DbgUi;

//DbgEngine ����ʱ�ı�DbgObject�е�m_hThread��m_hProcess��ֵ.
//��ͬ���쳣�������ɲ�ͬ���̺߳ͽ���������.
typedef enum
{
	e_s_sucess = 0,
	e_s_processQuit,
}E_Status;

class DbgEngine : public BreakpointEngine
{

	bool	m_bStopOnSystemBreakpoint; // ��������������,�Ƿ����ϵͳ�ϵ㴦
public:
	DbgEngine();
	~DbgEngine();
public:
	HANDLE m_hBreakpointEvent;
	HANDLE m_hUserIputEvent;
public:
	// ��������
	E_Status Exec();
	void	 Close();

	BOOL	WaitForBreakpointEvent(DWORD nTime);
	void	FinishBreakpointEvnet();

};

