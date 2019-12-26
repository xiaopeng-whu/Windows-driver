#include <ntddk.h>
#define SYSTEMPROCESSINFORMATION 5

// 定义一个值为0x800的控制码
#define IOCTL_KILL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
// 定义设备名和符号名
#define DEVICE_NAME L"\\Device\\KillDevice"
#define SYM_LINK_NAME L"\\??\\Kill"

PDEVICE_OBJECT pDevice;
UNICODE_STRING DeviceName;
UNICODE_STRING SymLinkName;

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

// 驱动卸载函数
NTSTATUS DriverUnload(PDRIVER_OBJECT Driver)
{
	NTSTATUS status;

	// 删除符号和设备
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(pDevice);
	DbgPrint("This Driver Is Unloading...\n");
	return STATUS_SUCCESS;
}

// 设备共用函数
NTSTATUS DeviceApi(PDEVICE_OBJECT Device, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	// I/O请求处理完毕
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 杀死进程函数
BOOLEAN KillProcess(LONG pid)
{
	HANDLE ProcessHandle;
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID Cid;

	// 初始化ObjectAttributes和Cid
	InitializeObjectAttributes(&ObjectAttributes, 0, 0, 0, 0);
	Cid.UniqueProcess = (HANDLE)pid;
	Cid.UniqueThread = 0;
	// 打开进程句柄
	status = ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttributes, &Cid);
	if (NT_SUCCESS(status))
	{
		DbgPrint("Open Process %d Successful!\n", pid);
		// 结束进程
		ZwTerminateProcess(ProcessHandle, status);
		// 关闭句柄
		ZwClose(ProcessHandle);
		return TRUE;
	}
	DbgPrint("Open Process %d Failed!\n", pid);
	return FALSE;
}

// 设备I/O控制函数
NTSTATUS DeviceIoctl(PDEVICE_OBJECT Device, PIRP pIrp)
{
	NTSTATUS status;
	// 获取IRP消息的数据
	PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(pIrp);
	// 获取传过来的控制码
	ULONG CODE = irps->Parameters.DeviceIoControl.IoControlCode;
	ULONG info = 0;


	switch (CODE)
	{
		// 若控制等于我们约定的IOCTL_KILL（0x800）
	case IOCTL_KILL:
	{
		DbgPrint("Enter the IO \n");
		// 获取要杀死的进程的PID
		LONG pid = *(PLONG)(pIrp->AssociatedIrp.SystemBuffer);
		DbgPrint("Get PID : %d\n", pid);
		if (KillProcess(pid))
		{
			DbgPrint("Kill Successful\n");
			DbgPrint("---------------\n");
		}
		else
		{
			DbgPrint("Kill Failed\n");
		}
		status = STATUS_SUCCESS;
		break;
	}
	default:
		DbgPrint("Unknown CODE!\n");
		status = STATUS_UNSUCCESSFUL;
		break;
	}

	PsProcessList();  //这里貌似还会显示刚关闭的进程
	// I/O请求处理完毕
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

// 驱动入口函数
NTSTATUS DriverEntry(PDRIVER_OBJECT Driver, PUNICODE_STRING RegPath)
{
	NTSTATUS status;

	// 注册驱动卸载函数
	Driver->DriverUnload = DriverUnload;

	// 通过循环将设备创建、读写、关闭等函数设置为通用的DeviceApi
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		Driver->MajorFunction[i] = DeviceApi;
	}
	// 单独把控制函数设置为DeviceIoctl
	Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoctl;

	// 将设备名转换为Unicode字符串
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
	PsProcessList();
	// 创建设备对象
	status = IoCreateDevice(Driver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, NULL, &pDevice);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create Device Faild!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// 将符号名转换为Unicode字符串
	RtlInitUnicodeString(&SymLinkName, SYM_LINK_NAME);
	// 将符号与设备关联
	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create SymLink Faild!\n");
		IoDeleteDevice(pDevice);
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("Initialize Success\n");

	// 设置pDevice以缓冲区方式读取
	pDevice->Flags = DO_BUFFERED_IO;

	return STATUS_SUCCESS;
}