// MyDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "MyDll.h"

void funTest() {
	MessageBox(0, L"我是插件！", 0, MB_OK);
}