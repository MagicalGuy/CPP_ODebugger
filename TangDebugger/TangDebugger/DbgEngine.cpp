#include "stdafx.h"
#include "DbgEngine.h"
#include "TangDebugger.h"
#include "iostream"
using namespace std;

DbgEngine::DbgEngine()
{
	m_hBreakpointEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hUserIputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

DbgEngine::~DbgEngine()
{
}


//加载模块
BOOL OnDllLoaded(const LOAD_DLL_DEBUG_INFO* pInfo) {

	//加载模块的调试信息
	DWORD64 moduleAddress = SymLoadModule64(
		Hprocess,
		pInfo->hFile,
		NULL,
		NULL,
		(DWORD64)pInfo->lpBaseOfDll,
		0);

	if (moduleAddress == 0) {

		std::wcout << TEXT("SymLoadModule64 failed: ") << GetLastError() << std::endl;
	}

	CloseHandle(pInfo->hFile);

	return TRUE;
}




E_Status DbgEngine::Exec()
{
	static bool bIsSystemBreakpoint = true;
	DEBUG_EVENT dbgEvent = { 0 };
	DWORD dwStatus = 0;
	DWORD bRet = 0;

	// 等待调试事件
	bRet = WaitForDebugEvent(&dbgEvent, 30);
	if (bRet == FALSE)
		return e_s_sucess;

	m_pid = dbgEvent.dwProcessId;
	m_tid = dbgEvent.dwThreadId;

	// 关闭旧的句柄
	CloseHandle(m_hCurrProcess);
	CloseHandle(m_hCurrThread);
	m_hCurrThread = m_hCurrProcess = 0;
	// 获取本轮异常使用到的句柄
	m_hCurrProcess = OpenProcess(PROCESS_ALL_ACCESS,/*所有权限*/
		FALSE,
		m_pid
	);

	//保存进程句柄
	Hprocess = m_hCurrProcess;


	m_hCurrThread = OpenThread(THREAD_ALL_ACCESS,
		FALSE,
		m_tid
	);

	Hthread = m_hCurrThread;

	//OnDllLoaded(&dbgEvent.u.LoadDll);

	//测试
	if (m_hCurrThread)
	{
		context1.ContextFlags = CONTEXT_ALL;
		GetThreadContext(Hprocess, &context1);
		//CloseHandle(Hprocess);
	}



	//线程ID
	threadId = m_tid;



	dwStatus = DBG_CONTINUE;

	// 分析调试事件
	switch (dbgEvent.dwDebugEventCode)
	{
	case EXCEPTION_DEBUG_EVENT: /*异常调试事件*/
	{
		if (bIsSystemBreakpoint)
		{
			// 初始化符号服务器
			BreakpointEngine::InitSymbol(m_hCurrProcess);

			CREATE_PROCESS_DEBUG_INFO* pInfo = &dbgEvent.u.CreateProcessInfo;


			//初始化符号处理器方式二
			if (SymInitialize(Hprocess, NULL, FALSE) == TRUE) {

				//加载模块的调试信息
				DWORD64 moduleAddress = SymLoadModule64(
					Hprocess,
					pInfo->hFile,
					NULL,
					NULL,
					(DWORD64)pInfo->lpBaseOfImage,
					0);

				if (moduleAddress != 0) {

					//SetBreakPointAtEntryPoint();
				}
				else {
					std::wcout << TEXT("SymLoadModule64 failed: ") << GetLastError() << std::endl;
				}
			}
			else {

				std::wcout << TEXT("SymInitialize failed: ") << GetLastError() << std::endl;
			}

			//		CloseHandle(pInfo->hFile);
			//		CloseHandle(pInfo->hThread);
			//		CloseHandle(pInfo->hProcess);


			bIsSystemBreakpoint = false;

			// 在OEP处下断
			if (m_oep != 0) {

				BPObject* pBp = AddBreakPoint(m_oep, breakpointType_soft);
				if (pBp == nullptr)
				{
					TerminateProcess(m_hCurrProcess, 0);
					return e_s_processQuit;
				}
				pBp->SetCondition(true);

			}


			if (m_bStopOnSystemBreakpoint)
			{
				//调用处理断点的回调函数
				SetEvent(m_hBreakpointEvent);
				// 等待用户输入完成
				WaitForSingleObject(m_hUserIputEvent, -1);
				dwStatus = DBG_CONTINUE;
			}
			goto _SUCESS;
		}

		// 重新安装已失效的断点
		ReInstallBreakpoint();

		// 根据异常信息查找断点

		BpItr itr = FindBreakpoint(dbgEvent.u.Exception);
		// 判断迭代器是否有效, 如果无效,说明没有对应的断点
		if (IsInvalidIterator(itr))
		{
			printf("\n>>>>>>> 目标进程自身触发了异常 <<<<<<\n");
			// 设置断点事件为有信号状态
			SetEvent(m_hBreakpointEvent);
			// 等待用户输入完成的信号
			WaitForSingleObject(m_hUserIputEvent, -1);
			dwStatus = DBG_EXCEPTION_NOT_HANDLED;
		}
		else
		{
			// 修复异常,如果能够成功修正断点, 则调用用户的处理函数
			if (true == FixException(itr))
			{
				// 设置断点事件为有信号状态
				SetEvent(m_hBreakpointEvent);
				// 等待用户输入完成的信号
				WaitForSingleObject(m_hUserIputEvent, -1);
			}
		}
		break;
	}




	//创建进程事件
	case CREATE_PROCESS_DEBUG_EVENT:
		// 保存oep和加载基址
		m_oep = (uaddr)dbgEvent.u.CreateProcessInfo.lpStartAddress;
		m_imgBase = (uaddr)dbgEvent.u.CreateProcessInfo.lpBaseOfImage;
		imgBase = m_imgBase;
		break;
		//创建线程事件
	case CREATE_THREAD_DEBUG_EVENT:
		break;
		//进程退出事件
	case EXIT_PROCESS_DEBUG_EVENT:
		bIsSystemBreakpoint = true;
		return e_s_processQuit;
		break;
		//线程退出事件
	case EXIT_THREAD_DEBUG_EVENT:
		break;
		//装载一个dll模块
	case LOAD_DLL_DEBUG_EVENT:
		break;
		//卸载一个对模块
	case UNLOAD_DLL_DEBUG_EVENT:
		break;
		//被调试进程调用outputdebugstring的之类的函数
	case OUTPUT_DEBUG_STRING_EVENT:
		break;
	}


_SUCESS:
	ContinueDebugEvent(dbgEvent.dwProcessId,
		dbgEvent.dwThreadId,
		dwStatus);

	return e_s_sucess;
}


void DbgEngine::Close()
{
	DbgObject::Close();
	BreakpointEngine::Clear();
}


BOOL DbgEngine::WaitForBreakpointEvent(DWORD nTime)
{
	// 等待断点事件的信号
	return WaitForSingleObject(m_hBreakpointEvent, nTime) == WAIT_OBJECT_0;
}

void DbgEngine::FinishBreakpointEvnet()
{
	// 设置用户输入事件完成信号
	SetEvent(m_hUserIputEvent);
}
