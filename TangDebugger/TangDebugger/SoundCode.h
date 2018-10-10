#pragma once
#include "stdafx.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <Windows.h>
#include <DbgHelp.h>

////显示源文件中指定的行
void DisplaySourceLines(LPCTSTR sourceFile, int lineNum, unsigned int address, int after, int before);
//显示源文件中的一行
void DisplayLine(LPCTSTR sourceFile, const std::wstring& line, int lineNumber, BOOL isCurLine);
//在标准输出上输出16进制值
void PrintHex(unsigned int value, BOOL hasPrefix);