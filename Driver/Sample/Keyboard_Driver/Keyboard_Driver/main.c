//__stdcall
#include<ntddk.h>
#include<Ntddkbd.h>
#include<ntstrsafe.h>
#pragma code_seg("INT")

extern POBJECT_TYPE IoDriverObjectType;
#define KBD_DRIVER_NAME L"\\Driver\\kbdClass"
//用于计时
#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING objectName, ULONG Attributes, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess, POBJECT_TYPE objectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

ULONG keyCount = 0;

//设置大写锁定,小键盘锁定和shift键状态
#define    S_SHIFT                1
#define    S_CAPS                2
#define    S_NUM                4
static int kb_status = S_NUM;

#define KEY_UP 1 
#define KEY_DOWN 0 

//定义大写锁定键和ctrl键的扫描码,其实可以定义一个全键盘的扫描码
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
	// 这个结构的大小
	ULONG NodeSize;
	// 过滤设备对象（自己生成的设备）
	PDEVICE_OBJECT pFilterDeviceObject;
	// 绑定的设备对象
	PDEVICE_OBJECT TargetDeviceObject;
	// 绑定前底层设备对象
	PDEVICE_OBJECT LowerDeviceObject;
} DEV_EXT, * PDEV_EXT;

//填写设备扩展函数
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
	PDEV_EXT pDevExt = NULL; //设备扩展

	//获取驱动对象
	RtlInitUnicodeString(&kbdDriverName, KBD_DRIVER_NAME); //这个驱动就是系统用来管理物理键盘的驱动程序
	status = ObReferenceObjectByName(&kbdDriverName, OBJ_CASE_INSENSITIVE, 0, 0, IoDriverObjectType, KernelMode, 0, &kbdDriver);  //由驱动对象名字得到驱动对象指针（键盘驱动对象，不是我们自己写的那个驱动）
	//每次引用对象都会使指针加一
	if (status != STATUS_SUCCESS)
	{
		DbgPrint("获取驱动对象失败\n");
		return status;
	}
	else
	{
		ObDereferenceObject(pDriverObject);//先解除引用,驱动对象引用计数-1,不影响后面代码以免忘记

		//驱动对象的设备对象形成一个链表,一个一个生成过滤设备并绑定到它的设备栈栈顶
		pKbdDeviceObject = kbdDriver->DeviceObject; //键盘驱动的设备对象
		while (pKbdDeviceObject)
		{
			status = IoCreateDevice(pDriverObject, sizeof(DEV_EXT), 0, pKbdDeviceObject->DeviceType, pKbdDeviceObject->Characteristics, 0, &filterDeviceObject); //创建过滤设备
			if (status != STATUS_SUCCESS)
			{
				DbgPrint("创建设备失败\n");
				return status;
			}
			else
			{
				pTopDev = IoAttachDeviceToDeviceStack(filterDeviceObject, pKbdDeviceObject); //返回的是目标设备，设备栈栈顶的那个设备。也可以理解为绑定完成后，我们自己的过滤设备底下的那个设备。
				if (!pTopDev)
				{
					DbgPrint("附加失败\n");
					IoDeleteDevice(filterDeviceObject);
					status = STATUS_UNSUCCESSFUL;
					return status;
				}
				else
				{
					//绑定成功后先将实际被绑定的设备对象和驱动对象的设备对象添加到过滤设备的                    
					//拓展对象(自定义的东西方便后续访问他们)上
					pDevExt = (PDEV_EXT)filterDeviceObject->DeviceExtension;  //设置设备扩展
					initDevExt(pDevExt, filterDeviceObject, pTopDev, pKbdDeviceObject); //填写设备扩展函数

					//复制特征（设置过滤设备的属性）
					filterDeviceObject->DeviceType = pTopDev->DeviceType;
					filterDeviceObject->Characteristics = pTopDev->Characteristics;
					filterDeviceObject->StackSize = pTopDev->StackSize + 1;
					filterDeviceObject->Flags |= pTopDev->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
				}
			}

			//移动到下一个设备对象
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
			//左shift和右shift
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

//irp完成该函数
NTSTATUS readComplete(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	//所有的其他代码都是为这个函数准备的.
	PIO_STACK_LOCATION pIrpStack;
	ULONG buf_len = 0;
	PUCHAR buf = NULL;
	size_t i, numKeys;
	PKEYBOARD_INPUT_DATA KeyData;  //每按下一个键，就会产生一个结构
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp); //获得IRP堆栈

	if (NT_SUCCESS(pIrp->IoStatus.Status)) //判断这个IRP是否成功。（自底向上传是否发生异常）
	{
		//获取读取的缓冲区和长度
		buf = pIrp->AssociatedIrp.SystemBuffer;
		buf_len = pIrp->IoStatus.Information;

		//    //该缓冲区其实是KEYBOARD_INPUT_DATA结构体,KeyData获取一个结构体,有numKeys个
		//    //typedef struct _KEYBOARD_INPUT_DATA {
		//    USHORT UnitId;
		//    USHORT MakeCode; //扫描码
		//    USHORT Flags;        //1是按下0是弹起
		//    USHORT Reserved;
		//    ULONG ExtraInformation;
		//} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;
		KeyData = (PKEYBOARD_INPUT_DATA)buf;
		numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);	//完成信息的大小/每一个按键的大小=多少个键，因为有可能是组合键


		//对这些结构体包含的按键信息输出即扫描码和键是按下还是弹起
		for (i = 0; i < numKeys; i++)
		{
			DbgPrint("numkeys: %d\n", numKeys);
			DbgPrint("scanf code is %x\n", KeyData->MakeCode);
			DbgPrint("%s\n", KeyData->Flags ? "up" : "down");
			showKey(KeyData->MakeCode);
			//对大写锁定键进行过滤,替换为ctrl
			/*if (KeyData->MakeCode == CAPS_LOCK)
			{
				KeyData->MakeCode = LCONTROL;
			}*/
		}
	}
	keyCount--;
	if (pIrp->PendingReturned)  //判断irp的完成位，是否是在等待完成，如果是，则标记起来
	{
		IoMarkIrpPending(pIrp);

	}
	return pIrp->IoStatus.Status; //read是自下而上的，层层向上弹，弹到crsse.exe，所以不用IoCallDriver
}
//普通分发函数
NTSTATUS MyDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//非power,pnp,read类型的irp一律跳过
	IoSkipCurrentIrpStackLocation(pIrp); //跳过当前设备栈（上层驱动给下层驱动填写的，填写的是对irp操作了什么），因为没有对irp操作
	return IoCallDriver(((DEV_EXT*)pDeviceObject->DeviceExtension)->LowerDeviceObject, pIrp);  //把自顶向下的irp向下传（我们的设备在最顶层），发送给底层设备
}
//电源管理
NTSTATUS MyPowerDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//处理powerirp,其实和其他irp一样只是将power类型irp交给栈下面的设备对象处理
	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	return PoCallDriver(((DEV_EXT*)pDeviceObject->DeviceExtension)->LowerDeviceObject, pIrp);
}
//即插即用
NTSTATUS MyPnpDisPatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//处理即插即用,对于大多数即插即用的设备基本都这样处理就行
	DEV_EXT* devExt = (DEV_EXT*)pDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql; //优先级
	KEVENT event;

	//设备被移除,插入的时候发生这些副irp消息.直接跳过即可,但是因为移除的时候需要将附加的设备
	//对象解除绑定并删除
	switch (pIrpStack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
		DbgPrint("键盘被移除\n");
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
		//这里处理irp,因为irp从栈顶过来时是有I/O管理器发来的,还未被底层的设备对象处理
		//所以要先将irp向下发,然后等irp从栈底被写入键盘按键信息后再从栈底往上发,这时
		//就可以截获irp中的按键信息. 怎么截获呢?通过注册一个完成例程,因为当irp被处理完
		//会从底层向上调用他们注册的完成例程.所以这里需要通过注册完成例程获取irp并处理
		//详细原理参考:windows内核原理与实现 462页（电子书P488）
		//这里和跳过其实是一样的,只是因为需要注册完成例程而需要调用
		//函数IoCopyCurrentIrpStackLocationToNext而不是IoSkipCurrentIrpStackLocation
		keyCount++;
		devExt = (DEV_EXT*)pDeviceObject->DeviceExtension;
		pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
		IoCopyCurrentIrpStackLocationToNext(pIrp);

		//注册完成例程函数
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
			IoDetachDevice(devExt->TargetDeviceObject); //当时绑定的目标设备
			devExt->TargetDeviceObject = 0;
			IoDeleteDevice(pDeviceObject);
			devExt->pFilterDeviceObject = 0;
			DbgPrint("解除绑定\n");
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
	lDelay = RtlConvertLongToLargeInteger(100 * DELAY_ONE_MILLISECOND);//设置时间对象
	CurrentThread = KeGetCurrentThread();
	//将优先级降低
	KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);
	UNREFERENCED_PARAMETER(pDriverObject);
	DbgPrint("开始卸载\n");
	pDeviceObject = pDriverObject->DeviceObject;
	while (pDeviceObject)
	{
		MyDetach(pDeviceObject);//解除绑定并删除
		pDeviceObject = pDeviceObject->NextDevice;
	}

	while (keyCount)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}
	DbgPrint("卸载完成!\n");
	return;

}
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING us)
{
	NTSTATUS status = STATUS_SUCCESS;

	//设置分发函数,专门处理电源irp,pnp的irp和read的irp
	ULONG i = 0;
	for (; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = MyDisPatcher;
	}
	pDriverObject->MajorFunction[IRP_MJ_READ] = MyReadDisPatcher;  //按下某个间后，等待crss.exe读取按键内容，而不是自下而上主动传输
	pDriverObject->MajorFunction[IRP_MJ_POWER] = MyPowerDisPatcher;  //电源管理
	pDriverObject->MajorFunction[IRP_MJ_PNP] = MyPnpDisPatcher;  //即插即用管理

	pDriverObject->DriverUnload = UnLoadDriver;


	MyAttachDevices(pDriverObject);


	return status;
}