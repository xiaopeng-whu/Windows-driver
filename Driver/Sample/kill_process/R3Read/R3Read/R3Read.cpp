#include "pch.h"
#include <stdio.h>
#include <tchar.h> 
#include <windows.h>
#include <winioctl.h>

// 定义跟一个约定的控制码0x800
#define IOCTL_KILL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 主函数
int _tmain(int argc, _TCHAR* argv[])
{
	// 打开符号（设备句柄）
	HANDLE hDevice = CreateFile(L"\\\\.\\Kill", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed To Obtain Device Handle!");
		return -1;
	}

	DWORD len = 0;
	UCHAR buffer[20];
	memset(buffer, 0x00, 20);

	long pid = 0;

	// 接收用户输入的pid
	printf("Please Enter the Process ID : ");
	scanf_s("%d", &pid);

	// 发送pid给驱动
	BOOL status = DeviceIoControl(hDevice, IOCTL_KILL, &pid, 4, buffer, 20, &len, NULL);

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
