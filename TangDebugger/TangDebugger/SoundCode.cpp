#include"stdafx.h"
#include "SoundCode.h"
#include "TangDebugger.h"

#include <iostream>
#include <iomanip>


//在标准输出上输出16进制值。
void PrintHex(unsigned int value, BOOL hasPrefix) {

	std::wcout << std::hex << std::uppercase;

	if (hasPrefix == TRUE) {
		std::wcout << TEXT("0x");
	}

	std::wcout << std::setw(8) << std::setfill(TEXT('0')) << value << std::dec << std::nouppercase << std::flush;
}




//显示源文件中指定的行
void DisplaySourceLines(LPCTSTR sourceFile, int lineNum, unsigned int address, int after, int before) {

	std::wcout << std::endl;

	std::wifstream inputStream(sourceFile);

	if (inputStream.fail() == true) {

		std::wcout << TEXT("Open source file failed.") << std::endl
			<< TEXT("Path: ") << sourceFile << std::endl;
		return;
	}

	inputStream.imbue(std::locale("chs", std::locale::ctype));

	int curLineNumber = 1;

	//计算从第几行开始输出
	int startLineNumber = lineNum - before;
	if (startLineNumber < 1) {
		startLineNumber = 1;
	}

	std::wstring line;

	//跳过不需要显示的行
	while (curLineNumber < startLineNumber) {

		std::getline(inputStream, line);
		++curLineNumber;
	}

	//输出开始行到当前行之间的行
	while (curLineNumber < lineNum) {

		std::getline(inputStream, line);
		DisplayLine(sourceFile, line, curLineNumber, FALSE);
		++curLineNumber;
	}

	//输出当前行
	getline(inputStream, line);
	DisplayLine(sourceFile, line, curLineNumber, TRUE);
	++curLineNumber;

	//输出当前行到最后行之间的行
	int lastLineNumber = lineNum + after;
	while (curLineNumber <= lastLineNumber) {

		if (!getline(inputStream, line)) {
			break;
		}

		DisplayLine(sourceFile, line, curLineNumber, FALSE);
		++curLineNumber;
	}

	inputStream.close();
}



//显示源文件中的一行。
void DisplayLine(LPCTSTR sourceFile, const std::wstring& line, int lineNumber, BOOL isCurLine) {

	if (isCurLine == TRUE) {
		std::wcout << TEXT("=>");
	}
	else {
		std::wcout << TEXT("  ");
	}

	LONG displacement;
	IMAGEHLP_LINE64 lineInfo = { 0 };
	lineInfo.SizeOfStruct = sizeof(lineInfo);

	/*
	if (SymGetLineFromName64(
		Hprocess,
		NULL,
		(PCSTR)sourceFile,
		lineNumber,
		&displacement,
		&lineInfo) == FALSE) {

		std::wcout << TEXT("SymGetLineFromName64 failed: ") << GetLastError() << std::endl;
		return;
	}
	*/

	SymGetLineFromName64(
		Hprocess,
		NULL,
		(PCSTR)sourceFile,
		lineNumber,
		&displacement,
		&lineInfo);

	std::wcout << std::setw(4) << std::setfill(TEXT(' ')) << lineNumber << TEXT("  ");

	if (displacement == 0) {

		PrintHex((unsigned int)lineInfo.Address, FALSE);
	}
	else {

		std::wcout << std::setw(8) << TEXT(" ");
	}

	std::wcout << TEXT("  ") << line << std::endl;
}