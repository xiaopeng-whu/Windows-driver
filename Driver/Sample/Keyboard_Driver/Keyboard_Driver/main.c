//__stdcall
#include<ntddk.h>
#include<Ntddkbd.h>
#include<ntstrsafe.h>
#pragma code_seg("INT")

extern POBJECT_TYPE IoDriverObjectType;
#define KBD_DRIVER_NAME L"\\Driver\\kbdClass"
//���ڼ�ʱ
#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING objectName, ULONG Attributes, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess, POBJECT_TYPE objectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

ULONG keyCount = 0;

//���ô�д����,С����������shift��״̬
#define    S_SHIFT                1
#define    S_CAPS                2
#define    S_NUM                4
static int kb_status = S_NUM;

#define KEY_UP 1 
#define KEY_DOWN 0 

//�����д��������ctrl����ɨ����,��ʵ���Զ���һ��ȫ���̵�ɨ����
#define LCONTROL ((USHORT)0x1D) 
#define CAPS_LOCK ((USHORT)0x3A) 


unsigned char asciiTbl[] = {
	0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x08, 0x09,    //normal
	0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x5B, 0x5D, 0x0D, 0x00, 0x61, 0x73,
	0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3B, 0x27, 0x60, 0x00, 0x5C, 0x7A, 0x78, 0x63, 0x76,
	0x62, 0x6E, 0x6D, 0x2C, 0x2E, 0x2F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
	0x32, 0x33, 0x30, 0x2E,
	0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x08, 0x09,    //caps
	0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x5B, 0x5D, 0x0D, 0x00, 0x41, 0x53,
	0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3B, 0x27, 0x60, 0x00, 0x5C, 0x5A, 0x58, 0x43, 0x56,
	0x42, 0x4E, 0x4D, 0x2C, 0x2E, 0x2F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
	0x32, 0x33, 0x30, 0x2E,
	0x00, 0x1B, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x08, 0x09,    //shift
	0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x7B, 0x7D, 0x0D, 0x00, 0x41, 0x53,
	0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3A, 0x22, 0x7E, 0x00, 0x7C, 0x5A, 0x58, 0x43, 0x56,
	0x42, 0x4E, 0x4D, 0x3C, 0x3E, 0x3F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
	0x32, 0x33, 0x30, 0x2E,
	0x00, 0x1B, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x08, 0x09,    //caps + shift
	0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x7B, 0x7D, 0x0D, 0x00, 0x61, 0x73,
	0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3A, 0x22, 0x7E, 0x00, 0x7C, 0x7A, 0x78, 0x63, 0x76,
	0x62, 0x6E, 0x6D, 0x3C, 0x3E, 0x3F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
	0x32, 0x33, 0x30, 0x2E
};
typedef struct _DEV_EXT
{
	// ����ṹ�Ĵ�С
	ULONG NodeSize;
	// �����豸�����Լ����ɵ��豸��
	PDEVICE_OBJECT pFilterDeviceObject;
	// �󶨵��豸����
	PDEVICE_OBJECT TargetDeviceObject;
	// ��ǰ�ײ��豸����
	PDEVICE_OBJECT LowerDeviceObject;
} DEV_EXT, * PDEV_EXT;

//��д�豸��չ����
VOID initDevExt(PDEV_EXT pDevExt, PDEVICE_OBJECT pFdo, PDEVICE_OBJECT pTopDev, PDEVICE_OBJECT pKbdDeviceObject)
{
	memset(pDevExt, 0, sizeof(pDevExt));
	pDevExt->NodeSize = sizeof(pDevExt);
	pDevExt->LowerDeviceObject = pTopDev;
	pDevExt->pFilterDeviceObject = pFdo;
	pDevExt->TargetDeviceObject = pKbdDeviceObject;

}

