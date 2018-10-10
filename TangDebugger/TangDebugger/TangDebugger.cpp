// TangDebugger.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"



#include"TangDebugger.h"
#include "DbgEngine.h" // ��������
#include "dbgUi.h" // �û�����
#include "Expression.h" // ���ʽģ��
#include "DisAsmEngine.h" // ���������
#include "XEDParse.h" // �������
#include "BPObject.h"

#include"TlHelp32.h"
#include"Psapi.h"
#include "SoundCode.h"
#include"Pectrl.h"

#include"AddPlugin.h"

#include"DisAsmEngine.h"
#include "AtlBase.h"
#include "AtlConv.h"
#include <atlstr.h>
#include<atlconv.h>

#include"OutHex.h"

#include <iostream>
#include <iomanip>

#include <conio.h>
#include <limits>
using namespace std;

#pragma comment(lib,"XEDParse.lib")


HANDLE Hprocess;
HANDLE Hthread;
unsigned int threadId;

CONTEXT context1;

//���̼��ػ�ַ
uaddr imgBase;

PIMAGE_DOS_HEADER pDosH;	//DOSͷ
char* buff;
CString file_Name;

// ��ʾ�����������а�����Ϣ
void showHelp();
// ���ַ����ָ�������ַ���(����һ�������Ŀո��滻���ַ���������)
char* GetSecondArg(char* pBuff);
inline char* SkipSpace(char* pBuff);
 

// ����������Ķϵ㴦��ص�����
unsigned int __stdcall DbgBreakpointEvent(void* uParam);
unsigned int __stdcall OtherDbgBreakpointEvent(void* uParam);


// ��ȡ�������еĲ���
void GetCmdLineArg(char* pszCmdLine, int nArgCount, ...);
// ���öϵ�
void SetBreakpoint(DbgEngine* pDbg, DbgUi* pUi, char* szCmdLine);
DWORD	g_dwProcessStatus = 0;


typedef std::map<DWORD, std::wstring> ModuleBaseToNameMap;


//��ӡ�����б����ڸ���
void printProcessList();


//�������ö�ջ
bool stack_walk(STACKFRAME64& stack_frame, CONTEXT& context);
bool symbol_from_addr(DWORD addr, std::string& symbol, bool allow_in_func);


//��ʾ���ö�ջ
void OnShowStackTrack();

//��ʱ���õ�ö��ģ��ص�
BOOL CALLBACK EnumerateModuleCallBack(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext);


//��ʾԴ��
void OnShowSourceLines();

//��ȡ�����Խ��̵����̵߳������Ļ�����
BOOL GetDebuggeeContext(CONTEXT* pContext);

//�޸��ڴ�
DWORD CmdModifyData(CString& str1);


//��ȡ��ջ��Ϣ
BOOL GetStackInfo(DWORD dwThreadId, HANDLE hProcess);

//DUMP
void ExportDump();

//����Ȩ��
BOOL WINAPI EnablePrivileges();


//DUMP
void ExportDump() {
	FileSize();

	dump("exe.txt");

	cout << "����dump�ɹ�" << endl;
}



BOOL GetStackInfo(DWORD dwThreadId, HANDLE hProcess)
{
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, NULL, dwThreadId);
	CONTEXT ct = { CONTEXT_CONTROL };
	GetThreadContext(hThread, &ct);
	BYTE buff[512];
	DWORD dwRead = 0;

	if (!ReadProcessMemory(hProcess, (LPVOID)ct.Esp, buff, 512, &dwRead))
	{
		return FALSE;
	}
	printf("%-10s %-10s\n", "Addr", "Value");
	for (int i = 0; i < 10; ++i) {
		printf("%08X   %08X\n", (DWORD)ct.Esp + i * 4, ((DWORD*)buff)[i]);
	}
	return TRUE;
}


BOOL WINAPI EnablePrivileges()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return(FALSE);

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME,
		&tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
		(PTOKEN_PRIVILEGES)NULL, 0);

	if (GetLastError() != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}





