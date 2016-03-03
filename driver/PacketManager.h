#ifndef _PACKET_MANAGER_H
#define _PACKET_MANAGER_H

#include <ndis.h>

#define PM_TAG 'PmmP'


//包的类型
#define PM_PACKET_NDIS 0x721
#define PM_PACKET_IP 0x722
#define PM_PACKET_TCP 0x723
#define PM_PACKET_APP 0x724


#define PM_ERROR_OK 0
#define PM_ERROR_NO_RELATION 0x900

typedef struct _NDIS_PACKET_DATA
{
	PUCHAR VirtualAddress;
	UINT len;
}NDIS_PACKET_DATA, *PNDIS_PACKET_DATA;

typedef struct _NDIS_PACKET_IP
{
	LIST_ENTRY IpList;
	int IPLen;//真实的IP协议长度
	int SonNum;//孩子节点数目
	PNDIS_PACKET data;
	NDIS_PACKET_DATA BufferPointer[32];//保存NDIS PACKET的buffer指针
}NDIS_PACKET_IP, *PNDIS_PACKET_IP;

typedef struct _NDIS_PACKET_TCP
{
	LIST_ENTRY TcpDataList;
	int SonNum;//孩子节点数目
	LIST_ENTRY data;

	unsigned int seq_no;
	unsigned int ack_no;

	unsigned long srcaddr;
	unsigned long dstaddr;
	unsigned short TotLen;//总长，包括报头和数据
	unsigned short tcplen;//数据长

}NDIS_PACKET_TCP, *PNDIS_PACKET_TCP;

typedef struct _NDIS_PACKET_APP
{
	LIST_ENTRY appList;
	int AppLen;//真实的数据长度
	int SonNum;//孩子节点数目

	INT srcIp;
	INT srcPort;
	INT desIp;
	INT desPort;

	PNDIS_PACKET_TCP data;
}NDIS_PACKET_APP, *PNDIS_PACKET_APP;

PVOID PMAllocatePacket(int Type);
PNDIS_PACKET PMRemoveNdisPacket(PNDIS_PACKET_APP root);

//包内不得有ndis packet包
VOID PMDeleteAppPacket(PNDIS_PACKET_APP* root);

VOID PMFreeMemery(PVOID pBuffer, UINT length);

#endif //_PACKET_MANAGER_H