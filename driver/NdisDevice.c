#include "precomp.h"
#include "NdisDevice.h"
#include "..\common\Ioctls.h"
#include "PolicyList.h"


PDEVICE_OBJECT g_NdisMControlDev = NULL;
BOOLEAN			gNdisMonitor = FALSE;

extern FORBIDE_TCP_PORT gForbidTcpPortList;
extern FORBIDE_UDP_PORT gForbidUdpPortList;

void CreateNdisMControlDevice(
							  IN PDRIVER_OBJECT   pDriverObject,
							  IN PUNICODE_STRING  pRegistryPath)
{
	NTSTATUS						status = STATUS_SUCCESS ;
	PDEVICE_OBJECT                  deviceObject = NULL;
	UNICODE_STRING                  ntDeviceName = {0};
	UNICODE_STRING                  win32DeviceName = {0};
	INT								i = 0;
	PDEVICE_EXTENSION				pDevExt = 0;

	do 
	{

		//pDriverObject->DriverUnload = DDKUnload;

		//for (i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
		//{
		//	pDriverObject->MajorFunction[i] = DDKDispatchRoutin;
		//}

		//pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DDKDeviceIOControl;


		//
		// Create our device object using which an application can
		// access NDIS devices.
		//
		RtlInitUnicodeString(&ntDeviceName, NT_DEVICE_NAME);

		status = IoCreateDevice (pDriverObject,
			sizeof(DEVICE_EXTENSION),
			&ntDeviceName,
			FILE_DEVICE_UNKNOWN,
			0,
			FALSE,
			&deviceObject);


		if (!NT_SUCCESS (status))
		{
			//
			// Either not enough memory to create a deviceobject or another
			// deviceobject with the same name exits. This could happen
			// if you install another instance of this device.
			//
			break;
		}

		RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

		status = IoCreateSymbolicLink(&win32DeviceName, &ntDeviceName);

		if (!NT_SUCCESS(status))
		{
			break;
		}

		//deviceObject->Flags |= DO_DIRECT_IO;
		pDevExt = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
		pDevExt->pDevice = deviceObject;
		pDevExt->ustrDeviceName = ntDeviceName;
		pDevExt->iDeviceType = 1221;

		pDevExt->ustrSymLinkName.Buffer = ExAllocatePoolWithTag(PagedPool, win32DeviceName.MaximumLength, 'Sln1');
		RtlCopyMemory(pDevExt->ustrSymLinkName.Buffer, win32DeviceName.Buffer, win32DeviceName.MaximumLength);
		pDevExt->ustrSymLinkName.Length = win32DeviceName.Length;
		pDevExt->ustrSymLinkName.MaximumLength = win32DeviceName.MaximumLength;

		g_NdisMControlDev = deviceObject;

	} while (FALSE);

}

void DeleteNdisMControlDevice()
{

}

#pragma PAGEDCODE
NTSTATUS DDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj,
						   IN PIRP pIrp) 
{
	UCHAR type;
	PIO_STACK_LOCATION stack;
	NTSTATUS status;
	static char* irpname[] = 
	{
		"IRP_MJ_CREATE",
		"IRP_MJ_CREATE_NAMED_PIPE",
		"IRP_MJ_CLOSE",
		"IRP_MJ_READ",
		"IRP_MJ_WRITE",
		"IRP_MJ_QUERY_INFORMATION",
		"IRP_MJ_SET_INFORMATION",
		"IRP_MJ_QUERY_EA",
		"IRP_MJ_SET_EA",
		"IRP_MJ_FLUSH_BUFFERS",
		"IRP_MJ_QUERY_VOLUME_INFORMATION",
		"IRP_MJ_SET_VOLUME_INFORMATION",
		"IRP_MJ_DIRECTORY_CONTROL",
		"IRP_MJ_FILE_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CONTROL",
		"IRP_MJ_INTERNAL_DEVICE_CONTROL",
		"IRP_MJ_SHUTDOWN",
		"IRP_MJ_LOCK_CONTROL",
		"IRP_MJ_CLEANUP",
		"IRP_MJ_CREATE_MAILSLOT",
		"IRP_MJ_QUERY_SECURITY",
		"IRP_MJ_SET_SECURITY",
		"IRP_MJ_POWER",
		"IRP_MJ_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CHANGE",
		"IRP_MJ_QUERY_QUOTA",
		"IRP_MJ_SET_QUOTA",
		"IRP_MJ_PNP",
	};

	KdPrint(("Enter HelloDDKDispatchRoutin\n"));

	stack = IoGetCurrentIrpStackLocation(pIrp);
	//建立一个字符串数组与IRP类型对应起来


	type = stack->MajorFunction;
	if (type >= arraysize(irpname))
		KdPrint((" - Unknown IRP, major type %X\n", type));
	else
		KdPrint(("\t%s\n", irpname[type]));

	status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );

	KdPrint(("Leave HelloDDKDispatchRoutin\n"));

	return status;
}



