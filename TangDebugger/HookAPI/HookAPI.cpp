// HookAPI.cpp : 定义控制台应用程序的入口点。
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
		printf("正在被调试");
	}


    return 0;
}

