#pragma once
#include <Windows.h>
#include <atlstr.h>
#include <list>
using std::list;

#include "BreakpointEngine.h"


#define SHOW_TEXT_HEX		0x000000001
#define SHOW_TEXT_DEC		0x000000002
#define SHOW_LEN_BYTE		0x000000004
#define SHOW_LEN_WORD		0x000000008
#define SHOW_LEN_DWORD		0x000000010
#define SHOW_LEN_QWORD		0x000000020
#define	SHOW_TEXT_ANSI		0x000000040
#define	SHOW_TEXT_UNICODE	0x000000080

typedef enum
{
	e_st_log = 0,
	//�Ĵ�����Ϣ
	e_st_regInfo,
	//�ڴ���Ϣ
	e_st_memInfo,
	//�������Ϣ
	e_st_DisAsmInfo
}E_ShowType;

class DbgUi
{
	static HANDLE	m_hStdOut;
	BreakpointEngine* m_pBpEngine;
public:
	DbgUi(BreakpointEngine* pBpEngine);
	~DbgUi();
	//�������ʹ�ӡ
	void Show(E_ShowType type, const CStringA& pszData);
	//��ӡ�ض�λ��ĳ��ɫ����
	void Show(const char* pszStr, int x, int y, int color);

	// ��ʽ: ��ɫ1 ,�ַ���1.......��ɫX,�ַ���X(��0��β.)
	void ShowEx(int x, int y, ...);

	//��ӡ�Ĵ���
	inline void printReg(SIZE_T dwReg, WORD color);
	//��ӡ��־λ
	inline void printEflag(DWORD dwFlag, WORD color);
public:
	//��ʾ���
	void showAsm(SIZE_T Addr,
		const WCHAR* ShowOpc,
		const WCHAR* pszDiAsm,
		const WCHAR* pszCom,
		const WCHAR* pszLineHeader = L"  "
	);
	//��ʾ�Ĵ���
	void showReg(const CONTEXT& ct);
	//��ʾ�ڴ�
	void showMem(SIZE_T virtualAddress, const LPBYTE lpBuff, int nSize, DWORD dwShowFlag);
	//��ʾջ��Ϣ
	void showStack(SIZE_T virtualAddress, const LPBYTE lpBuff, int nSize);
	//��ʾ�ϵ��б�
	void showBreakPointList(list<BPObject*>::const_iterator beginItr, list<BPObject*>::const_iterator endItr);
};