int main(){
	EnablePrivileges();
	// ���ô���ҳ,��֧������
	setlocale(LC_ALL, "chs");
	cout << "\t\t---------< ������ >---------\n";
	cout << "\t\t-------< ��h�鿴���� >------\n";


	DbgEngine dbgEng; // �������������

	char szPath[MAX_PATH];
	bool bCreateThread = false;
	unsigned int taddr = 0;
	uintptr_t	 tid = 0;

	uint processId = 0;
	int flag = 0;


	while (true)
	{

		while (true)
		{

			cout << "��ѡ��>>1.��  2.����" << endl;
			cin >> flag;

			//flag = cin.get();
			getchar();
			if (flag == 1) {
				cout << "��ק�ļ���������·���򿪵��Խ��̣�";
				cin.getline(szPath, MAX_PATH);
				file_Name = szPath;
				

				LPTSTR lpsz = new TCHAR[file_Name.GetLength() + 1];
				_tcscpy_s(lpsz, file_Name.GetLength() + 1, file_Name);

				AnalysisPE(lpsz);

				// �򿪵��Խ���
				if (dbgEng.Open(szPath))
					break;

				cout << "�����ʧ��\n";
			}
			else if (flag == 2) {
				printProcessList();
				cout << "������Ҫ���ӵĽ���ID��" << endl;
				cin >> processId;
				//dbgEng.Open(processId);
				//���ӽ���
				if (dbgEng.Open(processId))  break;
	
				//char str[1024];
				//wsprintfA(str, "%ls", file_Name);

				//if (dbgEng.Open(str))
				//	break;


				cout << "���ӽ���ʧ��" << endl;
			}
			else { 
				//fflush(stdin);
				//cin.ignore();
				//cin.clear();
				//cin.sync();
				cin.ignore(1024);
				cout << "���������룺" << endl; 
			}
		
		}

		cout << "���Խ��̴����ɹ�, ���Խ��е���\n";

		g_dwProcessStatus = 0;
		// ���������û�������߳�
		tid = _beginthreadex(0, 0, DbgBreakpointEvent, &dbgEng, 0, &taddr);

		while (1)
		{
			// ���е�����,Exec����������״̬,�����Ҫ����whileѭ����.
			if (e_s_processQuit == dbgEng.Exec()){
				dbgEng.Close();
				system("cls");
				cout << "�������˳�\n";
				g_dwProcessStatus = 1;
				break;
			}
		}
	}
}




//�޸��ڴ� e
DWORD CmdModifyData(CString& str1)
{
	DWORD	dwModifyAddr;
	DWORD	dwOldProtect;
	DWORD	dwReadCount;
	DWORD	dwInputValue;
	byte	bBuffer;
	char	szBuffer[4];

	if (str1.IsEmpty())
	{
		printf("û������Ҫ�޸ĵ��ڴ��ַ\r\n");
		//return MYERRCONTINUE;
		//�������ָʾ�����Գ�������
	}
	//ת������
	char	*pRet = NULL;
	USES_CONVERSION;

	dwModifyAddr = strtoul(T2CA(str1), &pRet, 16);
	//ת��ʧ��
	if (*pRet != NULL)
	{
		printf("Ҫ�޸ĵ��ڴ��ַ�������\r\n");
		//return MYERRCONTINUE;
		return 0;
		//�������ָʾ�����Գ�������
	}
	//
	while (1)
	{
		//�޸��ڴ汣������
		VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, PAGE_READWRITE, &dwOldProtect);
		if (!ReadProcessMemory(Hprocess, (LPVOID)dwModifyAddr, &bBuffer, 1, &dwReadCount)){
			//cout << dwModifyAddr<<endl;
			//printf("%08x\n",dwModifyAddr);
			//printf("%02X\n",bBuffer);
			printf("Ҫ�޸ĵ��ڴ��ַ��Ч\r\n");
			//�޷���ȡ����Ӱ��������ԣ����ﷵ��MYERRCONTINUE
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			//return MYERRCONTINUE;
			return 0;
			//�������ָʾ�����Գ�������
		}


		//��ʾԭֵ
		printf("%p  %02X\r\n", dwModifyAddr, bBuffer);
		//��ȡ�û������޸ĺ��ֵ
		fflush(stdin);
		memset(szBuffer, 0, sizeof(szBuffer));
		//scanf_s("%2[^\n]", szBuffer);
		cin >> szBuffer;
		fflush(stdin);
		//����Ϊ�����˳�e����
		if (szBuffer[0] == NULL){
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			return TRUE;
		}
		//����ת��Ϊ��ֵ����
		dwInputValue = strtoul(szBuffer, &pRet, 16);
		if (*pRet != NULL)
		{
			printf("�����ֵ����\r\n");
			//�޷���ȡ����Ӱ��������ԣ����ﷵ��MYERRCONTINUE
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			//return MYERRCONTINUE;
			return 0;
			//�������ָʾ�����Գ�������
		}
		bBuffer = dwInputValue;

		//д���޸ĺ��ֵ
		if (!WriteProcessMemory(Hprocess, (LPVOID)dwModifyAddr, &bBuffer, 1, &dwReadCount))
		{
			printf("�޸ĺ��ֵд��ʧ��\r\n");
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			//return MYERRCONTINUE;
			return 0;
			//�������ָʾ�����Գ�������
		}

		dwModifyAddr++;
		//��ԭ�ڴ�����
		VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
	}

}




