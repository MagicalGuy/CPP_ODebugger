#pragma once
#include <atlstr.h>
#include<iostream>
#include<string>
using namespace std;
/**
* HOOKEngine
*
*/
class HOOKEngine
{
	CStringA	m_functionName;
	void*		m_pAddress;
	char		m_uData[8];

	char*		m_MyFunctionCodeAddr;
	char*		m_pFuntionData;
public:
	HOOKEngine();
	~HOOKEngine();

};

