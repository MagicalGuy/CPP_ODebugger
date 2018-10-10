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
//设置断点条件
void BPObject::SetCondition(const  char* strConditoion)
{
	m_condition = strConditoion;
	m_bOnce = false;
}
//设置一次性断点
void BPObject::SetCondition(bool bOnce)
{
	m_bOnce = bOnce;
	m_condition.Empty();
}

//获取地址
uaddr BPObject::GetAddress()const
{
	return m_uAddress;
}

//获取断点条件
const char* BPObject::GetCondition() const
{
	if (m_condition.IsEmpty())
		return nullptr;
	return m_condition;
}


//判断是否需要删除
bool BPObject::NeedRemove() const
{
	return m_bOnce;
}

//判断是否命中断点
bool BPObject::IsHit() const
{
	if (!m_condition.IsEmpty()) {
		Expression exp(&m_dbgObj);
		return exp.getValue(m_condition) != 0;
	}
	return true;
}

