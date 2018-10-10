#include "stdafx.h"
#include "DBGSymbol.h"
#include <atlstr.h>

DbgSymbol::DbgSymbol()
{
}



DbgSymbol::~DbgSymbol()
{
}

//��ʼ������
void DbgSymbol::InitSymbol(HANDLE hProcess)
{
	DWORD Options = SymGetOptions();
	Options |= SYMOPT_DEBUG;
	::SymSetOptions(Options);

	::SymInitialize(hProcess,0,TRUE);
	return;
}

//��ȡ���ź�����Ӧ�ĵ�ַ
SIZE_T DbgSymbol::FindApiAddress(HANDLE hProcess, const char* pszName)
{
	DWORD64  dwDisplacement = 0;
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	//�������ֲ�ѯ������Ϣ�������pSymbol��
	if (!SymFromName(hProcess, pszName, pSymbol))
	{
		return 0;
	}
	//���غ�����ַ 
	return (SIZE_T)pSymbol->Address;
}

//��ȡ��ַ��Ӧ�ķ���
BOOL DbgSymbol::GetFunctionName(HANDLE hProcess, SIZE_T nAddress, CString& strName)
{
	DWORD64  dwDisplacement = 0;
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	//���ݵ�ַ��ȡ������Ϣ
	if (!SymFromAddr(hProcess, nAddress, &dwDisplacement, pSymbol))
		return FALSE;

	strName = pSymbol->Name;
	return TRUE;
}
