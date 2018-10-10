#pragma once
#include "DbgObject.h"
#include "BreakpointEngine.h"
class DbgUi;

//DbgEngine 会随时改变DbgObject中的m_hThread和m_hProcess的值.
//不同的异常可能是由不同的线程和进程引发的.
typedef enum
{
	e_s_sucess = 0,
	e_s_processQuit,
}E_Status;

class DbgEngine : public BreakpointEngine
{

	bool	m_bStopOnSystemBreakpoint; // 调试器引擎配置,是否断在系统断点处
public:
	DbgEngine();
	~DbgEngine();
public:
	HANDLE m_hBreakpointEvent;
	HANDLE m_hUserIputEvent;
public:
	// 运行引擎
	E_Status Exec();
	void	 Close();

	BOOL	WaitForBreakpointEvent(DWORD nTime);
	void	FinishBreakpointEvnet();

};

