// TangDebugger.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"



#include"TangDebugger.h"
#include "DbgEngine.h" // 调试引擎
#include "dbgUi.h" // 用户界面
#include "Expression.h" // 表达式模块
#include "DisAsmEngine.h" // 反汇编引擎
#include "XEDParse.h" // 汇编引擎
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

//进程加载基址
uaddr imgBase;

PIMAGE_DOS_HEADER pDosH;	//DOS头
char* buff;
CString file_Name;

// 显示调试器命令行帮助信息
void showHelp();
// 将字符串分割成两个字符串(将第一次遇到的空格替换成字符串结束符)
char* GetSecondArg(char* pBuff);
inline char* SkipSpace(char* pBuff);
 

// 调试器引擎的断点处理回调函数
unsigned int __stdcall DbgBreakpointEvent(void* uParam);
unsigned int __stdcall OtherDbgBreakpointEvent(void* uParam);


// 获取命令行中的参数
void GetCmdLineArg(char* pszCmdLine, int nArgCount, ...);
// 设置断点
void SetBreakpoint(DbgEngine* pDbg, DbgUi* pUi, char* szCmdLine);
DWORD	g_dwProcessStatus = 0;


typedef std::map<DWORD, std::wstring> ModuleBaseToNameMap;


//打印进程列表用于附加
void printProcessList();


//遍历调用堆栈
bool stack_walk(STACKFRAME64& stack_frame, CONTEXT& context);
bool symbol_from_addr(DWORD addr, std::string& symbol, bool allow_in_func);


//显示调用堆栈
void OnShowStackTrack();

//暂时弃用的枚举模块回调
BOOL CALLBACK EnumerateModuleCallBack(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext);


//显示源码
void OnShowSourceLines();

//获取被调试进程的主线程的上下文环境。
BOOL GetDebuggeeContext(CONTEXT* pContext);

//修改内存
DWORD CmdModifyData(CString& str1);


//获取堆栈信息
BOOL GetStackInfo(DWORD dwThreadId, HANDLE hProcess);

//DUMP
void ExportDump();

//提升权限
BOOL WINAPI EnablePrivileges();


//DUMP
void ExportDump() {
	FileSize();

	dump("exe.txt");

	cout << "导出dump成功" << endl;
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
	// 设置代码页,以支持中文
	setlocale(LC_ALL, "chs");
	cout << "\t\t---------< 调试器 >---------\n";
	cout << "\t\t-------< 按h查看帮助 >------\n";


	DbgEngine dbgEng; // 调试器引擎对象

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

			cout << "请选择>>1.打开  2.附加" << endl;
			cin >> flag;

			//flag = cin.get();
			getchar();
			if (flag == 1) {
				cout << "拖拽文件或者输入路径打开调试进程：";
				cin.getline(szPath, MAX_PATH);
				file_Name = szPath;
				

				LPTSTR lpsz = new TCHAR[file_Name.GetLength() + 1];
				_tcscpy_s(lpsz, file_Name.GetLength() + 1, file_Name);

				AnalysisPE(lpsz);

				// 打开调试进程
				if (dbgEng.Open(szPath))
					break;

				cout << "程序打开失败\n";
			}
			else if (flag == 2) {
				printProcessList();
				cout << "请输入要附加的进程ID：" << endl;
				cin >> processId;
				//dbgEng.Open(processId);
				//附加进程
				if (dbgEng.Open(processId))  break;
	
				//char str[1024];
				//wsprintfA(str, "%ls", file_Name);

				//if (dbgEng.Open(str))
				//	break;


				cout << "附加进程失败" << endl;
			}
			else { 
				//fflush(stdin);
				//cin.ignore();
				//cin.clear();
				//cin.sync();
				cin.ignore(1024);
				cout << "请重新输入：" << endl; 
			}
		
		}

		cout << "调试进程创建成功, 可以进行调试\n";

		g_dwProcessStatus = 0;
		// 开启接收用户输入的线程
		tid = _beginthreadex(0, 0, DbgBreakpointEvent, &dbgEng, 0, &taddr);

		while (1)
		{
			// 运行调试器,Exec不处于阻塞状态,因此需要放在while循环中.
			if (e_s_processQuit == dbgEng.Exec()){
				dbgEng.Close();
				system("cls");
				cout << "进程已退出\n";
				g_dwProcessStatus = 1;
				break;
			}
		}
	}
}




