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
	//寄存器信息
	e_st_regInfo,
	//内存信息
	e_st_memInfo,
	//反汇编信息
	e_st_DisAsmInfo
}E_ShowType;

class DbgUi
{
	static HANDLE	m_hStdOut;
	BreakpointEngine* m_pBpEngine;
public:
	DbgUi(BreakpointEngine* pBpEngine);
	~DbgUi();
	//根据类型打印
	void Show(E_ShowType type, const CStringA& pszData);
	//打印特定位置某颜色数据
	void Show(const char* pszStr, int x, int y, int color);

	// 格式: 颜色1 ,字符串1.......颜色X,字符串X(以0结尾.)
	void ShowEx(int x, int y, ...);

	//打印寄存器
	inline void printReg(SIZE_T dwReg, WORD color);
	//打印标志位
	inline void printEflag(DWORD dwFlag, WORD color);
public:
	//显示汇编
	void showAsm(SIZE_T Addr,
		const WCHAR* ShowOpc,
		const WCHAR* pszDiAsm,
		const WCHAR* pszCom,
		const WCHAR* pszLineHeader = L"  "
	);
	//显示寄存器
	void showReg(const CONTEXT& ct);
	//显示内存
	void showMem(SIZE_T virtualAddress, const LPBYTE lpBuff, int nSize, DWORD dwShowFlag);
	//显示栈信息
	void showStack(SIZE_T virtualAddress, const LPBYTE lpBuff, int nSize);
	//显示断点列表
	void showBreakPointList(list<BPObject*>::const_iterator beginItr, list<BPObject*>::const_iterator endItr);
};