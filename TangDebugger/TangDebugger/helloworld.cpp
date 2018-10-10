// helloworld.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include<iostream>
using namespace std;

int main()
{
	for (int i = 0; i < 10; ++i) {
		printf("hello world\n");

		/*	__try {
				*(int*)0 = 0;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				printf("__except\n");
			}*/
	}
	int count = 0;
	count++;

	if (count==1) {
		cout << count;
	
	}


	int a = 1 + 1;
	int b = 2 + 3;

	printf("%d\n",a+b);

	printf("exit\n");
	system("pause");
    return 0;
}