//��ʾԴ��������Ĵ�������
void OnShowSourceLines() {

	int afterLines = 10;
	int beforeLines = 10;


	//��ȡEIP
	CONTEXT context = {};

	//CONTEXT context = {};
	context.ContextFlags = CONTEXT_FULL;

	//if (GetThreadContext(Hthread, &context) == FALSE)

	GetDebuggeeContext(&context);

	//��ȡԴ�ļ��Լ�����Ϣ
	IMAGEHLP_LINE64 lineInfo = {};
	lineInfo.SizeOfStruct = sizeof(lineInfo);
	DWORD displacement = 0;
	//�ɵ�ַ��ȡԴ�ļ�·���Լ��к�


	SymGetLineFromAddr64(
		Hprocess,
		context.Eip,
		&displacement,
		&lineInfo);
		

	/*
	if (SymGetLineFromAddr64(
		Hprocess,
		context.Eip,
		&displacement,
		&lineInfo) == FALSE) {

		DWORD errorCode = GetLastError();

		switch (errorCode) {

			// 126 ��ʾ��û��ͨ��SymLoadModule64����ģ����Ϣ
		case 126:
			std::wcout << TEXT("Debug info in current module has not loaded.") << std::endl;
			return;

			// 487 ��ʾģ��û�е��Է���
		case 487:
			std::wcout << TEXT("No debug info in current module.") << std::endl;
			return;

		default:
			std::wcout << TEXT("SymGetLineFromAddr64 failed: ") << errorCode << std::endl;
			return;
		}
	}
	*/

	//(LPCTSTR)L"f:\\dd\\vctools\\crt\\vcstartup\\src\\startup\\exe_main.cpp", 
	//C:\Users\Tangos\Documents\Visual Studio 2015\Projects\�½��ļ���\helloworld\\helloworld.cpp
	//(LPCTSTR)lineInfo.FileName,
	DisplaySourceLines(
		(LPCTSTR)L"C:\\Users\\Tangos\\Documents\\Visual Studio 2015\\Projects\\�½��ļ���\\helloworld\\helloworld.cpp",
		lineInfo.LineNumber,
		(unsigned int)lineInfo.Address,
		afterLines,
		beforeLines);


	/*	DisplaySourceLines(
			(LPCTSTR)L"C:\\Users\\Tangos\\Documents\\Visual Studio 2015\\Projects\\TangDebugger\\TangDebugger\\helloworld.cpp",
			lineInfo.LineNumber,
			(unsigned int)lineInfo.Address,
			afterLines,
			beforeLines);
			*/
}


//��ӡ�����б�

void printProcessList() {
/*	printf("PID  ======  �ļ���  =======  ·��\n");
	
	HANDLE hToolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hToolhelp == INVALID_HANDLE_VALUE)
	{
		cout << "��ȡ���̿���ʧ��";
	}
	PROCESSENTRY32	stProcess = { 0 };
	stProcess.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hToolhelp, &stProcess);
	for (int i = 0; Process32Next(hToolhelp, &stProcess); i++)
	{
		TCHAR	pszPid[10];
		_itot_s(stProcess.th32ProcessID, pszPid, 10);

		printf("%u      %ws     ", pszPid, stProcess.szExeFile);
		HANDLE	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, stProcess.th32ProcessID);
		TCHAR	pszProcessPath[MAX_PATH + 2];
		if (GetModuleFileNameEx(hProcess, NULL, pszProcessPath, MAX_PATH + 2))
		{
			printf("%ws\n",pszProcessPath);
		}
		else cout << endl;
		CloseHandle(hProcess);
	}
	CloseHandle(hToolhelp);
	return;
*/
	

	printf("��������     ����ID��\n");

	PROCESSENTRY32 pe32;

	pe32.dwSize = sizeof(pe32);

	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE){
		printf("����\n");
		return;
	}

	BOOL bMore = ::Process32First(hProcessSnap, &pe32);

	while (bMore){

		printf("%ws    %u\n", pe32.szExeFile, pe32.th32ProcessID);

		bMore = ::Process32Next(hProcessSnap, &pe32);

	}

	::CloseHandle(hProcessSnap);

	return;
	
}



BOOL CALLBACK EnumerateModuleCallBack(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext) {

	ModuleBaseToNameMap* pModuleMap = (ModuleBaseToNameMap*)UserContext;

	LPCWSTR name = wcsrchr(ModuleName, TEXT('\\')) + 1;

	(*pModuleMap)[(DWORD)ModuleBase] = name;

	return TRUE;
}



//��ȡ�����Խ��̵����̵߳������Ļ�����
BOOL GetDebuggeeContext(CONTEXT* pContext) {
	//SuspendThread(Hthread);


	//CONTEXT context = {};
	//context.ContextFlags = CONTEXT_FULL;

  if (GetThreadContext(Hthread, pContext) == FALSE) {

		std::wcout << TEXT("GetThreadContext failed: ") << GetLastError() << std::endl;
		return FALSE;
	}

	//ResumeThread(Hthread);

	return TRUE;
}


bool stack_walk(STACKFRAME64& stack_frame, CONTEXT& context)
{
	HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
	if (!thread)
	{
		return false;
	}
	bool ret = StackWalk64(IMAGE_FILE_MACHINE_I386, Hprocess, thread, &stack_frame, &context, 
		NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL) == TRUE;

	CloseHandle(thread);

	return ret;
}



