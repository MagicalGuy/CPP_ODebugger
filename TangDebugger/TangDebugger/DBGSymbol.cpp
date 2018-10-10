#include "stdafx.h"
#include "DBGSymbol.h"
#include <atlstr.h>

DbgSymbol::DbgSymbol()
{
}



DbgSymbol::~DbgSymbol()
{
}

//初始化符号
void DbgSymbol::InitSymbol(HANDLE hProcess)
{
	DWORD Options = SymGetOptions();
	Options |= SYMOPT_DEBUG;
	::SymSetOptions(Options);

	::SymInitialize(hProcess,0,TRUE);
	return;
}

//获取符号函数对应的地址
SIZE_T DbgSymbol::FindApiAddress(HANDLE hProcess, const char* pszName)
{
	DWORD64  dwDisplacement = 0;
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	//根据名字查询符号信息，输出到pSymbol中
	if (!SymFromName(hProcess, pszName, pSymbol))
	{
		return 0;
	}
	//返回函数地址 
	return (SIZE_T)pSymbol->Address;
}

//获取地址对应的符号
BOOL DbgSymbol::GetFunctionName(HANDLE hProcess, SIZE_T nAddress, CString& strName)
{
	DWORD64  dwDisplacement = 0;
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	//根据地址获取符号信息
	if (!SymFromAddr(hProcess, nAddress, &dwDisplacement, pSymbol))
		return FALSE;

	strName = pSymbol->Name;
	return TRUE;
}
