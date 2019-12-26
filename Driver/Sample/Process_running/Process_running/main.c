#include "ntddk.h"
#define SYSTEMPROCESSINFORMATION 5
//处理进程信息，需要用到这两个结构体
typedef struct _SYSTEM_THREADS
{
	LARGE_INTEGER           KernelTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           CreateTime;
	ULONG                   WaitTime;
	PVOID                   StartAddress;
	CLIENT_ID               ClientIs;
	KPRIORITY               Priority;
	KPRIORITY               BasePriority;
	ULONG                   ContextSwitchCount;
	ULONG                   ThreadState;
	KWAIT_REASON            WaitReason;
}SYSTEM_THREADS, * PSYSTEM_THREADS;

//进程信息结构体  
typedef struct _SYSTEM_PROCESSES
{
	ULONG                           NextEntryDelta;    //链表下一个结构和上一个结构的偏移
	ULONG                           ThreadCount;
	ULONG                           Reserved[6];
	LARGE_INTEGER                   CreateTime;
	LARGE_INTEGER                   UserTime;
	LARGE_INTEGER                   KernelTime;
	UNICODE_STRING                  ProcessName;     //进程名字
	KPRIORITY                       BasePriority;
	ULONG                           ProcessId;      //进程的pid号
	ULONG                           InheritedFromProcessId;
	ULONG                           HandleCount;
	ULONG                           Reserved2[2];
	VM_COUNTERS                     VmCounters;
	IO_COUNTERS                     IoCounters; //windows 2000 only  
	struct _SYSTEM_THREADS          Threads[1];
}SYSTEM_PROCESSES, * PSYSTEM_PROCESSES;

//声明ZqQueryAyatemInformation
NTSTATUS ZwQuerySystemInformation(
	IN ULONG SystemInformationClass,  //处理进程信息,只需要处理类别为5的即可
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength
);


NTSTATUS PsProcessList()
{
	NTSTATUS nStatus;
	ULONG retLength;  //缓冲区长度
	PVOID pProcInfo;
	PSYSTEM_PROCESSES pProcIndex;
	//调用函数，获取进程信息
	nStatus = ZwQuerySystemInformation(
		SYSTEMPROCESSINFORMATION,   //获取进程信息,宏定义为5
		NULL,
		0,
		&retLength  //返回的长度，即为我们需要申请的缓冲区的长度
	);
	if (!retLength)
	{
		DbgPrint("ZwQuerySystemInformation error!\n");
		return nStatus;
	}
	DbgPrint("retLength =  %u\n", retLength);
	//申请空间
	pProcInfo = ExAllocatePool(NonPagedPool, retLength);
	if (!pProcInfo)
	{
		DbgPrint("ExAllocatePool error!\n");
		return STATUS_UNSUCCESSFUL;
	}
	nStatus = ZwQuerySystemInformation(
		SYSTEMPROCESSINFORMATION,   //获取进程信息,宏定义为5
		pProcInfo,
		retLength,
		&retLength
	);
	if (NT_SUCCESS(nStatus)/*STATUS_INFO_LENGTH_MISMATCH == nStatus*/)

	{
		pProcIndex = (PSYSTEM_PROCESSES)pProcInfo;
		//第一个进程应该是 pid 为 0 的进程
		if (pProcIndex->ProcessId == 0)
			DbgPrint("PID 0 System Idle Process\n");
		//循环打印所有进程信息,因为最后一天进程的NextEntryDelta值为0，所以先打印后判断
		do
		{
			pProcIndex = (PSYSTEM_PROCESSES)((char*)pProcIndex + pProcIndex->NextEntryDelta);
			//进程名字字符串处理，防止打印时，出错
			if (pProcIndex->ProcessName.Buffer == NULL)
				pProcIndex->ProcessName.Buffer = L"NULL";
			DbgPrint("ProcName:  %-20ws     pid:  %u\n", pProcIndex->ProcessName.Buffer, pProcIndex->ProcessId);
		} while (pProcIndex->NextEntryDelta != 0);
	}
	else
	{
		DbgPrint("error code : %u!!!\n", nStatus);
	}
	ExFreePool(pProcInfo);
	return nStatus;
}
//卸载驱动
VOID OnUnload(IN PDRIVER_OBJECT driver)
{
	DbgPrint("Driver has been unloaded!!!\n");
}
//驱动入口函数
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driver, IN PUNICODE_STRING reg_path)
{
	PsProcessList();
	driver->DriverUnload = OnUnload;
	return STATUS_SUCCESS;
}