NTSTATUS MyAttachDevices(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDRIVER_OBJECT kbdDriver = NULL;
	UNICODE_STRING kbdDriverName;
	PDEVICE_OBJECT pKbdDeviceObject = NULL;
	PDEVICE_OBJECT filterDeviceObject = NULL;
	PDEVICE_OBJECT pTopDev = NULL;
	PDEV_EXT pDevExt = NULL; //�豸��չ

	//��ȡ��������
	RtlInitUnicodeString(&kbdDriverName, KBD_DRIVER_NAME); //�����������ϵͳ��������������̵���������
	status = ObReferenceObjectByName(&kbdDriverName, OBJ_CASE_INSENSITIVE, 0, 0, IoDriverObjectType, KernelMode, 0, &kbdDriver);  //�������������ֵõ���������ָ�루�����������󣬲��������Լ�д���Ǹ�������
	//ÿ�����ö��󶼻�ʹָ���һ
	if (status != STATUS_SUCCESS)
	{
		DbgPrint("��ȡ��������ʧ��\n");
		return status;
	}
	else
	{
		ObDereferenceObject(pDriverObject);//�Ƚ������,�����������ü���-1,��Ӱ����������������

		//����������豸�����γ�һ������,һ��һ�����ɹ����豸���󶨵������豸ջջ��
		pKbdDeviceObject = kbdDriver->DeviceObject; //�����������豸����
		while (pKbdDeviceObject)
		{
			status = IoCreateDevice(pDriverObject, sizeof(DEV_EXT), 0, pKbdDeviceObject->DeviceType, pKbdDeviceObject->Characteristics, 0, &filterDeviceObject); //���������豸
			if (status != STATUS_SUCCESS)
			{
				DbgPrint("�����豸ʧ��\n");
				return status;
			}
			else
			{
				pTopDev = IoAttachDeviceToDeviceStack(filterDeviceObject, pKbdDeviceObject); //���ص���Ŀ���豸���豸ջջ�����Ǹ��豸��Ҳ�������Ϊ����ɺ������Լ��Ĺ����豸���µ��Ǹ��豸��
				if (!pTopDev)
				{
					DbgPrint("����ʧ��\n");
					IoDeleteDevice(filterDeviceObject);
					status = STATUS_UNSUCCESSFUL;
					return status;
				}
				else
				{
					//�󶨳ɹ����Ƚ�ʵ�ʱ��󶨵��豸���������������豸������ӵ������豸��                    
					//��չ����(�Զ���Ķ������������������)��
					pDevExt = (PDEV_EXT)filterDeviceObject->DeviceExtension;  //�����豸��չ
					initDevExt(pDevExt, filterDeviceObject, pTopDev, pKbdDeviceObject); //��д�豸��չ����

					//�������������ù����豸�����ԣ�
					filterDeviceObject->DeviceType = pTopDev->DeviceType;
					filterDeviceObject->Characteristics = pTopDev->Characteristics;
					filterDeviceObject->StackSize = pTopDev->StackSize + 1;
					filterDeviceObject->Flags |= pTopDev->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
				}
			}

			//�ƶ�����һ���豸����
			pKbdDeviceObject = pKbdDeviceObject->NextDevice;
		}

		return STATUS_SUCCESS;
	}
}

void __stdcall showKey(USHORT ch)
{
	UCHAR c = 0;
	int off = 0;
	if ((ch & 0x80) == 0)
	{

		if (ch < 0x47 || ((ch >= 0x47 && ch < 0x54) && (kb_status & S_NUM)))
		{
			c = asciiTbl[ch + off];
		}

		switch (ch)
		{
		case 0x3a:
			kb_status ^= S_CAPS;
			break;
		case 0x45:
			kb_status ^= S_NUM;
			break;
			//��shift����shift
		case 0x2a:
		case 0x36:
			kb_status |= S_SHIFT;
			break;
		default:
			break;
		}
	}
	else
	{
		if (ch == 0xaa || ch == 0xb6)
		{
			kb_status &= ~S_SHIFT;
		}
	}
	if (c >= 0x20 && c < 0x7f)
	{
		DbgPrint("%c \n", c);
	}

}

