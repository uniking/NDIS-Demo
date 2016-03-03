#include "PacketManager.h"



PVOID PMAllocatePacket(int Type)
{
	PVOID pNode=NULL;
	NDIS_STATUS nStatus=0;
	UINT len = 0;

	do 
	{
		switch(Type)
		{
		case PM_PACKET_IP:
			len = sizeof(NDIS_PACKET_IP);
			break;
		case PM_PACKET_TCP:
			len = sizeof(NDIS_PACKET_TCP);
			break;
		case PM_PACKET_APP:
			len = sizeof(NDIS_PACKET_APP);
			break;
		default:
			;
		}

		if (len == 0)
		{
			break;
		}

		NdisAllocateMemoryWithTag(&pNode, len, PM_TAG);


	} while (FALSE);
	

	return pNode;
}

//返回NULL，表示说有数据已经取完，可以进行删除了
PNDIS_PACKET PMRemoveNdisPacket(PNDIS_PACKET_APP root)
{
	PNDIS_PACKET_TCP tcpPacket = NULL;
	PNDIS_PACKET_IP ipPacket = NULL;
	PNDIS_PACKET ndisPacket = NULL;
	PLIST_ENTRY ipNode = NULL;

	tcpPacket = root->data;

	ipNode = tcpPacket->data.Flink;
	for(;ipNode != &tcpPacket->data; ipNode = ipNode->Flink)
	{
		ipPacket =  CONTAINING_RECORD(ipNode, NDIS_PACKET_IP, IpList);
		if (ipPacket->data != NULL)
		{
			ndisPacket = ipPacket->data;
			ipPacket->data = NULL;
			break;//找到一个就可结束了
		}
	}

	return ndisPacket;
}

VOID PMDeleteAppPacket(PNDIS_PACKET_APP* root)
{
	PNDIS_PACKET_TCP tcpPacket = NULL;
	PNDIS_PACKET_IP ipPacket = NULL;
	PNDIS_PACKET ndisPacket = NULL;
	PLIST_ENTRY ipNode = NULL;

	tcpPacket = (*root)->data;

	for (;tcpPacket->data.Flink != &tcpPacket->data; )
	{
		ipNode = RemoveHeadList(&tcpPacket->data);
		ipPacket = CONTAINING_RECORD(ipNode, NDIS_PACKET_IP, IpList);

		ASSERT(ipPacket->data == NULL);

		NdisFreeMemory(ipPacket, sizeof(NDIS_PACKET_IP), PM_TAG);
	}

	NdisFreeMemory(tcpPacket, sizeof(NDIS_PACKET_TCP), PM_TAG);

	NdisFreeMemory(*root, sizeof(NDIS_PACKET_APP), PM_TAG);

	*root = NULL;
}

VOID PMFreeMemery(PVOID pBuffer, UINT length)
{
	NdisFreeMemory(pBuffer, length, PM_TAG);
}