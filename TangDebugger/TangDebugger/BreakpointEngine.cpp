#include "stdafx.h"
#pragma execution_character_set("utf-8")
#include "BreakpointEngine.h"
#include "BPSoft.h"
#include "BPHard.h"
#include "BPAcc.h"

BreakpointEngine::BreakpointEngine()
	:m_pRecoveryBp(0)
{
}

BreakpointEngine::~BreakpointEngine()
{
	Clear();
}

//查找断点 返回list的一个迭代器
BpItr BreakpointEngine::FindBreakpoint(const EXCEPTION_DEBUG_INFO& ExceptionInfo)
{
	BPObject* pBp = nullptr;
	for (BpItr i = m_bpList.begin();
		i != m_bpList.end();
		++i)
	{
		// 利用断点对象自己提供的方法来判断异常信息是否是由该断点产生的
		if ((*i)->IsMe(ExceptionInfo))
			return i;
	}
	return m_bpList.end();
}


//根据提供的地址和类型查找断点,返回断点对象
BPObject* BreakpointEngine::FindBreakpoint(uaddr uAddress, E_BPType eType)
{
	for (auto& i : m_bpList)
	{
		// 判断地址是否一致,判断类型是否一致
		if (i->GetAddress() == uAddress && i->Type() == eType)
			return i;
	}
	return nullptr;
}


//修复异常
bool BreakpointEngine::FixException(BpItr FindItr)
{
	BPObject* pBp = *FindItr;

	// 从被调试进程中移除断点(使断点失效)
	pBp->Remove();

	// 获取断点是否被命中,内部会有一些条件表达式,
	// 如果条件表达式为true才被命中,否则不被命中
	bool bHit = pBp->IsHit();

	// 判断断点是否需要从断点列表中移除
	if (pBp->NeedRemove())
	{
		// 释放空间
		delete pBp;
		// 从断点表中删除断点记录
		m_bpList.erase(FindItr);
	}
	else // 没有被命中.
	{
		// tf断点作用：
		//  1. 作为恢复其他功能断点而被设下的tf断点
		//  2. 用户单步时设下的tf断点.
		// 如果tf断点重复,有一个功能断点需要修复
		if (pBp->Type() == breakpointType_tf)
		{
			pBp->Install();
			return bHit;
		}

		// 因为断点已经被移除, 断点已经失效,因此,需要恢复断点的有效性
		// 将断点放入待恢复断点表中
		m_pRecoveryBp = pBp;

		// 插入tf断点,触发一个异常,用于恢复失效的断点
		BPObject *pTf = new BPTF(*this, false);
		pTf->Install();
		m_bpList.push_front(pTf);
	}

	// 返回断点是否被命中
	return bHit;
}

// 重新安装断点
bool BreakpointEngine::ReInstallBreakpoint()
{
	if (m_pRecoveryBp == nullptr)
		return false;
	m_pRecoveryBp->Install();
	m_pRecoveryBp = nullptr;
	return true;
}

//添加断点到断点列表中
BPObject* BreakpointEngine::AddBreakPoint(uaddr uAddress, E_BPType eType, uint uLen)
{
	// 添加断点
	BPObject	*pBp = nullptr;

	// 判断是否含有重复断点
	pBp = CheckRepeat(uAddress, eType);
	if (pBp != nullptr)
	{
		// 判断是否有重复的TF断点
		if (pBp->Type() != breakpointType_tf)
			return nullptr;

		// 如果重复的断点是TF断点,则需要将TF断点转换
		// 转换成用户断点（否则无法断在用户界面上）
		((BPTF*)pBp)->ConverToUserBreakpoint();
		return pBp;
	}

	if (eType == breakpointType_tf)// TF断点
		pBp = new BPTF(*this, true);
	else if (eType == breakpointType_soft) // 软件断点
		pBp = (new BPSoft(*this, uAddress));
	else if (eType >= breakpointType_acc && eType <= breakpointType_acc_rw)//内存访问断点
		pBp = (new BPAcc(*this, uAddress, eType, uLen));
	else if (eType >= breakpointType_hard && eType <= breakpointType_hard_rw)// 硬件断点
		pBp = (new BPHard(*this, uAddress, eType, uLen));
	else
		return nullptr;
	if (pBp->Install() == false)
	{
		delete pBp;
		return false;
	}

	// 将断点插入到断点链表中
	m_bpList.push_front(pBp);
	return pBp;
}

BPObject* BreakpointEngine::AddBreakPoint(const char* pszApiName)
{
	// 查找API的地址
	uaddr address = FindApiAddress(m_hCurrProcess, pszApiName);
	if (address == 0)
		return nullptr;
	// 添加一个软件断点
	return AddBreakPoint(address, breakpointType_soft);
}


//清空断点列表
void BreakpointEngine::Clear()
{
	for (auto& i : m_bpList)
	{
		i->Remove();
		delete i;
	}
	m_bpList.clear();
	m_pRecoveryBp = nullptr;
}



//移除断点
bool BreakpointEngine::DeleteBreakpoint(uint uIndex)
{
	if (uIndex >= m_bpList.size())
		return false;

	for (BpItr i = m_bpList.begin(); i != m_bpList.end(); ++i)
	{
		if (uIndex-- == 0)
		{
			if (m_pRecoveryBp == *i)
				m_pRecoveryBp = nullptr;

			delete *i;
			m_bpList.erase(i);
			return true;
		}
	}
	return false;
}


//设置表达式条件
void BreakpointEngine::SetExp(BPObject* pBp, const CStringA& strExp)
{
	if (pBp != nullptr)
		pBp->SetCondition(strExp);
}

//设置一次性断点条件
void BreakpointEngine::SetOnce(BPObject* pBp, bool bOnce)
{
	if (pBp != nullptr)
		pBp->SetCondition(bOnce);
}

//判断是否是一个无效的迭代器
bool BreakpointEngine::IsInvalidIterator(const BpItr& itr)const
{
	return itr == m_bpList.end();
}

//获取断点列表的迭代器开始位置
list<BPObject*>::const_iterator BreakpointEngine::GetBPListBegin()const
{
	return m_bpList.begin();
}


//获取断点列表的迭代器结束位置
list<BPObject*>::const_iterator BreakpointEngine::GetBPListEnd() const
{
	return m_bpList.end();
}


//检查断点是否重复
BPObject* BreakpointEngine::CheckRepeat(uaddr uAddress, E_BPType eType)
{
	for (auto& i : m_bpList)
	{
		// 一次性断点也视为重复
		if (i->GetAddress() == uAddress && i->m_bOnce != true)  return i;

	}
	return nullptr;
}
