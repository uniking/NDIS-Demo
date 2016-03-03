#include "app.h"
#include "PacketManager.h"
#include "Function.h"

LIST_ENTRY g_app_fiction_process;
NDIS_SPIN_LOCK  g_app_process_SpinLock;

void app_init()
{
	NdisInitializeListHead(&g_app_fiction_process);
	NdisAllocateSpinLock(&g_app_process_SpinLock);
}

BOOLEAN app_ip_and_port_match(PNDIS_PACKET_APP pPacket, PAPP_FICTION_PROCESS pProcess)
{
	if (pPacket->srcIp == pProcess->srcIp &&
		pPacket->srcPort == pProcess->srcPort &&
		pPacket->desIp == pProcess->desIp &&
		pPacket->desPort == pProcess->desPort
		)
	{
		return TRUE;
	}

	return FALSE;
}

int app_receive(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_APP pPacket, INT Type, PVOID pBuffer, UINT length)
{
	int type = 0;
	PLIST_ENTRY pProcNode = NULL;
	PAPP_FICTION_PROCESS pProcess = NULL;
	NDIS_STATUS status = 0;

	pProcNode = g_app_fiction_process.Flink;
	for (;pProcNode != &g_app_fiction_process; pProcNode = pProcNode->Flink)
	{
		pProcess = CONTAINING_RECORD(pProcNode, APP_FICTION_PROCESS, proList);
		if (app_ip_and_port_match(pPacket, pProcess))
		{
			break;
		}
	}

	if (pProcNode != &g_app_fiction_process)
	{//找到
	}
	else
	{//未找到
		NdisAllocateMemoryWithTag(&pProcess, sizeof(APP_FICTION_PROCESS), PM_TAG);
		if (pProcess == NULL)
		{
			ASSERT(FALSE);
			return 0;
		}

		NdisInitializeListHead(&pProcess->proList);
		NdisInitializeListHead(&pProcess->receiveList);

		pProcess->srcIp = pPacket->srcIp;
		pProcess->srcPort = pPacket->srcPort;
		pProcess->desIp = pPacket->desIp;
		pProcess->desPort = pPacket->desPort;

		NdisInterlockedInsertHeadList(&g_app_fiction_process, &pProcess->proList, &g_app_process_SpinLock);
	}

	NdisInterlockedInsertTailList(&pProcess->receiveList, &pPacket->appList, &g_app_process_SpinLock);


	//通知线程处理数据
	//暂时直接将数据包提交到上层
	{
		PNDIS_PACKET_APP preceivePacket;
		PNDIS_PACKET ndisPacket;
		PLIST_ENTRY receiveNode = pProcess->receiveList.Flink;
		while(pProcess->receiveList.Flink != &pProcess->receiveList)
		{
			receiveNode = NdisInterlockedRemoveHeadList(&pProcess->receiveList, &g_app_process_SpinLock);
			preceivePacket = CONTAINING_RECORD(receiveNode, NDIS_PACKET_APP, appList);
			
			do 
			{
				ndisPacket = PMRemoveNdisPacket(preceivePacket);
				if (ndisPacket == NULL)
				{
					PMDeleteAppPacket(&preceivePacket);
					break;
				}
				else
				{
					NdisMIndicateReceivePacket(pAdapt->MiniportHandle, &ndisPacket, 1);
					status = RECEIVE_STORAGE_PACKET;
				}
				
			} while (TRUE);
		}
		
	}


	return status;
}