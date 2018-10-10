#include "stdafx.h"
#include "BPTF.h"
#include "RegStruct.h"
#include "Expression.h"

BPTF::BPTF(DbgObject& dbgObj, bool bIsTFBP)
	:BPObject(dbgObj)
	, m_bIsUserBP(bIsTFBP)
{
	//Install();
}

BPTF::~BPTF()
{
	Remove();
}


bool BPTF::Install()
{
	// ����TF��־λ����1����

	CONTEXT	ct = { CONTEXT_CONTROL };
	if (!m_dbgObj.GetRegInfo(ct))
		return false;

	PEFLAGS pEflags = (PEFLAGS)&ct.EFlags;
	pEflags->TF = 1;

	m_bOnce = m_condition.IsEmpty();
	return m_dbgObj.SetRegInfo(ct);
}


bool BPTF::Remove()
{
	return true;
}

bool BPTF::IsHit()const
{
	// ��¼���д���
	// �ж��Ƿ��б���ʽ
	if (!m_condition.IsEmpty()) {
		Expression exp(&m_dbgObj);
		return exp.getValue(m_condition) != 0;
	}

	// ���TF�ϵ��������ָ������ϵ��,��ô��Զ����false

	return m_bIsUserBP && m_bOnce;
}

bool BPTF::IsMe(const EXCEPTION_DEBUG_INFO& ExcDebInf)const
{
	// �ж϶ϵ������Ƿ�ƥ����
	if (ExcDebInf.ExceptionRecord.ExceptionCode != EXCEPTION_SINGLE_STEP)
		return false;

	// ��ȡ���ԼĴ���
	CONTEXT ct = { CONTEXT_DEBUG_REGISTERS };
	if (!GetThreadContext(m_dbgObj.m_hCurrThread, &ct))
		return false;

	PDBG_REG6 pDr6 = (PDBG_REG6)&ct.Dr6;
	// �жϵ��ԼĴ����Ƿ���ֵ
	return !(pDr6->B0 || pDr6->B1 || pDr6->B2 || pDr6->B3);
}

E_BPType BPTF::Type()const
{
	return breakpointType_tf;
}

bool BPTF::NeedRemove() const
{
	return m_bOnce;
}

//תΪ�û��ϵ�
void BPTF::ConverToUserBreakpoint()
{
	Install();
	m_bIsUserBP = true;
}