bool symbol_from_addr(DWORD addr, std::string& symbol, bool allow_in_func)
{
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	bool ret = SymFromAddr(Hprocess, addr, NULL, pSymbol) == TRUE;// && ;
	if (!ret)
	{
		return false;
	}

	symbol = pSymbol->Name;

	if (pSymbol->Address == addr)
	{
		return true;
	}

	if (allow_in_func)
	{
		char offset[10];
		sprintf_s(offset, "+%X", addr - (DWORD)pSymbol->Address);
		symbol += offset;
		return true;
	}

	return false;
}




//��ʾ���ö�ջ
void OnShowStackTrack() {


	//ö��ģ�飬����ģ��Ļ�ַ-���Ʊ�
	//ModuleBaseToNameMap moduleMap;

	/*if (EnumerateLoadedModules64(Hprocess,
		(PENUMLOADED_MODULES_CALLBACK64)EnumerateModuleCallBack,
		&moduleMap) == FALSE) {

		std::wcout << TEXT("EnumerateLoadedModules64 failed: ") << GetLastError() << std::endl;
		return;
	}*/

	//CONTEXT context;
	//GetDebuggeeContext(&context);

	//STACKFRAME64 stackFrame = { 0 };
	//stackFrame.AddrPC.Mode = AddrModeFlat;
	//stackFrame.AddrPC.Offset = context.Eip;
	//stackFrame.AddrStack.Mode = AddrModeFlat;
	//stackFrame.AddrStack.Offset = context.Esp;
	//stackFrame.AddrFrame.Mode = AddrModeFlat;
	//stackFrame.AddrFrame.Offset = context.Ebp;

	//while (true) {

	//	//��ȡջ֡
	//	if (StackWalk64(
	//		IMAGE_FILE_MACHINE_I386,
	//		Hprocess,
	//		Hthread,
	//		&stackFrame,
	//		&context,
	//		NULL,
	//		SymFunctionTableAccess64,
	//		SymGetModuleBase64,
	//		NULL) == FALSE) {

	//		break;
	//	}

	//	PrintHex((DWORD)stackFrame.AddrPC.Offset, FALSE);
	//	std::wcout << TEXT("  ");

	//	//��ʾģ������
	//	DWORD moduleBase = (DWORD)SymGetModuleBase64(Hprocess, stackFrame.AddrPC.Offset);

	//	const std::wstring& moduleName = moduleMap[moduleBase];

	//	if (moduleName.length() != 0) {
	//		//std::wcout << moduleName;
	//		printf("%ws",moduleName);
	//	}
	//	else {
	//		std::wcout << TEXT("??");
	//	}

	//	std::wcout << TEXT('!');

	//	//��ʾ��������
	//	BYTE buffer[sizeof(SYMBOL_INFO) + 128 * sizeof(TCHAR)] = { 0 };
	//	PSYMBOL_INFO pSymInfo = (PSYMBOL_INFO)buffer;
	//	pSymInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	//	pSymInfo->MaxNameLen = 128;

	//	DWORD64 displacement;

	//	if (SymFromAddr(
	//		Hprocess,
	//		stackFrame.AddrPC.Offset,
	//		&displacement,
	//		pSymInfo) == TRUE) {

	//		std::wcout << pSymInfo->Name << std::endl;
	//	}
	//	else {

	//		std::wcout << TEXT("??") << std::endl;
	//	}
	//}



	//CONTEXT context;
	//GetDebuggeeContext(&context);
	STACKFRAME64 frame = { 0 };
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrPC.Offset = context1.Eip;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context1.Esp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context1.Ebp;


	while (stack_walk(frame, context1))
	{
		char buffer[100];
		std::string symbol;
		sprintf_s(buffer, "%08X", (DWORD)frame.AddrPC.Offset);
		if (symbol_from_addr((DWORD)frame.AddrPC.Offset, symbol, true))
		{
			strcat_s(buffer, "(");
			strcat_s(buffer, symbol.c_str());
			strcat_s(buffer, ")");
		}


		sprintf_s(buffer, "%08X", (DWORD)frame.AddrReturn.Offset);
		

		sprintf_s(buffer, "%08X", (DWORD)frame.AddrFrame.Offset);
		

		sprintf_s(buffer, "%08X", (DWORD)frame.AddrStack.Offset);
		

	}


}



// ��ʾ�����
void ShowAsm(DbgEngine& dbgEngine,
	DbgUi& ui,
	DisAsmEngine& disAsmEng,
	int nLine = 30,
	SIZE_T Addr = 0)
{
	static CONTEXT ct = { CONTEXT_CONTROL };
	if (Addr == 0)
	{
		dbgEngine.GetRegInfo(ct);
		Addr = ct.Eip;
	}

	vector<DISASMSTRUST> vecDisAsm;
	disAsmEng.diAsm(Addr, vecDisAsm, nLine);

	for (vector<DISASMSTRUST>::iterator i = vecDisAsm.begin();
		i != vecDisAsm.end();
		++i)
	{
		ui.showAsm(Addr,
			(*i).strOpCode,
			(*i).strAsm,
			(*i).strCom
		);
		Addr += (*i).dwCodeLen;
	}
}