//irp��ɸú���
NTSTATUS readComplete(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	//���е��������붼��Ϊ�������׼����.
	PIO_STACK_LOCATION pIrpStack;
	ULONG buf_len = 0;
	PUCHAR buf = NULL;
	size_t i, numKeys;
	PKEYBOARD_INPUT_DATA KeyData;  //ÿ����һ�������ͻ����һ���ṹ
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp); //���IRP��ջ

	if (NT_SUCCESS(pIrp->IoStatus.Status)) //�ж����IRP�Ƿ�ɹ������Ե����ϴ��Ƿ����쳣��
	{
		//��ȡ��ȡ�Ļ������ͳ���
		buf = pIrp->AssociatedIrp.SystemBuffer;
		buf_len = pIrp->IoStatus.Information;

		//    //�û�������ʵ��KEYBOARD_INPUT_DATA�ṹ��,KeyData��ȡһ���ṹ��,��numKeys��
		//    //typedef struct _KEYBOARD_INPUT_DATA {
		//    USHORT UnitId;
		//    USHORT MakeCode; //ɨ����
		//    USHORT Flags;        //1�ǰ���0�ǵ���
		//    USHORT Reserved;
		//    ULONG ExtraInformation;
		//} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;
		KeyData = (PKEYBOARD_INPUT_DATA)buf;
		numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);	//�����Ϣ�Ĵ�С/ÿһ�������Ĵ�С=���ٸ�������Ϊ�п�������ϼ�


		//����Щ�ṹ������İ�����Ϣ�����ɨ����ͼ��ǰ��»��ǵ���
		for (i = 0; i < numKeys; i++)
		{
			DbgPrint("numkeys: %d\n", numKeys);
			DbgPrint("scanf code is %x\n", KeyData->MakeCode);
			DbgPrint("%s\n", KeyData->Flags ? "up" : "down");
			showKey(KeyData->MakeCode);
			//�Դ�д���������й���,�滻Ϊctrl
			/*if (KeyData->MakeCode == CAPS_LOCK)
			{
				KeyData->MakeCode = LCONTROL;
			}*/
		}
	}
	keyCount--;
	if (pIrp->PendingReturned)  //�ж�irp�����λ���Ƿ����ڵȴ���ɣ�����ǣ���������
	{
		IoMarkIrpPending(pIrp);

	}
	return pIrp->IoStatus.Status; //read�����¶��ϵģ�������ϵ�������crsse.exe�����Բ���IoCallDriver
}
//��ͨ�ַ�����
NTSTATUS MyDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//��power,pnp,read���͵�irpһ������
	IoSkipCurrentIrpStackLocation(pIrp); //������ǰ�豸ջ���ϲ��������²�������д�ģ���д���Ƕ�irp������ʲô������Ϊû�ж�irp����
	return IoCallDriver(((DEV_EXT*)pDeviceObject->DeviceExtension)->LowerDeviceObject, pIrp);  //���Զ����µ�irp���´������ǵ��豸����㣩�����͸��ײ��豸
}
//��Դ����
NTSTATUS MyPowerDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//����powerirp,��ʵ������irpһ��ֻ�ǽ�power����irp����ջ������豸������
	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	return PoCallDriver(((DEV_EXT*)pDeviceObject->DeviceExtension)->LowerDeviceObject, pIrp);
}
//���弴��
NTSTATUS MyPnpDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//�����弴��,���ڴ�������弴�õ��豸�����������������
	DEV_EXT* devExt = (DEV_EXT*)pDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql; //���ȼ�
	KEVENT event;

	//�豸���Ƴ�,�����ʱ������Щ��irp��Ϣ.ֱ����������,������Ϊ�Ƴ���ʱ����Ҫ�����ӵ��豸
	//�������󶨲�ɾ��
	switch (pIrpStack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
		DbgPrint("���̱��Ƴ�\n");
		IoSkipCurrentIrpStackLocation(pIrp);
		IoCallDriver(devExt->LowerDeviceObject, pIrp);
		IoDetachDevice(devExt->LowerDeviceObject);
		IoDeleteDevice(pDeviceObject);
		status = STATUS_SUCCESS;
		break;
	default:
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(devExt->LowerDeviceObject, pIrp);
		status = STATUS_SUCCESS;
		break;
	}

	return status;
}