//修改内存 e
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
		printf("没有输入要修改的内存地址\r\n");
		//return MYERRCONTINUE;
		//命令完成指示被调试程序运行
	}
	//转换参数
	char	*pRet = NULL;
	USES_CONVERSION;

	dwModifyAddr = strtoul(T2CA(str1), &pRet, 16);
	//转换失败
	if (*pRet != NULL)
	{
		printf("要修改的内存地址输入错误\r\n");
		//return MYERRCONTINUE;
		return 0;
		//命令完成指示被调试程序运行
	}
	//
	while (1)
	{
		//修改内存保护属性
		VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, PAGE_READWRITE, &dwOldProtect);
		if (!ReadProcessMemory(Hprocess, (LPVOID)dwModifyAddr, &bBuffer, 1, &dwReadCount)){
			//cout << dwModifyAddr<<endl;
			//printf("%08x\n",dwModifyAddr);
			//printf("%02X\n",bBuffer);
			printf("要修改的内存地址无效\r\n");
			//无法读取错误不影响后续调试，这里返回MYERRCONTINUE
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			//return MYERRCONTINUE;
			return 0;
			//命令完成指示被调试程序运行
		}


		//显示原值
		printf("%p  %02X\r\n", dwModifyAddr, bBuffer);
		//获取用户输入修改后的值
		fflush(stdin);
		memset(szBuffer, 0, sizeof(szBuffer));
		//scanf_s("%2[^\n]", szBuffer);
		cin >> szBuffer;
		fflush(stdin);
		//输入为空则退出e命令
		if (szBuffer[0] == NULL){
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			return TRUE;
		}
		//输入转换为数值类型
		dwInputValue = strtoul(szBuffer, &pRet, 16);
		if (*pRet != NULL)
		{
			printf("输入的值错误\r\n");
			//无法读取错误不影响后续调试，这里返回MYERRCONTINUE
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			//return MYERRCONTINUE;
			return 0;
			//命令完成指示被调试程序运行
		}
		bBuffer = dwInputValue;

		//写入修改后的值
		if (!WriteProcessMemory(Hprocess, (LPVOID)dwModifyAddr, &bBuffer, 1, &dwReadCount))
		{
			printf("修改后的值写入失败\r\n");
			VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
			//return MYERRCONTINUE;
			return 0;
			//命令完成指示被调试程序运行
		}

		dwModifyAddr++;
		//还原内存属性
		VirtualProtectEx(Hprocess, (LPVOID)dwModifyAddr, 1, dwOldProtect, &dwReadCount);
	}

}




