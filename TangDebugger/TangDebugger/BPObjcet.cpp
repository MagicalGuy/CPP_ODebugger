#include "stdafx.h"
#include"BPObject.h"

#include "BPObject.h"

#include "Expression.h"

BPObject::BPObject(DbgObject& dbgObj):m_dbgObj(dbgObj), m_bOnce()
{
}

BPObject::~BPObject()
{
}
//���öϵ�����
void BPObject::SetCondition(const  char* strConditoion)
{
	m_condition = strConditoion;
	m_bOnce = false;
}
//����һ���Զϵ�
void BPObject::SetCondition(bool bOnce)
{
	m_bOnce = bOnce;
	m_condition.Empty();
}

//��ȡ��ַ
uaddr BPObject::GetAddress()const
{
	return m_uAddress;
}

//��ȡ�ϵ�����
const char* BPObject::GetCondition() const
{
	if (m_condition.IsEmpty())
		return nullptr;
	return m_condition;
}


//�ж��Ƿ���Ҫɾ��
bool BPObject::NeedRemove() const
{
	return m_bOnce;
}

//�ж��Ƿ����жϵ�
bool BPObject::IsHit() const
{
	if (!m_condition.IsEmpty()) {
		Expression exp(&m_dbgObj);
		return exp.getValue(m_condition) != 0;
	}
	return true;
}