// ����ϵ�Ļص�����
unsigned int __stdcall DbgBreakpointEvent(void* uParam)
{
	DbgEngine* pDbg = (DbgEngine*)uParam;
	DbgUi ui(pDbg);
	Expression exp(pDbg);
	DisAsmEngine disAsm(pDbg);

	char szCmdLine[64];
	CONTEXT ct = { CONTEXT_ALL };
	vector<DISASMSTRUST> vecDisAsm;
	char* pCmdLine = 0;


	DWORD	dwStatus = 0;
	while (1)
	{
		if (pDbg->WaitForBreakpointEvent(30))
		{
			// ����
			system("cls");
			// ��ȡ�Ĵ�����Ϣ
			pDbg->GetRegInfo(ct);
			// ʹ��uiģ�齫�Ĵ�����Ϣ���
			ui.showReg(ct);
			// ��������
			ShowAsm(*pDbg, ui, disAsm, 20, ct.Eip);
			dwStatus = 1;
		}
		if (dwStatus)
		{
			printf("%s>", dwStatus == 1 ? "��ͣ��" : "������");
			dwStatus = 0;
		}

		if (_kbhit())
		{
			do
			{
				// �����û����������
				gets_s(szCmdLine, 64);
			} while (*szCmdLine == '\0');
			dwStatus = 2;
		}
		else
			continue;


		// ������ͷ�ո�
		pCmdLine = SkipSpace(szCmdLine);

		// �ж��Ƿ���Ҫ�˳�������
		if (*(DWORD*)pCmdLine == 'tixe' || g_dwProcessStatus == 1){
			pDbg->Close();
			pDbg->FinishBreakpointEvnet();
			return 0;
		}


		// �����û����������
		switch (*pCmdLine)
		{			
			/*�鿴�����*/
		case 'u': // �鿴�����
		{
			dwStatus = 1;
			char *pAddr = 0;
			char* pLineCount = 0;
			GetCmdLineArg(pCmdLine + 1, 2, &pAddr, &pLineCount);
			if (pAddr == nullptr)
				pAddr = "eip";
			if (pLineCount == nullptr)
				pLineCount = "20";
			// ��ʾ�����
			ShowAsm(*pDbg, ui, disAsm, exp.getValue(pLineCount), exp.getValue(pAddr));
			break;
		}
		//��ȡ���ö�ջ
		case 'c':
		{	
			dwStatus = 1;
			OnShowStackTrack();
			//GetStackInfo(threadId,Hprocess);
			break;
		}
		//�޸��ڴ�
		case 'x':
		{
			dwStatus = 1;
			pCmdLine = SkipSpace(pCmdLine + 1);

			//printf("%s",pCmdLine);
			CString str(pCmdLine);
			CmdModifyData(str);
			break;
		}


		/*������*/
		case 'a':
		{
			dwStatus = 1;
			// ��ȡ��ʼ��ַ
			XEDPARSE xpre = { 0 };
			xpre.x64 = false; // �Ƿ�ת����64λ��opCode
			memset(xpre.dest, 0x90, XEDPARSE_MAXASMSIZE);
			pCmdLine = SkipSpace(pCmdLine + 1);
			if (*pCmdLine == 0)
			{
				printf("ָ���ʽ����, ��ʽΪ: a ��ַ\n");
				continue;// ��������whileѭ��
			}
			printf("����quit�˳����ģʽ\n");
			uaddr address = exp.getValue(pCmdLine);
			if (address == 0)
				continue;// ��������whileѭ��

			while (true)
			{
				printf("%08X: ", address);
				cin.getline(xpre.instr, XEDPARSE_MAXBUFSIZE);
				if (strcmp(xpre.instr, "quit") == 0)
					break;
				DWORD uLen = disAsm.getCoodeLen(address);
				xpre.cip = address;// ָ�����ڵĵ�ַ
				if (false == XEDParseAssemble(&xpre))
				{
					printf("%s\n", xpre.error);
					continue;// ��������whileѭ��
				}
				// ������д�뵽Ŀ�����
				if (!pDbg->WriteMemory(address, xpre.dest, uLen))
					continue;// ��������whileѭ��
							 // ��ַ++
				address += xpre.dest_size;
			}
			break;
		}
		/*�鿴ջ*/
		case 'k':
		{
			dwStatus = 1;
			pDbg->GetRegInfo(ct);
			BYTE	buff[sizeof(SIZE_T) * 20];
			pDbg->ReadMemory(ct.Esp, buff, sizeof(SIZE_T) * 20);
			ui.showStack(ct.Esp, buff, sizeof(SIZE_T) * 20);
			break;
		}
		/*�鿴���޸ļĴ���*/
		case 'r':/*�Ĵ�����д*/
		{
			dwStatus = 1;
			// ��ȡ�Ĵ�����ֵ:
			// r �Ĵ����� 
			// ���üĴ�����ֵ
			// r �Ĵ����� = ���ʽ
			char* p = szCmdLine + 1;
			p = SkipSpace(p);
			if (*p == 0)
			{
				ct.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
				pDbg->GetRegInfo(ct);
				ui.showReg(ct);
				break;
			}
			SSIZE_T nValue = exp.getValue(szCmdLine + 1);

			char* pSecArg = SkipSpace(szCmdLine + 1);
			pSecArg = GetSecondArg(pSecArg);
			if (*pSecArg == 0)
				printf("%s = 0x%X\n", szCmdLine + 1, nValue);
			break;
		}
		/*�鿴�ڴ�*/
		case 'd':/*�鿴����*/
		{
			dwStatus = 1;
			char *p = &szCmdLine[1];
			// ɸѡ���ݸ�ʽ
			switch (*p)
			{
			case 'u':/*unicode�ַ���*/
			{
				SSIZE_T uAddr = exp.getValue(szCmdLine + 2);
				BYTE lpBuff[16 * 6];
				pDbg->ReadMemory(uAddr, lpBuff, 16 * 6);
				ui.showMem(uAddr, lpBuff, 16 * 6, 1);
			}
			break;

			case 'c':
			{
				get_cmd_line();
			
			}
			break;

			case 'p':
			{
				pDbg->Clear();
				ExportDump();
			}
			break;

			case 'a':/*ansi�ַ���*/
				p = &szCmdLine[2];

			default:
			{
				SSIZE_T uAddr = exp.getValue(p);
				BYTE lpBuff[16 * 6];
				pDbg->ReadMemory(uAddr, lpBuff, 16 * 6);
				ui.showMem(uAddr, lpBuff, 16 * 6, 0);
			}
			break;
			
			}
			break;
		}
		/*��������*/
		case 't': // ����
		{
			// ʹ�õ���������ĺ��������һ��TF�ϵ�
			BPObject *pBp = pDbg->AddBreakPoint(0, breakpointType_tf);
			if (pBp == nullptr)
				return 0;
			char* pCondition = 0;
			GetCmdLineArg(pCmdLine + 1, 1, &pCondition);
			// ���öϵ���ж�����
			if (pCondition != 0)
			{
				pBp->SetCondition(pCondition);
			}
			else
				pBp->SetCondition(true);
			pDbg->FinishBreakpointEvnet();
			break;
		}
		/*��������*/
		case 'p': // ����
		{
			BPObject *pBp = nullptr;
			// �жϵ�ǰ�Ƿ���callָ��
			pDbg->GetRegInfo(ct);
			SIZE_T uEip = ct.Eip;
			BYTE c[2] = { 0 };
			pDbg->ReadMemory(uEip, c, 2);
			DWORD dwCodeLen = 5;
			/**
			��* call �Ļ�������:
			 ��* 0xe8 : 5byte,
			  ��* 0x9a : 7byte,
			   ��* 0xff :
				��*	 0x10ff ~ 0x1dff
				 ��* rep ǰ׺��ָ��Ҳ���Բ���
				  ��*/
			if (c[0] == 0xe8/*call*/
				|| c[0] == 0xf3/*rep*/
				|| c[0] == 0x9a/*call*/
				|| (c[0] == 0xff && 0x10 <= c[1] && c[1] <= 0x1d)/*call*/
				)
			{
				dwCodeLen = disAsm.getCoodeLen(uEip);
				pBp = pDbg->AddBreakPoint(uEip + dwCodeLen, breakpointType_soft);
			}
			else
				pBp = pDbg->AddBreakPoint(0, breakpointType_tf);

			if (pBp == nullptr)
				return 0;
			// ��ȡ����
			char* pCondition = SkipSpace(pCmdLine + 1);
			if (*pCondition != 0)
			{
				pBp->SetCondition(pCondition);
			}
			else
				pBp->SetCondition(true);

			pDbg->FinishBreakpointEvnet();
			break;
		}
		/*�鿴���ص�ģ��*/
		case 'm':
		{
			dwStatus = 1;
			if (*SkipSpace(pCmdLine + 1) == 'l')
			{
				list<MODULEFULLINFO> modList;
				pDbg->GetModuleList(modList);
				printf("+------------------+----------+----------------------------------------------------+\n");
				printf("|     ���ػ�ַ     + ģ���С |                    ģ����                          |\n");
				printf("+------------------+----------+----------------------------------------------------+\n");
				for (auto &i : modList)
				{
					printf("| %016I64X | %08X | %-50s |\n", i.uStart, i.uSize, (LPCSTR)i.name);
				}
				printf("+------------------+----------+----------------------------------------------------+\n");
			}
			continue;// ��������whileѭ��
		}
		/*���öϵ�*/
		case 'b':/*�¶�*/
		{
			dwStatus = 1;
			SetBreakpoint(pDbg, &ui, pCmdLine);
			break;
		}

		/*���г���*/
		case 'g':
			pDbg->FinishBreakpointEvnet();
			break;
			/*�鿴����*/
		case 'h':
		{
			dwStatus = 1;

			char *p = &szCmdLine[1];
			if (*p == 'h') {
				//hook������
				//cout << "Ҫhook�ĺ�������";
				//string str;
				//cin >> str;
				//const char* ch = str.c_str();
				HINSTANCE hDLL;
				hDLL = LoadLibrary(L"kernel32.dll");

				//test
				//FARPROC myfun = (FARPROC)GetProcAddress(hDLL, "OpenProcessToken");
				FARPROC myfun = (FARPROC)GetProcAddress(hDLL, "IsDebuggerPresent");

				//FARPROC myfun = (FARPROC)GetProcAddress(hDLL, ch);
				//�¸�Ӳ���ϵ���  (unsigned int)myfun
				BPObject* pBp = pDbg->AddBreakPoint((unsigned int)myfun, breakpointType_hard_e, 1);
				printf("hello");

			}

			else showHelp();

		}
			break;// ��������whileѭ��
		//�鿴�����
		case 'i':
		{
			//USES_CONVERSION;
			//TCHAR* filePath = CW2T(file_Name);

			GetPEImpTab(pDosH);
			cout << "������Ҫ�鿴�ڼ���DLL��" << endl;
			int numIndex;
			cin >> numIndex;
			ShowImpFunc(numIndex);
			break;
		}
			//�鿴������
		case 'o':
		{
			GetPEExpTab(pDosH);
			ShowExpFunc();
			break;
			}
		case 's': 
		{
			OnShowSourceLines();
		}
		break;

		}
	}


	return 0;
}


