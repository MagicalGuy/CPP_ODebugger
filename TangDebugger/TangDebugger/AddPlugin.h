#pragma once
#include "stdafx.h"
//#include<afxwin.h>
#include<windows.h>


typedef struct _Plugin_T
{
	//
	// 插件的名称
	//
	char  szPluginName[256];
	//
	// DLL的实例句柄
	//
	HINSTANCE hPlugin;
	//HMODULE hPlugin;
	//
	// 插件中处理命令行的函数
	void(*pfn_get_cmd_line)(char* cmdline);
} PLUGIN_T, *LPPLUGIN_T;


//现在还需要一个容器结构将插件的信息组织起来, 方便调用, 如下:
struct _Plugin_Vector{
	PLUGIN_T  Plugins[50];
	//
	// 插件计数
	//
	unsigned int nCount;
} g_pv;


//预设50个插件
//有了上面两个结构, load_plugin()就可以根据用户的输入来载入相应的插件.代码如下:
int load_plugin(char* szFileName)
{
	//
	// 取得dll的实例句柄
	//

	CString str = CString(szFileName);
	USES_CONVERSION;
	LPCWSTR szFileName1 = A2CW(W2A(str));
	str.ReleaseBuffer();


	g_pv.Plugins[g_pv.nCount].hPlugin = LoadLibrary(szFileName1);
	if (g_pv.Plugins[g_pv.nCount].hPlugin == NULL)
	{
		fprintf(stderr, "LoadLibrary() failed. --err: %d/n", GetLastError());
		return -1;
	}
	//
	// 取得插件名.由于szFileName包含文件的扩展名(.dll),
	// 所以要将扩展名去掉
	//
	char  szPluginName[256];
	char* p = 0;
	strcpy_s(szPluginName, szFileName);
	p = strchr(szPluginName, '.');
	//*p = '/0';
	strcpy_s(g_pv.Plugins[g_pv.nCount].szPluginName, szPluginName);
	//
	// 取得插件中处理命令行的函数指针(插件的导出函数)
	//
	g_pv.Plugins[g_pv.nCount].pfn_get_cmd_line = (void(*)(char*))GetProcAddress(g_pv.Plugins[g_pv.nCount].hPlugin, "funTest");
	printf("Plugin %s Load Ok!/n", g_pv.Plugins[g_pv.nCount].szPluginName);


	typedef void(*PFUN)();
	PFUN MyFun = (PFUN)g_pv.Plugins[g_pv.nCount].pfn_get_cmd_line;
	MyFun();

	g_pv.nCount++;
	return 0;
}
//这样就可以通过.load <dll name>来加载所需要的插件, 如:.load hack.dll(需要带扩展名).
//主程序中由get_cmd_line()来负责对输入的命令进行处理, 代码如下 :
void get_cmd_line() {
	char cmdline[256];
	printf("请直接输入插件名称# ");
	gets_s(cmdline);
	while (_stricmp(cmdline, ".quit"))
	{
		if (!_strnicmp(cmdline, ".load", 5))
		{
			char* p = strchr(cmdline, ' ');
			if (p != NULL)
				load_plugin(p + 1);
			else
				printf("显示不了插件");
		}
		else
			printf("成功");
		gets_s(cmdline);

	}
}