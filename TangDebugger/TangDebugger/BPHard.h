#pragma once
#include "BPObject.h"

class BPHard : public BPObject
{
	uint			m_uLen;
	E_BPType		m_eType;
	uint			m_uDbgRegNum;
public:
	BPHard(DbgObject& dbgObj, uaddr uAddress, E_BPType eType, uint uLen);
	virtual ~BPHard();

	// 插入断点
	virtual bool Install();
	// 移除断点
	virtual bool Remove();

	// 返回断点的类型
	virtual	E_BPType Type()const;
	// 判断断点是否是自己
	virtual bool IsMe(const EXCEPTION_DEBUG_INFO& ExcDebInf)const;
};
