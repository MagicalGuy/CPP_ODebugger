#pragma once
#include <iostream>
#include <iomanip>
#include <windows.h>
#include <fstream>
#include<stdio.h>
#include<string.h>


#define MAX 16*10000
using namespace std;

//��ȡ�ļ���С
DWORD FileSize();
//�ڴ�������ʮ�����Ƶ������ļ�
void HexToFile();
//pe���ݸ���
void ExeTofile();

//�ڴ�����dump���ļ�
void dump(string str);
