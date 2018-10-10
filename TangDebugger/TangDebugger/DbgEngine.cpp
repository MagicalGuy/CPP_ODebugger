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


//����ģ��
BOOL OnDllLoaded(const LOAD_DLL_DEBUG_INFO* pInfo) {

	//����ģ��ĵ�����Ϣ
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

	// �ȴ������¼�
	bRet = WaitForDebugEvent(&dbgEvent, 30);
	if (bRet == FALSE)
		return e_s_sucess;

	m_pid = dbgEvent.dwProcessId;
	m_tid = dbgEvent.dwThreadId;

	// �رվɵľ��
	CloseHandle(m_hCurrProcess);
	CloseHandle(m_hCurrThread);
	m_hCurrThread = m_hCurrProcess = 0;
	// ��ȡ�����쳣ʹ�õ��ľ��
	m_hCurrProcess = OpenProcess(PROCESS_ALL_ACCESS,/*����Ȩ��*/
		FALSE,
		m_pid
	);

	//������̾��
	Hprocess = m_hCurrProcess;


	m_hCurrThread = OpenThread(THREAD_ALL_ACCESS,
		FALSE,
		m_tid
	);

	Hthread = m_hCurrThread;

	//OnDllLoaded(&dbgEvent.u.LoadDll);

	//����
	if (m_hCurrThread)
	{
		context1.ContextFlags = CONTEXT_ALL;
		GetThreadContext(Hprocess, &context1);
		//CloseHandle(Hprocess);
	}



	//�߳�ID
	threadId = m_tid;



	dwStatus = DBG_CONTINUE;

	// ���������¼�
	switch (dbgEvent.dwDebugEventCode)
	{
	case EXCEPTION_DEBUG_EVENT: /*�쳣�����¼�*/
	{
		if (bIsSystemBreakpoint)
		{
			// ��ʼ�����ŷ�����
			BreakpointEngine::InitSymbol(m_hCurrProcess);

			CREATE_PROCESS_DEBUG_INFO* pInfo = &dbgEvent.u.CreateProcessInfo;


			//��ʼ�����Ŵ�������ʽ��
			if (SymInitialize(Hprocess, NULL, FALSE) == TRUE) {

				//����ģ��ĵ�����Ϣ
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

			// ��OEP���¶�
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
				//���ô���ϵ�Ļص�����
				SetEvent(m_hBreakpointEvent);
				// �ȴ��û��������
				WaitForSingleObject(m_hUserIputEvent, -1);
				dwStatus = DBG_CONTINUE;
			}
			goto _SUCESS;
		}

		// ���°�װ��ʧЧ�Ķϵ�
		ReInstallBreakpoint();

		// �����쳣��Ϣ���Ҷϵ�

		BpItr itr = FindBreakpoint(dbgEvent.u.Exception);
		// �жϵ������Ƿ���Ч, �����Ч,˵��û�ж�Ӧ�Ķϵ�
		if (IsInvalidIterator(itr))
		{
			printf("\n>>>>>>> Ŀ��������������쳣 <<<<<<\n");
			// ���öϵ��¼�Ϊ���ź�״̬
			SetEvent(m_hBreakpointEvent);
			// �ȴ��û�������ɵ��ź�
			WaitForSingleObject(m_hUserIputEvent, -1);
			dwStatus = DBG_EXCEPTION_NOT_HANDLED;
		}
		else
		{
			// �޸��쳣,����ܹ��ɹ������ϵ�, ������û��Ĵ�����
			if (true == FixException(itr))
			{
				// ���öϵ��¼�Ϊ���ź�״̬
				SetEvent(m_hBreakpointEvent);
				// �ȴ��û�������ɵ��ź�
				WaitForSingleObject(m_hUserIputEvent, -1);
			}
		}
		break;
	}




	//���������¼�
	case CREATE_PROCESS_DEBUG_EVENT:
		// ����oep�ͼ��ػ�ַ
		m_oep = (uaddr)dbgEvent.u.CreateProcessInfo.lpStartAddress;
		m_imgBase = (uaddr)dbgEvent.u.CreateProcessInfo.lpBaseOfImage;
		imgBase = m_imgBase;
		break;
		//�����߳��¼�
	case CREATE_THREAD_DEBUG_EVENT:
		break;
		//�����˳��¼�
	case EXIT_PROCESS_DEBUG_EVENT:
		bIsSystemBreakpoint = true;
		return e_s_processQuit;
		break;
		//�߳��˳��¼�
	case EXIT_THREAD_DEBUG_EVENT:
		break;
		//װ��һ��dllģ��
	case LOAD_DLL_DEBUG_EVENT:
		break;
		//ж��һ����ģ��
	case UNLOAD_DLL_DEBUG_EVENT:
		break;
		//�����Խ��̵���outputdebugstring��֮��ĺ���
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
	// �ȴ��ϵ��¼����ź�
	return WaitForSingleObject(m_hBreakpointEvent, nTime) == WAIT_OBJECT_0;
}

void DbgEngine::FinishBreakpointEvnet()
{
	// �����û������¼�����ź�
	SetEvent(m_hUserIputEvent);
}