#pragma PAGEDCODE
NTSTATUS DDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj,
							IN PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING ustrFilePath = {0};
	UNICODE_STRING ustrSrcFilePath = {0};
	UNICODE_STRING ustrDstFilePath = {0};
	IO_STATUS_BLOCK 					DstIoStatus = { 0 };
	PVOID		buffer = NULL;
	INT	*pProcessId = NULL;
	PVOID pBuffer = NULL;
	PDEVICE_EXTENSION pDevExt = NULL;
	ULONG info = 0;


	//得到当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//得到输入缓冲区大小
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	//得到输出缓冲区大小
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	//得到IOCTL码
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	//buffer
	if (pIrp->MdlAddress != NULL)
	{
		pBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
	}
	if (pIrp->AssociatedIrp.SystemBuffer != NULL)
	{
		pBuffer = pIrp->AssociatedIrp.SystemBuffer;
	}


	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;



	KdPrint(("Enter HelloDDKDeviceIOControl\n"));

	switch (code)
	{						// process request
	case IOCTL_START_MONITOR:
		{
			gNdisMonitor = TRUE;
			break;
		}
	case IOCTL_STOP_MONITOR:
		{
			gNdisMonitor = FALSE;
			break;
		}

	default:
		status = STATUS_INVALID_VARIANT;
	}

	// 完成IRP
	//pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;	// bytes xfered
	//IoCompleteRequest( pIrp, IO_NO_INCREMENT );

	KdPrint(("Leave HelloDDKDeviceIOControl\n"));

	return status;
}

#pragma PAGEDCODE
VOID DDKUnload (IN PDRIVER_OBJECT pDriverObject) 
{
	PDEVICE_OBJECT	pNextObj;
	PDEVICE_EXTENSION pDevExt;
	PUNICODE_STRING pLinkName;

	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL) 
	{
		pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;
		//删除符号链接
		pLinkName = &(pDevExt->ustrSymLinkName);
		IoDeleteSymbolicLink(pLinkName);
		ExFreePoolWithTag(pLinkName->Buffer, 'Sln1');
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice( pDevExt->pDevice );
	}
}



