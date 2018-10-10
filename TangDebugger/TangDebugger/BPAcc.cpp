#include "stdafx.h"
/**
* 这个cpp的代码未经过完全测试
*/

#include "BPAcc.h"
#include "Expression.h"
#include "Pectrl.h"

BPAcc::BPAcc(DbgObject& dbgObj, uaddr uAddr, E_BPType eType, uint uLen)
	:BPObject(dbgObj)
	, m_eType(eType)
	, m_uLen(uLen)
	, m_oldProtect1()
	, m_oldProtect2()
	, m_currentHitAddress()
	, m_currentHitAccType()
{
	m_uAddress = uAddr;
	//Install();
}

BPAcc::~BPAcc()
{
	Remove();
}

//内存断点安装
bool BPAcc::Install()
{
	if (m_uLen >= 0x1000)
		return false;
	uaddr uPageBase = (m_uAddress & 0xFFFFF000);

	DWORD	dwTemp = 0;
	if (uPageBase > ToAligentSize(m_uAddress, 0x1000))
	{
		dwTemp = VirtualProtectEx(m_dbgObj.m_hCurrProcess,
			(LPVOID)uPageBase,
			0x1000,
			PAGE_NOACCESS,
			&m_oldProtect2
		);
	}

	// 设定断点不能超出两个分页
	dwTemp = VirtualProtectEx(m_dbgObj.m_hCurrProcess,
		(LPVOID)m_uAddress,
		m_uLen,
		PAGE_NOACCESS,
		&m_oldProtect1
	);
	m_bOnce = false;
	return dwTemp == TRUE;
	

	//PAGE_READONLY： 该区域为只读。如果应用程序试图访问区域中的页的时候，将会被拒绝访
	//PAGE_READWRITE 区域可被应用程序读写
	//PAGE_EXECUTE： 区域包含可被系统执行的代码。试图读写该区域的操作将被拒绝。
	//PAGE_EXECUTE_READ ：区域包含可执行代码，应用程序可以读该区域。
	//PAGE_EXECUTE_READWRITE： 区域包含可执行代码，应用程序可以读写该区域。
	//PAGE_GUARD： 区域第一次被访问时进入一个STATUS_GUARD_PAGE异常，这个标志要和其他保护标志合并使用，表明区域被第一次访问的权限
	//PAGE_NOACCESS： 任何访问该区域的操作将被拒绝
	//PAGE_NOCACHE： RAM中的页映射到该区域时将不会被微处理器缓存（cached)

}


bool BPAcc::Remove()
{
	uaddr uPageBase = (m_uAddress & 0xFFFFF000);

	DWORD	dwTemp = 0;
	DWORD	dwOldProtect = 0;
	if (uPageBase > ToAligentSize(m_uAddress, 0x1000))
	{
		dwTemp = VirtualProtectEx(m_dbgObj.m_hCurrProcess,
			(LPVOID)uPageBase,
			0x1000,
			m_oldProtect2,
			&dwOldProtect
		);
	}

	// 设定断点的时候，不能超出两个分页
	dwTemp = VirtualProtectEx(m_dbgObj.m_hCurrProcess,
		(LPVOID)m_uAddress,
		m_uLen,
		m_oldProtect1,
		&dwOldProtect
	);
	return dwTemp == TRUE;
}

bool BPAcc::IsHit()const
{
	// 判断是否含有表达式
	if (m_currentHitAddress != m_uAddress)
		return false;

	switch (m_eType)
	{
	case breakpointType_acc_r:
		if (m_currentHitAccType != 0)
			return false;
		break;
	case breakpointType_acc_w:
		if (m_currentHitAccType != 1)
			return false;
		break;
	case breakpointType_acc_e:
		if (m_currentHitAccType != 8)
			return false;
		break;
	}


	if (!m_condition.IsEmpty()) {
		Expression exp(&m_dbgObj);
		return exp.getValue(m_condition) != 0;
	}
	return true;
}

bool BPAcc::IsMe(const EXCEPTION_DEBUG_INFO& ExcDebInf)const
{
	*(DWORD*)&m_currentHitAccType = ExcDebInf.ExceptionRecord.ExceptionInformation[0];
	*(DWORD*)&m_currentHitAddress = ExcDebInf.ExceptionRecord.ExceptionInformation[1];

	// 判断发生异常的地址
	if (ExcDebInf.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		DWORD dwPageBase = (m_uAddress & 0xFFFFF000);
		if (m_currentHitAddress >= dwPageBase && m_currentHitAddress <= m_uAddress + m_uLen)
			return true;
	}
	return false;
}

E_BPType BPAcc::Type()const
{
	return m_eType;
}


bool BPAcc::NeedRemove() const
{
	return m_bOnce;
}