//显示源代码命令的处理函数。
void OnShowSourceLines() {

	int afterLines = 10;
	int beforeLines = 10;


	//获取EIP
	CONTEXT context = {};

	//CONTEXT context = {};
	context.ContextFlags = CONTEXT_FULL;

	//if (GetThreadContext(Hthread, &context) == FALSE)

	GetDebuggeeContext(&context);

	//获取源文件以及行信息
	IMAGEHLP_LINE64 lineInfo = {};
	lineInfo.SizeOfStruct = sizeof(lineInfo);
	DWORD displacement = 0;
	//由地址获取源文件路径以及行号


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

			// 126 表示还没有通过SymLoadModule64加载模块信息
		case 126:
			std::wcout << TEXT("Debug info in current module has not loaded.") << std::endl;
			return;

			// 487 表示模块没有调试符号
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
	//C:\Users\Tangos\Documents\Visual Studio 2015\Projects\新建文件夹\helloworld\\helloworld.cpp
	//(LPCTSTR)lineInfo.FileName,
	DisplaySourceLines(
		(LPCTSTR)L"C:\\Users\\Tangos\\Documents\\Visual Studio 2015\\Projects\\新建文件夹\\helloworld\\helloworld.cpp",
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


//打印进程列表

void printProcessList() {
/*	printf("PID  ======  文件名  =======  路径\n");
	
	HANDLE hToolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hToolhelp == INVALID_HANDLE_VALUE)
	{
		cout << "获取进程快照失败";
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
	

	printf("进程名称     进程ID号\n");

	PROCESSENTRY32 pe32;

	pe32.dwSize = sizeof(pe32);

	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE){
		printf("出错\n");
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



//获取被调试进程的主线程的上下文环境。
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




//显示调用堆栈
void OnShowStackTrack() {


	//枚举模块，建立模块的基址-名称表
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

	//	//获取栈帧
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

	//	//显示模块名称
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

	//	//显示函数名称
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



// 显示反汇编
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



// 处理断点的回调函数
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
			// 清屏
			system("cls");
			// 获取寄存器信息
			pDbg->GetRegInfo(ct);
			// 使用ui模块将寄存器信息输出
			ui.showReg(ct);
			// 输出反汇编
			ShowAsm(*pDbg, ui, disAsm, 20, ct.Eip);
			dwStatus = 1;
		}
		if (dwStatus)
		{
			printf("%s>", dwStatus == 1 ? "暂停中" : "运行中");
			dwStatus = 0;
		}

		if (_kbhit())
		{
			do
			{
				// 接收用户输入的命令
				gets_s(szCmdLine, 64);
			} while (*szCmdLine == '\0');
			dwStatus = 2;
		}
		else
			continue;


		// 跳过行头空格
		pCmdLine = SkipSpace(szCmdLine);

		// 判断是否需要退出调试器
		if (*(DWORD*)pCmdLine == 'tixe' || g_dwProcessStatus == 1){
			pDbg->Close();
			pDbg->FinishBreakpointEvnet();
			return 0;
		}


		// 解析用户输入的命令
		switch (*pCmdLine)
		{			
			/*查看反汇编*/
		case 'u': // 查看反汇编
		{
			dwStatus = 1;
			char *pAddr = 0;
			char* pLineCount = 0;
			GetCmdLineArg(pCmdLine + 1, 2, &pAddr, &pLineCount);
			if (pAddr == nullptr)
				pAddr = "eip";
			if (pLineCount == nullptr)
				pLineCount = "20";
			// 显示反汇编
			ShowAsm(*pDbg, ui, disAsm, exp.getValue(pLineCount), exp.getValue(pAddr));
			break;
		}
		//获取调用堆栈
		case 'c':
		{	
			dwStatus = 1;
			OnShowStackTrack();
			//GetStackInfo(threadId,Hprocess);
			break;
		}
		//修改内存
		case 'x':
		{
			dwStatus = 1;
			pCmdLine = SkipSpace(pCmdLine + 1);

			//printf("%s",pCmdLine);
			CString str(pCmdLine);
			CmdModifyData(str);
			break;
		}


		/*输入汇编*/
		case 'a':
		{
			dwStatus = 1;
			// 获取开始地址
			XEDPARSE xpre = { 0 };
			xpre.x64 = false; // 是否转换成64位的opCode
			memset(xpre.dest, 0x90, XEDPARSE_MAXASMSIZE);
			pCmdLine = SkipSpace(pCmdLine + 1);
			if (*pCmdLine == 0)
			{
				printf("指令格式错误, 格式为: a 地址\n");
				continue;// 结束本次while循环
			}
			printf("输入quit退出汇编模式\n");
			uaddr address = exp.getValue(pCmdLine);
			if (address == 0)
				continue;// 结束本次while循环

			while (true)
			{
				printf("%08X: ", address);
				cin.getline(xpre.instr, XEDPARSE_MAXBUFSIZE);
				if (strcmp(xpre.instr, "quit") == 0)
					break;
				DWORD uLen = disAsm.getCoodeLen(address);
				xpre.cip = address;// 指令所在的地址
				if (false == XEDParseAssemble(&xpre))
				{
					printf("%s\n", xpre.error);
					continue;// 结束本次while循环
				}
				// 将代码写入到目标进程
				if (!pDbg->WriteMemory(address, xpre.dest, uLen))
					continue;// 结束本次while循环
							 // 地址++
				address += xpre.dest_size;
			}
			break;
		}
		/*查看栈*/
		case 'k':
		{
			dwStatus = 1;
			pDbg->GetRegInfo(ct);
			BYTE	buff[sizeof(SIZE_T) * 20];
			pDbg->ReadMemory(ct.Esp, buff, sizeof(SIZE_T) * 20);
			ui.showStack(ct.Esp, buff, sizeof(SIZE_T) * 20);
			break;
		}
		/*查看和修改寄存器*/
		case 'r':/*寄存器读写*/
		{
			dwStatus = 1;
			// 获取寄存器的值:
			// r 寄存器名 
			// 设置寄存器的值
			// r 寄存器名 = 表达式
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
		/*查看内存*/
		case 'd':/*查看数据*/
		{
			dwStatus = 1;
			char *p = &szCmdLine[1];
			// 筛选数据格式
			switch (*p)
			{
			case 'u':/*unicode字符串*/
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

			case 'a':/*ansi字符串*/
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
		/*单步步入*/
		case 't': // 步入
		{
			// 使用调试器引擎的函数来添加一个TF断点
			BPObject *pBp = pDbg->AddBreakPoint(0, breakpointType_tf);
			if (pBp == nullptr)
				return 0;
			char* pCondition = 0;
			GetCmdLineArg(pCmdLine + 1, 1, &pCondition);
			// 设置断点的中断条件
			if (pCondition != 0)
			{
				pBp->SetCondition(pCondition);
			}
			else
				pBp->SetCondition(true);
			pDbg->FinishBreakpointEvnet();
			break;
		}
		/*单步步过*/
		case 'p': // 步过
		{
			BPObject *pBp = nullptr;
			// 判断当前是否是call指令
			pDbg->GetRegInfo(ct);
			SIZE_T uEip = ct.Eip;
			BYTE c[2] = { 0 };
			pDbg->ReadMemory(uEip, c, 2);
			DWORD dwCodeLen = 5;
			/**
			　* call 的机器码有:
			 　* 0xe8 : 5byte,
			  　* 0x9a : 7byte,
			   　* 0xff :
				　*	 0x10ff ~ 0x1dff
				 　* rep 前缀的指令也可以步过
				  　*/
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
			// 获取条件
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
		/*查看加载的模块*/
		case 'm':
		{
			dwStatus = 1;
			if (*SkipSpace(pCmdLine + 1) == 'l')
			{
				list<MODULEFULLINFO> modList;
				pDbg->GetModuleList(modList);
				printf("+------------------+----------+----------------------------------------------------+\n");
				printf("|     加载基址     + 模块大小 |                    模块名                          |\n");
				printf("+------------------+----------+----------------------------------------------------+\n");
				for (auto &i : modList)
				{
					printf("| %016I64X | %08X | %-50s |\n", i.uStart, i.uSize, (LPCSTR)i.name);
				}
				printf("+------------------+----------+----------------------------------------------------+\n");
			}
			continue;// 结束本次while循环
		}
		/*设置断点*/
		case 'b':/*下断*/
		{
			dwStatus = 1;
			SetBreakpoint(pDbg, &ui, pCmdLine);
			break;
		}

		/*运行程序*/
		case 'g':
			pDbg->FinishBreakpointEvnet();
			break;
			/*查看帮助*/
		case 'h':
		{
			dwStatus = 1;

			char *p = &szCmdLine[1];
			if (*p == 'h') {
				//hook函数名
				//cout << "要hook的函数名：";
				//string str;
				//cin >> str;
				//const char* ch = str.c_str();
				HINSTANCE hDLL;
				hDLL = LoadLibrary(L"kernel32.dll");

				//test
				//FARPROC myfun = (FARPROC)GetProcAddress(hDLL, "OpenProcessToken");
				FARPROC myfun = (FARPROC)GetProcAddress(hDLL, "IsDebuggerPresent");

				//FARPROC myfun = (FARPROC)GetProcAddress(hDLL, ch);
				//下个硬件断点在  (unsigned int)myfun
				BPObject* pBp = pDbg->AddBreakPoint((unsigned int)myfun, breakpointType_hard_e, 1);
				printf("hello");

			}

			else showHelp();

		}
			break;// 结束本次while循环
		//查看导入表
		case 'i':
		{
			//USES_CONVERSION;
			//TCHAR* filePath = CW2T(file_Name);

			GetPEImpTab(pDosH);
			cout << "请输入要查看第几个DLL：" << endl;
			int numIndex;
			cin >> numIndex;
			ShowImpFunc(numIndex);
			break;
		}
			//查看导出表
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


// 显示帮助信息
void showHelp()
{
	printf("----------------------------------------------------\n");
	printf("h : 查看帮助\n");
	printf("hh : Hook api\n");
	printf("exit: 退出调试会话\n");
	printf("ml: 显示模块列表\n");
	printf("g : 运行程序\n");
	printf("p : 单步\n");
	printf("t : 步过\n");
	printf("a : 进入汇编模式\n");
	printf("    格式为 : a 开始地址\n");
	printf("    输入quit结束汇编模式\n");
	printf("u : 查看反汇编\n");
	printf("    格式为 : u 开始地址(表达式) 指令条数\n");
	printf("    例如   : u eip\n");
	printf("    例如   : u eax 100\n");
	printf("    例如   : u 0x401000 100\n");
	printf("d : 查看内存数据\n");
	printf("    格式为 : d 开始地址\n");
	printf("    格式为 : da 开始地址(显示字符串时使用ANSIII字符)\n");
	printf("    格式为 : du 开始地址(显示字符串时使用Unicode字符)\n");
	printf("    格式为 : dp dump功能\n");
	printf("x : 修改内存数据\n");
	printf("    格式为 : x 开始地址(后续操作直接输入修改值)\n");
	printf("r : 查看/设置寄存器\n");
	printf("    格式为 : r 寄存器名\n");
	printf("    修改寄存器 r eax = 0x1000\n");
	printf("b : 设置断点\n");
	printf("    格式:\n");
	printf("    bp 地址 条件表达式 => 软件断点\n");
	printf("    例如: bp 0x401000 eax==0 && byte[0x403000]==97\n");
	printf("    bh 地址 断点属性 条件表达式 => 硬件断点\n");
	printf("    例如: bh 0x401000 e \n");
	printf("    bm 地址 断点属性 条件表达式 => 内存断点\n");
	printf("    例如: bm 0x401000 e \n");
	printf("    bl 列出所有断点\n");
	printf("    bc 序号 删除指定序号的断点\n");
	printf("k : 查看栈\n");
	printf("c : 查看调用堆栈\n");
	printf("i : 查看被调试程序的导入表\n");
	printf("o : 查看被调试程序的导出表\n");
	printf("dc:加入插件\n");
	printf("----------------------------------------------------\n");

}

// 获取第二个参数(参数之间以空格间隔开
char* GetSecondArg(char* pBuff)
{
	for (; *pBuff != 0; ++pBuff)
	{
		if (*pBuff == ' ')//找到第一个空格
		{
			*pBuff = 0; // 把空格变成字符串结束符,分隔两个参数
			return pBuff + 1;//返回第二个参数的开始地址
		}
	}
	return pBuff;
}

// 跳过空格(包括换行符,tab符)
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

// 设置断点
void SetBreakpoint(DbgEngine* pDbg, DbgUi* pUi, char* szCmdLine)
{
	char* pAddr = 0;//断点地址
	char* pType = 0;//断点类型
	char* pLen = 0;//断点长度
	char* pRule = 0; // 断点命中规则
	Expression exp(pDbg);

	char  cType = *(SkipSpace(szCmdLine + 1));// 断点类型
	E_BPType   bpType = e_bt_none;
	SIZE_T   uAddr = 0; // 下断地址
	uint		uBPLen = 1;
	switch (cType)
	{
	case 'p':/*普通断点*/
	{
		char* pAddrr = 0;
		bpType = breakpointType_soft;
		GetCmdLineArg(szCmdLine + 2, 2, &pAddrr, &pRule);
		if (pAddrr == nullptr)
			pAddrr = "eip";
		// 得到地址
		uAddr = exp.getValue(pAddrr);
		break;
	}
	case 'l':/*断点列表*/
	{
		pUi->showBreakPointList(pDbg->GetBPListBegin(), pDbg->GetBPListEnd());
		return;
	}
	case 'c':/*删除断点*/
	{
		DWORD	dwIndex = 0;
		sscanf_s(szCmdLine + 2, "%d", &dwIndex);
		pDbg->DeleteBreakpoint(dwIndex);
		return;
	}
	case 'n':/*函数名断点*/
	{
		if (nullptr == pDbg->AddBreakPoint(SkipSpace(szCmdLine + 2)))
			cout << "找不到符号\n";
		cout << "设置成功!\n";
		return;
	}
	//硬件断点在下面的m分支中找
	case 'h':/*硬件断点*/
	case 'm': /*内存访问断点*/
	{
		GetCmdLineArg(szCmdLine + 2, 4, &pAddr, &pType, &pLen, &pRule);
		if (pAddr == 0 || pType == 0)
		{
			printf("bm/bh 地址 类型(r/w/e) 长度(1/2/4)(可选) 条件(可选)\n");
			return;
		}

		uAddr = exp.getValue(pAddr);
		switch (*pType) // 筛选断点的类型
		{
		case 'r':bpType = cType == 'm' ? breakpointType_acc_r : breakpointType_hard_r; break;
		case 'w':bpType = cType == 'm' ? breakpointType_acc_w : breakpointType_hard_w; break;
		case 'e':bpType = cType == 'm' ? breakpointType_acc_e : breakpointType_hard_e; break;
		default:
			printf("断点类型设置错误,访问断点的类型有: r(读),w(写),e(执行)\n");
			return;
		}

		uBPLen = exp.getValue(pLen);
		if (uBPLen == 0)
			uBPLen = 1;

		if (pLen == 0 && cType == 'h') //如果是硬件断点,则将长度设为0
			pLen = "0";
		else if (pLen > 0 && cType == 'h') // 检测硬件断点地址和长度的对应关系
		{
			if (*pType == 'e') // 如果是执行断点,长度只能为0
				uBPLen = 0;
			else // 如果是读写断点,断点地址和长度必须满足对应关系
			{
				// 默认长度为4个字节
				uBPLen = 3;
				// 如果长度是4 , 但地址并不是4的倍数,则断点长度最多是2个字节
				if (*pLen == '4' && uAddr % 4 != 0)
					uBPLen = 1;
				// 如果长度是2 , 但地址并不是2的倍数,则断点长度最多是1个字节
				if (*pLen == '2' && uAddr % 2 != 0)
					uBPLen = 0;
			}
		}
		break;
	}
	default:
		cout << "没有该类型的断点\n";
		return;
	}

	// 获取完断点的地址,类型,条件后, 进行下断.
	BPObject* pBp = pDbg->AddBreakPoint(uAddr, bpType, uBPLen);
	if (pBp == nullptr)
	{
		printf("设置断点失败\n");
		return;
	}

	// 如果断点携带表达式, 则把表达式设置到断点上
	if (pRule != nullptr)
		BreakpointEngine::SetExp(pBp, pRule);
	return;
}