//////////////////////////////////////////////////////////////////////////
NTSTATUS
DevOpen(
		IN PDEVICE_OBJECT    pDeviceObject,
		IN PIRP              pIrp
		)
{
	PIO_STACK_LOCATION  pIrpSp;
	NTSTATUS            NtStatus = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pDeviceObject);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	pIrpSp->FileObject->FsContext = NULL;
	pIrpSp->FileObject->FsContext2 = NULL;

	DBGPRINT(("==>Pt DevOpen: FileObject %p\n", pIrpSp->FileObject));

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DBGPRINT(("<== Pt DevOpen\n"));

	return NtStatus;
} 
NTSTATUS
DevCleanup(
		   IN PDEVICE_OBJECT    pDeviceObject,
		   IN PIRP              pIrp
		   )
{
	PIO_STACK_LOCATION  pIrpSp;
	NTSTATUS            NtStatus = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pDeviceObject);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	DBGPRINT(("==>Pt DevCleanup: FileObject %p\n", pIrpSp->FileObject ));

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DBGPRINT(("<== Pt DevCleanup\n"));

	return NtStatus;
} 
NTSTATUS
DevClose(
		 IN PDEVICE_OBJECT    pDeviceObject,
		 IN PIRP              pIrp
		 )
{
	PIO_STACK_LOCATION  pIrpSp;
	NTSTATUS            NtStatus = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pDeviceObject);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	DBGPRINT(("==>Pt DevClose: FileObject %p\n", pIrpSp->FileObject ));

	pIrpSp->FileObject->FsContext = NULL;
	pIrpSp->FileObject->FsContext2 = NULL;

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DBGPRINT(("<== Pt DevClose\n"));

	return NtStatus;
} 
NTSTATUS
DevIoControl(
			 IN PDEVICE_OBJECT    pDeviceObject,
			 IN PIRP              pIrp
			 )
{
	PIO_STACK_LOCATION  pIrpSp;
	NTSTATUS            NtStatus = STATUS_SUCCESS;
	ULONG               BytesReturned = 0;
	ULONG               FunctionCode;
	PUCHAR              ioBuffer = NULL;
	ULONG               inputBufferLength;
	ULONG               outputBufferLength;

	UNREFERENCED_PARAMETER(pDeviceObject);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	ioBuffer = pIrp->AssociatedIrp.SystemBuffer;
	inputBufferLength  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	FunctionCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

	DBGPRINT(("==>Pt DevIoControl: FileObject %p\n", pIrpSp->FileObject ));

	switch (FunctionCode)
	{
	case IOCTL_START_MONITOR:
		{
			gNdisMonitor = TRUE;
			NtStatus = STATUS_SUCCESS;
			break;
		}
	case IOCTL_STOP_MONITOR:
		{
		gNdisMonitor = FALSE;
		NtStatus = STATUS_SUCCESS;
		break;
		}
	case IOCTL_FORBID_TCP_PORT_BY_PID:
		{
			PCOMMAND_MESSAGE pMessage = NULL;
			INT i = 0;
			pMessage = (PCOMMAND_MESSAGE)ioBuffer;

			while(i < pMessage->PortPolicy.num)
			{
				AddForbidTcpPort(pMessage->PortPolicy.port[i]);
				i++;
			}
			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IOCTL_ALLOW_TCP_PORT_BY_PID:
		{
			PCOMMAND_MESSAGE pMessage = NULL;
			INT i = 0;
			pMessage = (PCOMMAND_MESSAGE)ioBuffer;

			while(i < pMessage->PortPolicy.num)
			{
				AddForbidTcpPort(pMessage->PortPolicy.port[i]);
				i++;
			}
			NtStatus = STATUS_SUCCESS;
			break;
		}
	case IOCTL_FORBID_UDP_PORT_BY_PID:
		{
			PCOMMAND_MESSAGE pMessage = NULL;
			INT i = 0;
			pMessage = (PCOMMAND_MESSAGE)ioBuffer;

			while(i < pMessage->PortPolicy.num)
			{
				AddForbidUdpPort(pMessage->PortPolicy.port[i]);
				i++;
			}
			NtStatus = STATUS_SUCCESS;
			break;
		}
	case IOCTL_ALLOW_UDP_PORT_BY_PID:
		{
			PCOMMAND_MESSAGE pMessage = NULL;
			INT i = 0;
			pMessage = (PCOMMAND_MESSAGE)ioBuffer;

			while(i < pMessage->PortPolicy.num)
			{
				DeleteForbidUdpPort(pMessage->PortPolicy.port[i]);
				i++;
			}
			NtStatus = STATUS_SUCCESS;
			break;
		}
	default:
		NtStatus = STATUS_NOT_SUPPORTED;
		break;
	}

	if (NtStatus != STATUS_PENDING)
	{
		pIrp->IoStatus.Information = BytesReturned;
		pIrp->IoStatus.Status = NtStatus;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}

	DBGPRINT(("<== Pt DevIoControl\n"));

	return NtStatus;
}

VOID InitEnvironment()
{
	INT i = 0;

	while(i < MAX_FORBID_PORT_NUM)
	{
		gForbidTcpPortList.TcpPortList[i] = BAD_PROT;
		i++;
	}

	i = 0;
	while(i < MAX_FORBID_PORT_NUM)
	{
		gForbidUdpPortList.UdpPortList[i] = BAD_PROT;
		i++;
	}

}