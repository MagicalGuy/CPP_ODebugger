#pragma once
#include "stdafx.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <Windows.h>
#include <DbgHelp.h>

////��ʾԴ�ļ���ָ������
void DisplaySourceLines(LPCTSTR sourceFile, int lineNum, unsigned int address, int after, int before);
//��ʾԴ�ļ��е�һ��
void DisplayLine(LPCTSTR sourceFile, const std::wstring& line, int lineNumber, BOOL isCurLine);
//�ڱ�׼��������16����ֵ
void PrintHex(unsigned int value, BOOL hasPrefix);