// ��ʾ������Ϣ
void showHelp()
{
	printf("----------------------------------------------------\n");
	printf("h : �鿴����\n");
	printf("hh : Hook api\n");
	printf("exit: �˳����ԻỰ\n");
	printf("ml: ��ʾģ���б�\n");
	printf("g : ���г���\n");
	printf("p : ����\n");
	printf("t : ����\n");
	printf("a : ������ģʽ\n");
	printf("    ��ʽΪ : a ��ʼ��ַ\n");
	printf("    ����quit�������ģʽ\n");
	printf("u : �鿴�����\n");
	printf("    ��ʽΪ : u ��ʼ��ַ(���ʽ) ָ������\n");
	printf("    ����   : u eip\n");
	printf("    ����   : u eax 100\n");
	printf("    ����   : u 0x401000 100\n");
	printf("d : �鿴�ڴ�����\n");
	printf("    ��ʽΪ : d ��ʼ��ַ\n");
	printf("    ��ʽΪ : da ��ʼ��ַ(��ʾ�ַ���ʱʹ��ANSIII�ַ�)\n");
	printf("    ��ʽΪ : du ��ʼ��ַ(��ʾ�ַ���ʱʹ��Unicode�ַ�)\n");
	printf("    ��ʽΪ : dp dump����\n");
	printf("x : �޸��ڴ�����\n");
	printf("    ��ʽΪ : x ��ʼ��ַ(��������ֱ�������޸�ֵ)\n");
	printf("r : �鿴/���üĴ���\n");
	printf("    ��ʽΪ : r �Ĵ�����\n");
	printf("    �޸ļĴ��� r eax = 0x1000\n");
	printf("b : ���öϵ�\n");
	printf("    ��ʽ:\n");
	printf("    bp ��ַ �������ʽ => ����ϵ�\n");
	printf("    ����: bp 0x401000 eax==0 && byte[0x403000]==97\n");
	printf("    bh ��ַ �ϵ����� �������ʽ => Ӳ���ϵ�\n");
	printf("    ����: bh 0x401000 e \n");
	printf("    bm ��ַ �ϵ����� �������ʽ => �ڴ�ϵ�\n");
	printf("    ����: bm 0x401000 e \n");
	printf("    bl �г����жϵ�\n");
	printf("    bc ��� ɾ��ָ����ŵĶϵ�\n");
	printf("k : �鿴ջ\n");
	printf("c : �鿴���ö�ջ\n");
	printf("i : �鿴�����Գ���ĵ����\n");
	printf("o : �鿴�����Գ���ĵ�����\n");
	printf("dc:������\n");
	printf("----------------------------------------------------\n");

}

