#include <ntddk.h>
#define SYSTEMPROCESSINFORMATION 5

// ����һ��ֵΪ0x800�Ŀ�����
#define IOCTL_KILL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
// �����豸���ͷ�����
#define DEVICE_NAME L"\\Device\\KillDevice"
#define SYM_LINK_NAME L"\\??\\Kill"

PDEVICE_OBJECT pDevice;
UNICODE_STRING DeviceName;
UNICODE_STRING SymLinkName;

//���������Ϣ����Ҫ�õ��������ṹ��
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

//������Ϣ�ṹ��  
typedef struct _SYSTEM_PROCESSES
{
	ULONG                           NextEntryDelta;    //������һ���ṹ����һ���ṹ��ƫ��
	ULONG                           ThreadCount;
	ULONG                           Reserved[6];
	LARGE_INTEGER                   CreateTime;
	LARGE_INTEGER                   UserTime;
	LARGE_INTEGER                   KernelTime;
	UNICODE_STRING                  ProcessName;     //��������
	KPRIORITY                       BasePriority;
	ULONG                           ProcessId;      //���̵�pid��
	ULONG                           InheritedFromProcessId;
	ULONG                           HandleCount;
	ULONG                           Reserved2[2];
	VM_COUNTERS                     VmCounters;
	IO_COUNTERS                     IoCounters; //windows 2000 only  
	struct _SYSTEM_THREADS          Threads[1];
}SYSTEM_PROCESSES, * PSYSTEM_PROCESSES;

//����ZqQueryAyatemInformation
NTSTATUS ZwQuerySystemInformation(
	IN ULONG SystemInformationClass,  //���������Ϣ,ֻ��Ҫ�������Ϊ5�ļ���
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength
);

NTSTATUS PsProcessList()
{
	NTSTATUS nStatus;
	ULONG retLength;  //����������
	PVOID pProcInfo;
	PSYSTEM_PROCESSES pProcIndex;
	//���ú�������ȡ������Ϣ
	nStatus = ZwQuerySystemInformation(
		SYSTEMPROCESSINFORMATION,   //��ȡ������Ϣ,�궨��Ϊ5
		NULL,
		0,
		&retLength  //���صĳ��ȣ���Ϊ������Ҫ����Ļ������ĳ���
	);
	if (!retLength)
	{
		DbgPrint("ZwQuerySystemInformation error!\n");
		return nStatus;
	}
	DbgPrint("retLength =  %u\n", retLength);
	//����ռ�
	pProcInfo = ExAllocatePool(NonPagedPool, retLength);
	if (!pProcInfo)
	{
		DbgPrint("ExAllocatePool error!\n");
		return STATUS_UNSUCCESSFUL;
	}
	nStatus = ZwQuerySystemInformation(
		SYSTEMPROCESSINFORMATION,   //��ȡ������Ϣ,�궨��Ϊ5
		pProcInfo,
		retLength,
		&retLength
	);
	if (NT_SUCCESS(nStatus)/*STATUS_INFO_LENGTH_MISMATCH == nStatus*/)

	{
		pProcIndex = (PSYSTEM_PROCESSES)pProcInfo;
		//��һ������Ӧ���� pid Ϊ 0 �Ľ���
		if (pProcIndex->ProcessId == 0)
			DbgPrint("PID 0 System Idle Process\n");
		//ѭ����ӡ���н�����Ϣ,��Ϊ���һ����̵�NextEntryDeltaֵΪ0�������ȴ�ӡ���ж�
		do
		{
			pProcIndex = (PSYSTEM_PROCESSES)((char*)pProcIndex + pProcIndex->NextEntryDelta);
			//���������ַ���������ֹ��ӡʱ������
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

// ����ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT Driver)
{
	NTSTATUS status;

	// ɾ�����ź��豸
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(pDevice);
	DbgPrint("This Driver Is Unloading...\n");
	return STATUS_SUCCESS;
}

// �豸���ú���
NTSTATUS DeviceApi(PDEVICE_OBJECT Device, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	// I/O���������
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// ɱ�����̺���
BOOLEAN KillProcess(LONG pid)
{
	HANDLE ProcessHandle;
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID Cid;

	// ��ʼ��ObjectAttributes��Cid
	InitializeObjectAttributes(&ObjectAttributes, 0, 0, 0, 0);
	Cid.UniqueProcess = (HANDLE)pid;
	Cid.UniqueThread = 0;
	// �򿪽��̾��
	status = ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttributes, &Cid);
	if (NT_SUCCESS(status))
	{
		DbgPrint("Open Process %d Successful!\n", pid);
		// ��������
		ZwTerminateProcess(ProcessHandle, status);
		// �رվ��
		ZwClose(ProcessHandle);
		return TRUE;
	}
	DbgPrint("Open Process %d Failed!\n", pid);
	return FALSE;
}

// �豸I/O���ƺ���
NTSTATUS DeviceIoctl(PDEVICE_OBJECT Device, PIRP pIrp)
{
	NTSTATUS status;
	// ��ȡIRP��Ϣ������
	PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(pIrp);
	// ��ȡ�������Ŀ�����
	ULONG CODE = irps->Parameters.DeviceIoControl.IoControlCode;
	ULONG info = 0;


	switch (CODE)
	{
		// �����Ƶ�������Լ����IOCTL_KILL��0x800��
	case IOCTL_KILL:
	{
		DbgPrint("Enter the IO \n");
		// ��ȡҪɱ���Ľ��̵�PID
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

	PsProcessList();  //����ò�ƻ�����ʾ�չرյĽ���
	// I/O���������
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT Driver, PUNICODE_STRING RegPath)
{
	NTSTATUS status;

	// ע������ж�غ���
	Driver->DriverUnload = DriverUnload;

	// ͨ��ѭ�����豸��������д���رյȺ�������Ϊͨ�õ�DeviceApi
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		Driver->MajorFunction[i] = DeviceApi;
	}
	// �����ѿ��ƺ�������ΪDeviceIoctl
	Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoctl;

	// ���豸��ת��ΪUnicode�ַ���
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
	PsProcessList();
	// �����豸����
	status = IoCreateDevice(Driver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, NULL, &pDevice);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create Device Faild!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// ��������ת��ΪUnicode�ַ���
	RtlInitUnicodeString(&SymLinkName, SYM_LINK_NAME);
	// ���������豸����
	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create SymLink Faild!\n");
		IoDeleteDevice(pDevice);
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("Initialize Success\n");

	// ����pDevice�Ի�������ʽ��ȡ
	pDevice->Flags = DO_BUFFERED_IO;

	return STATUS_SUCCESS;
}