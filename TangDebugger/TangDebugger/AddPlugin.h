#pragma once
#include "stdafx.h"
//#include<afxwin.h>
#include<windows.h>


typedef struct _Plugin_T
{
	//
	// ���������
	//
	char  szPluginName[256];
	//
	// DLL��ʵ�����
	//
	HINSTANCE hPlugin;
	//HMODULE hPlugin;
	//
	// ����д��������еĺ���
	void(*pfn_get_cmd_line)(char* cmdline);
} PLUGIN_T, *LPPLUGIN_T;


//���ڻ���Ҫһ�������ṹ���������Ϣ��֯����, �������, ����:
struct _Plugin_Vector{
	PLUGIN_T  Plugins[50];
	//
	// �������
	//
	unsigned int nCount;
} g_pv;


//Ԥ��50�����
//�������������ṹ, load_plugin()�Ϳ��Ը����û���������������Ӧ�Ĳ��.��������:
int load_plugin(char* szFileName)
{
	//
	// ȡ��dll��ʵ�����
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
	// ȡ�ò����.����szFileName�����ļ�����չ��(.dll),
	// ����Ҫ����չ��ȥ��
	//
	char  szPluginName[256];
	char* p = 0;
	strcpy_s(szPluginName, szFileName);
	p = strchr(szPluginName, '.');
	//*p = '/0';
	strcpy_s(g_pv.Plugins[g_pv.nCount].szPluginName, szPluginName);
	//
	// ȡ�ò���д��������еĺ���ָ��(����ĵ�������)
	//
	g_pv.Plugins[g_pv.nCount].pfn_get_cmd_line = (void(*)(char*))GetProcAddress(g_pv.Plugins[g_pv.nCount].hPlugin, "funTest");
	printf("Plugin %s Load Ok!/n", g_pv.Plugins[g_pv.nCount].szPluginName);


	typedef void(*PFUN)();
	PFUN MyFun = (PFUN)g_pv.Plugins[g_pv.nCount].pfn_get_cmd_line;
	MyFun();

	g_pv.nCount++;
	return 0;
}
//�����Ϳ���ͨ��.load <dll name>����������Ҫ�Ĳ��, ��:.load hack.dll(��Ҫ����չ��).
//����������get_cmd_line()������������������д���, �������� :
void get_cmd_line() {
	char cmdline[256];
	printf("��ֱ������������# ");
	gets_s(cmdline);
	while (_stricmp(cmdline, ".quit"))
	{
		if (!_strnicmp(cmdline, ".load", 5))
		{
			char* p = strchr(cmdline, ' ');
			if (p != NULL)
				load_plugin(p + 1);
			else
				printf("��ʾ���˲��");
		}
		else
			printf("�ɹ�");
		gets_s(cmdline);

	}
}