// ��ȡ�ڶ�������(����֮���Կո�����
char* GetSecondArg(char* pBuff)
{
	for (; *pBuff != 0; ++pBuff)
	{
		if (*pBuff == ' ')//�ҵ���һ���ո�
		{
			*pBuff = 0; // �ѿո����ַ���������,�ָ���������
			return pBuff + 1;//���صڶ��������Ŀ�ʼ��ַ
		}
	}
	return pBuff;
}

// �����ո�(�������з�,tab��)
inline char* SkipSpace(char* pBuff)
{
	for (; *pBuff == ' ' || *pBuff == '\t' || *pBuff == '\r' || *pBuff == '\n'; ++pBuff);
	return pBuff;
}


void GetCmdLineArg(char* pszCmdLine, int nArgCount, ...)
{
	va_list argptr;
	va_start(argptr, nArgCount);

	while (nArgCount-- > 0 && *pszCmdLine != 0)
	{
		if (*pszCmdLine == ' ' || *pszCmdLine == '\t')
			*pszCmdLine++ = 0;

		pszCmdLine = SkipSpace(pszCmdLine);
		if (*pszCmdLine == 0)
			break;
		DWORD*& dwArg = va_arg(argptr, DWORD*);
		*dwArg = (DWORD)pszCmdLine;

		for (; *pszCmdLine != 0 && *pszCmdLine != ' ' && *pszCmdLine != '\t'; ++pszCmdLine);
	}
	va_end(argptr);
}

