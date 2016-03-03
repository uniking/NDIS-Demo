#include <ndis.h>

#ifndef _NDIS_DEVICE_H
#define _NDIS_DEVICE_H

//#define NT_DEVICE_NAME          L"\\Device\\NdisMCtl"
//#define DOS_DEVICE_NAME         L"\\Global??\\NdisMCtl"

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
//
//#define IOCTL_START_TIMER CTL_CODE(\
//	FILE_DEVICE_UNKNOWN, \
//	0x800, \
//	METHOD_BUFFERED, \
//	FILE_ANY_ACCESS)
//
//#define IOCTL_STOP_TIMER CTL_CODE(\
//	FILE_DEVICE_UNKNOWN, \
//	0x801, \
//	METHOD_IN_DIRECT, \
//	FILE_ANY_ACCESS)

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;
	INT				iDeviceType;
	UNICODE_STRING ustrDeviceName;	//设备名称
	UNICODE_STRING ustrSymLinkName;	//符号链接名
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

void CreateNdisMControlDevice(
							  IN PDRIVER_OBJECT   pDriverObject,
							  IN PUNICODE_STRING  pRegistryPath);

void DeleteNdisMControlDevice();

NTSTATUS DDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj,
						   IN PIRP pIrp);

NTSTATUS DDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj,
							IN PIRP pIrp);

VOID DDKUnload (IN PDRIVER_OBJECT pDriverObject);

NTSTATUS
DevOpen(
		IN PDEVICE_OBJECT    pDeviceObject,
		IN PIRP              pIrp
		);

NTSTATUS
DevCleanup(
		   IN PDEVICE_OBJECT    pDeviceObject,
		   IN PIRP              pIrp
		   );

NTSTATUS
DevClose(
		 IN PDEVICE_OBJECT    pDeviceObject,
		 IN PIRP              pIrp
		 );

NTSTATUS
DevIoControl(
			 IN PDEVICE_OBJECT    pDeviceObject,
			 IN PIRP              pIrp
			 );

VOID InitEnvironment();

#endif//_NDIS_DEVICE_H
