#ifndef _APP_H
#define _APP_H
#include <ndis.h>
#include "passthru.h"
#include "PacketManager.h"

#define APP_HTTP 0x1100
#define APP_FTP 0x1101


//虚拟进程数据结构
typedef struct _APP_FICTION_PROCESS
{
	LIST_ENTRY proList;//进程链
	LIST_ENTRY receiveList;//要处理的接收数据包
	LIST_ENTRY sendList;//要处理的发送数据包
	INT realPid;//真实进程id
	INT FicPid;//虚拟进程id

	INT srcIp;
	INT srcPort;
	INT desIp;
	INT desPort;

	
}APP_FICTION_PROCESS, *PAPP_FICTION_PROCESS;

int app_receive(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_APP pPacket, INT Type, PVOID pBuffer, UINT length);
void app_init();

#endif//_APP_H