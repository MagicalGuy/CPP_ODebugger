// HookAPI.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include<Windows.h>
#include<iostream>
using namespace std;

int main()
{
	HANDLE hToken;
	bool flag = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	cout << "haha" << endl;
	cin.get();


	bool isdebuggeron;
	isdebuggeron = IsDebuggerPresent();
	if (isdebuggeron)
	{
		printf("���ڱ�����");
	}


    return 0;
}

