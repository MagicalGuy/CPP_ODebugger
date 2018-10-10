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

//���Ҷϵ� ����list��һ��������
BpItr BreakpointEngine::FindBreakpoint(const EXCEPTION_DEBUG_INFO& ExceptionInfo)
{
	BPObject* pBp = nullptr;
	for (BpItr i = m_bpList.begin();
		i != m_bpList.end();
		++i)
	{
		// ���öϵ�����Լ��ṩ�ķ������ж��쳣��Ϣ�Ƿ����ɸöϵ������
		if ((*i)->IsMe(ExceptionInfo))
			return i;
	}
	return m_bpList.end();
}


//�����ṩ�ĵ�ַ�����Ͳ��Ҷϵ�,���ضϵ����
BPObject* BreakpointEngine::FindBreakpoint(uaddr uAddress, E_BPType eType)
{
	for (auto& i : m_bpList)
	{
		// �жϵ�ַ�Ƿ�һ��,�ж������Ƿ�һ��
		if (i->GetAddress() == uAddress && i->Type() == eType)
			return i;
	}
	return nullptr;
}


//�޸��쳣
bool BreakpointEngine::FixException(BpItr FindItr)
{
	BPObject* pBp = *FindItr;

	// �ӱ����Խ������Ƴ��ϵ�(ʹ�ϵ�ʧЧ)
	pBp->Remove();

	// ��ȡ�ϵ��Ƿ�����,�ڲ�����һЩ�������ʽ,
	// ����������ʽΪtrue�ű�����,���򲻱�����
	bool bHit = pBp->IsHit();

	// �ж϶ϵ��Ƿ���Ҫ�Ӷϵ��б����Ƴ�
	if (pBp->NeedRemove())
	{
		// �ͷſռ�
		delete pBp;
		// �Ӷϵ����ɾ���ϵ��¼
		m_bpList.erase(FindItr);
	}
	else // û�б�����.
	{
		// tf�ϵ����ã�
		//  1. ��Ϊ�ָ��������ܶϵ�������µ�tf�ϵ�
		//  2. �û�����ʱ���µ�tf�ϵ�.
		// ���tf�ϵ��ظ�,��һ�����ܶϵ���Ҫ�޸�
		if (pBp->Type() == breakpointType_tf)
		{
			pBp->Install();
			return bHit;
		}

		// ��Ϊ�ϵ��Ѿ����Ƴ�, �ϵ��Ѿ�ʧЧ,���,��Ҫ�ָ��ϵ����Ч��
		// ���ϵ������ָ��ϵ����
		m_pRecoveryBp = pBp;

		// ����tf�ϵ�,����һ���쳣,���ڻָ�ʧЧ�Ķϵ�
		BPObject *pTf = new BPTF(*this, false);
		pTf->Install();
		m_bpList.push_front(pTf);
	}

	// ���ضϵ��Ƿ�����
	return bHit;
}

// ���°�װ�ϵ�
bool BreakpointEngine::ReInstallBreakpoint()
{
	if (m_pRecoveryBp == nullptr)
		return false;
	m_pRecoveryBp->Install();
	m_pRecoveryBp = nullptr;
	return true;
}

//��Ӷϵ㵽�ϵ��б���
BPObject* BreakpointEngine::AddBreakPoint(uaddr uAddress, E_BPType eType, uint uLen)
{
	// ��Ӷϵ�
	BPObject	*pBp = nullptr;

	// �ж��Ƿ����ظ��ϵ�
	pBp = CheckRepeat(uAddress, eType);
	if (pBp != nullptr)
	{
		// �ж��Ƿ����ظ���TF�ϵ�
		if (pBp->Type() != breakpointType_tf)
			return nullptr;

		// ����ظ��Ķϵ���TF�ϵ�,����Ҫ��TF�ϵ�ת��
		// ת�����û��ϵ㣨�����޷������û������ϣ�
		((BPTF*)pBp)->ConverToUserBreakpoint();
		return pBp;
	}

	if (eType == breakpointType_tf)// TF�ϵ�
		pBp = new BPTF(*this, true);
	else if (eType == breakpointType_soft) // ����ϵ�
		pBp = (new BPSoft(*this, uAddress));
	else if (eType >= breakpointType_acc && eType <= breakpointType_acc_rw)//�ڴ���ʶϵ�
		pBp = (new BPAcc(*this, uAddress, eType, uLen));
	else if (eType >= breakpointType_hard && eType <= breakpointType_hard_rw)// Ӳ���ϵ�
		pBp = (new BPHard(*this, uAddress, eType, uLen));
	else
		return nullptr;
	if (pBp->Install() == false)
	{
		delete pBp;
		return false;
	}

	// ���ϵ���뵽�ϵ�������
	m_bpList.push_front(pBp);
	return pBp;
}

BPObject* BreakpointEngine::AddBreakPoint(const char* pszApiName)
{
	// ����API�ĵ�ַ
	uaddr address = FindApiAddress(m_hCurrProcess, pszApiName);
	if (address == 0)
		return nullptr;
	// ���һ������ϵ�
	return AddBreakPoint(address, breakpointType_soft);
}


//��նϵ��б�
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



//�Ƴ��ϵ�
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


//���ñ��ʽ����
void BreakpointEngine::SetExp(BPObject* pBp, const CStringA& strExp)
{
	if (pBp != nullptr)
		pBp->SetCondition(strExp);
}

//����һ���Զϵ�����
void BreakpointEngine::SetOnce(BPObject* pBp, bool bOnce)
{
	if (pBp != nullptr)
		pBp->SetCondition(bOnce);
}

//�ж��Ƿ���һ����Ч�ĵ�����
bool BreakpointEngine::IsInvalidIterator(const BpItr& itr)const
{
	return itr == m_bpList.end();
}

//��ȡ�ϵ��б�ĵ�������ʼλ��
list<BPObject*>::const_iterator BreakpointEngine::GetBPListBegin()const
{
	return m_bpList.begin();
}


//��ȡ�ϵ��б�ĵ���������λ��
list<BPObject*>::const_iterator BreakpointEngine::GetBPListEnd() const
{
	return m_bpList.end();
}


//���ϵ��Ƿ��ظ�
BPObject* BreakpointEngine::CheckRepeat(uaddr uAddress, E_BPType eType)
{
	for (auto& i : m_bpList)
	{
		// һ���Զϵ�Ҳ��Ϊ�ظ�
		if (i->GetAddress() == uAddress && i->m_bOnce != true)  return i;

	}
	return nullptr;
}