NTSTATUS MyReadDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	DEV_EXT* devExt;
	PIO_STACK_LOCATION pIrpStack;
	KEVENT waitEvent;
	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
	if (pIrp->CurrentLocation == 1)
	{
		ULONG returnInformation = 0;
		DbgPrint("bogus current\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = returnInformation;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;

	}
	else
	{
		//���ﴦ��irp,��Ϊirp��ջ������ʱ����I/O������������,��δ���ײ���豸������
		//����Ҫ�Ƚ�irp���·�,Ȼ���irp��ջ�ױ�д����̰�����Ϣ���ٴ�ջ�����Ϸ�,��ʱ
		//�Ϳ��Խػ�irp�еİ�����Ϣ. ��ô�ػ���?ͨ��ע��һ���������,��Ϊ��irp��������
		//��ӵײ����ϵ�������ע����������.����������Ҫͨ��ע��������̻�ȡirp������
		//��ϸԭ��ο�:windows�ں�ԭ����ʵ�� 462ҳ��������P488��
		//�����������ʵ��һ����,ֻ����Ϊ��Ҫע��������̶���Ҫ����
		//����IoCopyCurrentIrpStackLocationToNext������IoSkipCurrentIrpStackLocation
		keyCount++;
		devExt = (DEV_EXT*)pDeviceObject->DeviceExtension;
		pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
		IoCopyCurrentIrpStackLocationToNext(pIrp);

		//ע��������̺���
		IoSetCompletionRoutine(pIrp, readComplete, pDeviceObject, 1, 1, 1);
		return IoCallDriver(devExt->LowerDeviceObject, pIrp);
	}
}
VOID MyDetach(PDEVICE_OBJECT pDeviceObject)
{
	DEV_EXT* devExt = pDeviceObject->DeviceExtension;
	__try
	{
		__try
		{
			IoDetachDevice(devExt->TargetDeviceObject); //��ʱ�󶨵�Ŀ���豸
			devExt->TargetDeviceObject = 0;
			IoDeleteDevice(pDeviceObject);
			devExt->pFilterDeviceObject = 0;
			DbgPrint("�����\n");
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}
	__finally {}
}
VOID UnLoadDriver(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDeviceObject, pOldDeivceObject;
	DEV_EXT devExt;
	LARGE_INTEGER lDelay;
	PRKTHREAD CurrentThread;
	lDelay = RtlConvertLongToLargeInteger(100 * DELAY_ONE_MILLISECOND);//����ʱ�����
	CurrentThread = KeGetCurrentThread();
	//�����ȼ�����
	KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);
	UNREFERENCED_PARAMETER(pDriverObject);
	DbgPrint("��ʼж��\n");
	pDeviceObject = pDriverObject->DeviceObject;
	while (pDeviceObject)
	{
		MyDetach(pDeviceObject);//����󶨲�ɾ��
		pDeviceObject = pDeviceObject->NextDevice;
	}

	while (keyCount)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}
	DbgPrint("ж�����!\n");
	return;

}
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING us)
{
	NTSTATUS status = STATUS_SUCCESS;

	//���÷ַ�����,ר�Ŵ����Դirp,pnp��irp��read��irp
	ULONG i = 0;
	for (; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = MyDisPatcher;
	}
	pDriverObject->MajorFunction[IRP_MJ_READ] = MyReadDisPatcher;  //����ĳ����󣬵ȴ�crss.exe��ȡ�������ݣ����������¶�����������
	pDriverObject->MajorFunction[IRP_MJ_POWER] = MyPowerDisPatcher;  //��Դ����
	pDriverObject->MajorFunction[IRP_MJ_PNP] = MyPnpDisPatcher;  //���弴�ù���

	pDriverObject->DriverUnload = UnLoadDriver;


	MyAttachDevices(pDriverObject);


	return status;
}