// ���öϵ�
void SetBreakpoint(DbgEngine* pDbg, DbgUi* pUi, char* szCmdLine)
{
	char* pAddr = 0;//�ϵ��ַ
	char* pType = 0;//�ϵ�����
	char* pLen = 0;//�ϵ㳤��
	char* pRule = 0; // �ϵ����й���
	Expression exp(pDbg);

	char  cType = *(SkipSpace(szCmdLine + 1));// �ϵ�����
	E_BPType   bpType = e_bt_none;
	SIZE_T   uAddr = 0; // �¶ϵ�ַ
	uint		uBPLen = 1;
	switch (cType)
	{
	case 'p':/*��ͨ�ϵ�*/
	{
		char* pAddrr = 0;
		bpType = breakpointType_soft;
		GetCmdLineArg(szCmdLine + 2, 2, &pAddrr, &pRule);
		if (pAddrr == nullptr)
			pAddrr = "eip";
		// �õ���ַ
		uAddr = exp.getValue(pAddrr);
		break;
	}
	case 'l':/*�ϵ��б�*/
	{
		pUi->showBreakPointList(pDbg->GetBPListBegin(), pDbg->GetBPListEnd());
		return;
	}
	case 'c':/*ɾ���ϵ�*/
	{
		DWORD	dwIndex = 0;
		sscanf_s(szCmdLine + 2, "%d", &dwIndex);
		pDbg->DeleteBreakpoint(dwIndex);
		return;
	}
	case 'n':/*�������ϵ�*/
	{
		if (nullptr == pDbg->AddBreakPoint(SkipSpace(szCmdLine + 2)))
			cout << "�Ҳ�������\n";
		cout << "���óɹ�!\n";
		return;
	}
	//Ӳ���ϵ��������m��֧����
	case 'h':/*Ӳ���ϵ�*/
	case 'm': /*�ڴ���ʶϵ�*/
	{
		GetCmdLineArg(szCmdLine + 2, 4, &pAddr, &pType, &pLen, &pRule);
		if (pAddr == 0 || pType == 0)
		{
			printf("bm/bh ��ַ ����(r/w/e) ����(1/2/4)(��ѡ) ����(��ѡ)\n");
			return;
		}

		uAddr = exp.getValue(pAddr);
		switch (*pType) // ɸѡ�ϵ������
		{
		case 'r':bpType = cType == 'm' ? breakpointType_acc_r : breakpointType_hard_r; break;
		case 'w':bpType = cType == 'm' ? breakpointType_acc_w : breakpointType_hard_w; break;
		case 'e':bpType = cType == 'm' ? breakpointType_acc_e : breakpointType_hard_e; break;
		default:
			printf("�ϵ��������ô���,���ʶϵ��������: r(��),w(д),e(ִ��)\n");
			return;
		}

		uBPLen = exp.getValue(pLen);
		if (uBPLen == 0)
			uBPLen = 1;

		if (pLen == 0 && cType == 'h') //�����Ӳ���ϵ�,�򽫳�����Ϊ0
			pLen = "0";
		else if (pLen > 0 && cType == 'h') // ���Ӳ���ϵ��ַ�ͳ��ȵĶ�Ӧ��ϵ
		{
			if (*pType == 'e') // �����ִ�жϵ�,����ֻ��Ϊ0
				uBPLen = 0;
			else // ����Ƕ�д�ϵ�,�ϵ��ַ�ͳ��ȱ��������Ӧ��ϵ
			{
				// Ĭ�ϳ���Ϊ4���ֽ�
				uBPLen = 3;
				// ���������4 , ����ַ������4�ı���,��ϵ㳤�������2���ֽ�
				if (*pLen == '4' && uAddr % 4 != 0)
					uBPLen = 1;
				// ���������2 , ����ַ������2�ı���,��ϵ㳤�������1���ֽ�
				if (*pLen == '2' && uAddr % 2 != 0)
					uBPLen = 0;
			}
		}
		break;
	}
	default:
		cout << "û�и����͵Ķϵ�\n";
		return;
	}

	// ��ȡ��ϵ�ĵ�ַ,����,������, �����¶�.
	BPObject* pBp = pDbg->AddBreakPoint(uAddr, bpType, uBPLen);
	if (pBp == nullptr)
	{
		printf("���öϵ�ʧ��\n");
		return;
	}

	// ����ϵ�Я�����ʽ, ��ѱ��ʽ���õ��ϵ���
	if (pRule != nullptr)
		BreakpointEngine::SetExp(pBp, pRule);
	return;
}

