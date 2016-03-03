//#include "Function.h"
#include "precomp.h"
#include "Function.h"
#include "PolicyList.h"
#include "Http.h"
#include "checkup.h"
#include "nh.h"
#include "ipv4.h"
#include "PacketManager.h"

extern BOOLEAN			gNdisMonitor;
extern NDIS_SPIN_LOCK  g_ReassSpinLock;
/*

 以太网包
  |
  |--------->IP
  |          |
  |          ---->TCP
  |          |
  |          ---->UDP
  |          |
  |          ---->ICMP
  |
  ----------->IPv6
  |
  ----------->ARP
  |
  ----------->PARP
  |
  ----------->PPP
  |
  ----------->



*/
NDIS_STATUS PacketAnalysis(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET pPacket, INT Type)
{
	NDIS_STATUS status = 0;
	PNDIS_BUFFER NdisBuffer = NULL;
	UINT TotalPacketLength = 0;
	UINT copysize = 0;
	UINT DataOffset = 0;
	UINT PhysicalBufferCount = 0;
	UINT BufferCount = 0;
	PUCHAR mybuffer = NULL;
	PUCHAR tembuffer = NULL;
	NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;

	//if (!gNdisMonitor)
	//{
	//	return NDIS_STATUS_SUCCESS;
	//}

    if(Type == SEND_PACKET)
    {
        //KdPrint(("send packet:"));
		//return 0;
    }
    else
    {
        //KdPrint(("receive packet:"));
		//return 0;
    }
    

    /*
    获取NDIS_PACKET的详细信息
    如第一个ndis buffer包（头MDL）、ndis buffer包的数目、
    */
	NdisQueryPacket(pPacket,
		&PhysicalBufferCount,
		&BufferCount,
		&NdisBuffer,
		&TotalPacketLength
		);

	HighestAcceptableMax.QuadPart = -1;
	status = NdisAllocateMemory(&mybuffer, 2048, 0, HighestAcceptableMax);

	if(status != NDIS_STATUS_SUCCESS)
	{
		return NDIS_STATUS_FAILURE;
	}

	NdisZeroMemory(mybuffer, 2048);

    /*
    获取一个ndis buffer包的虚拟地址和长度
    */
	NdisQueryBufferSafe(NdisBuffer,
		&tembuffer,
		&copysize,
		NormalPagePriority
		);

	NdisMoveMemory(mybuffer, tembuffer, copysize);

	DataOffset = copysize;

	while(1)
	{
        /*
        获取下一个ndis buffer包
        */
		NdisGetNextBuffer(NdisBuffer, &NdisBuffer);
		if(NdisBuffer == NULL)
		{
			break;
		}

		NdisQueryBufferSafe(NdisBuffer,
			&tembuffer,
			&copysize,
			NormalPagePriority
			);

		NdisMoveMemory(mybuffer+ DataOffset, tembuffer, copysize);
		DataOffset += copysize;
	}

	//分析
	status = EthAnalysis(pAdapt, MiniportHandle, pPacket, Type, (PVOID)mybuffer, DataOffset);
	//if (status == RECEIVE_STORAGE_PACKET)
	//{
	//	if (MiniportHandle != NULL)
	//	{
	//		NdisMIndicateReceivePacket(pAdapt->MiniportHandle, &MyPacket, 1);
	//	}
	//}
	

	//NdisFreePacket(pPacket);
	//按原样复制到pPacket
	//{
	//	NdisQueryPacket(pPacket,
	//		&PhysicalBufferCount,
	//		&BufferCount,
	//		&NdisBuffer,
	//		&TotalPacketLength
	//		);

	//	/*
	//	获取一个ndis buffer包的虚拟地址和长度
	//	*/
	//	NdisQueryBufferSafe(NdisBuffer,
	//		&tembuffer,
	//		&copysize,
	//		NormalPagePriority
	//		);

	//	NdisMoveMemory(tembuffer, mybuffer, copysize);

	//	DataOffset = copysize;

	//	while(1)
	//	{
	//		/*
	//		获取下一个ndis buffer包
	//		*/
	//		NdisGetNextBuffer(NdisBuffer, &NdisBuffer);
	//		if(NdisBuffer == NULL)
	//		{
	//			break;
	//		}

	//		NdisQueryBufferSafe(NdisBuffer,
	//			&tembuffer,
	//			&copysize,
	//			NormalPagePriority
	//			);

	//		NdisMoveMemory(tembuffer, mybuffer+DataOffset, copysize);
	//		DataOffset += copysize;
	//	}
	//}



	//释放
	if (mybuffer != NULL)
	{
		NdisFreeMemory(mybuffer, 2048, 0);
	}

	return status;
}

NDIS_STATUS GetCurrentEnvironment()
{
	ULONG  IdleCount = 0;
	ULONG  KernelAndUser = 0;
	ULONG  Index = 0;
	ULONG  CpuUsage = 0;
	LARGE_INTEGER  SystemTime = {0};

	//#if NDIS_SUPPORT_60_COMPATIBLE_API
	//NDIS_SYSTEM_PROCESSOR_INFO  SystemProcessorInfo = {0};
	//#endif

	UINT Version = 0;
	PEPROCESS pEprocess = NULL;
	HANDLE hPID = 0;

	NdisGetCurrentProcessorCounts(
		&IdleCount,
		&KernelAndUser,
		&Index
		);

	NdisGetCurrentProcessorCpuUsage(
		&CpuUsage
		);

	NdisGetCurrentSystemTime(
		&SystemTime
		);

	//#if NDIS_SUPPORT_60_COMPATIBLE_API
	//NdisGetProcessorInformation(
	//	&SystemProcessorInfo
	//	);
	//#endif

	Version = NdisGetVersion();
	pEprocess = PsGetCurrentProcess();

	//hPID = PsGetProcessId(pEprocess);

	hPID = PsGetCurrentProcessId();

	//KdPrint(("current process id %x\n", hPID));

	return NDIS_STATUS_SUCCESS;
}

/*
分析以太网包
*/
NDIS_STATUS EthAnalysis(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET pPacket, INT Type, PVOID pBuffer, INT length)
{
	NDIS_STATUS status = 0;
	PETH_HEADER pEthHeader = (PETH_HEADER)pBuffer;
	PUCHAR pPointer = (PUCHAR)pBuffer;
	ULONG EthDataLength = 0;
	ULONG crc = 0;

	//GetCurrentEnvironment();
	//KdPrint(("EthType=%x SrcAddr=%x-%x-%x-%x-%x-%x DstAddr=%x-%x-%x-%x-%x-%x\n", 
	//	pEthHeader->EthType, 
	//	pEthHeader->SrcAddr[0], pEthHeader->SrcAddr[1], pEthHeader->SrcAddr[2],
	//	pEthHeader->SrcAddr[3], pEthHeader->SrcAddr[4], pEthHeader->SrcAddr[5],
	//	pEthHeader->DstAddr[0], pEthHeader->DstAddr[1], pEthHeader->DstAddr[2],
	//	pEthHeader->DstAddr[3], pEthHeader->DstAddr[4], pEthHeader->DstAddr[5]
	//));

	if (pEthHeader->EthType == 0x0008)
	{//大尾值0x0800
		pPointer += (sizeof(ETH_HEADER))/sizeof(UCHAR);
		EthDataLength = length -  (sizeof(ETH_HEADER))/sizeof(UCHAR);
		//status = IPAnalysis(pAdapt, MiniportHandle, pPacket, Type, (PVOID)pPointer, EthDataLength);
		if (Type == SEND_PACKET)
		{
		}
		else
		{
			PNDIS_PACKET_IP pIpPacket;
			pIpPacket = PMAllocatePacket(PM_PACKET_IP);
			if (pPacket)
			{
				pIpPacket->data = pPacket;
				pIpPacket->IPLen = 0;
				pIpPacket->SonNum = 1;
				NdisInitializeListHead(&pIpPacket->IpList);
				status = ip_receive(pAdapt, MiniportHandle, pIpPacket, Type, pPointer, EthDataLength);
			}
			
		}

		//crc = EthCheckup(pPointer, EthDataLength);
		//KdPrint(("Eth crc=%x\n", crc));
	}
	if (pEthHeader->EthType == 0x0608)
	{
		//KdPrint(("ARP协议\n"));
	}
	if (pEthHeader->EthType == 0x3580)
	{
		//KdPrint(("RARP协议\n"));
	}
	if (pEthHeader->EthType == 0xdd86)
	{
		//KdPrint(("ipv6协议\n"));
	}
	if (pEthHeader->EthType == 0x0b88)
	{
		//KdPrint(("PPP协议\n"));
	}

	return status;
}

/*
分析Ip包
*/
NDIS_STATUS IPAnalysis(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET pPacket,INT Type, PVOID pBuffer, UINT length)
{
	PIP_HDR pIPHeader = (PIP_HDR)pBuffer;
	PTCP_HDR pTCPHdr =NULL;
	UCHAR * pSrcAddr = (PUCHAR)&pIPHeader->srcaddr;
	UCHAR * pDstAddr = (PUCHAR)&pIPHeader->dstaddr;
	PUCHAR pPointer = (PUCHAR)pIPHeader;
	UINT nDataLen = 0;
	NDIS_STATUS status = 0;
	unsigned short chk_sum;
	pipq ipQueue;
	pipasfrag pSendPack;
	PLIST_ENTRY pNode;

	if (pSrcAddr[3] == 128)
	{
		KdPrint(("version:%d hl:%d tos:%d tot_len:%d id:%d flag=%d offset:%d ttl:%d protocol:%d chk_sum:%d 源ip:%d.%d.%d.%d 目标ip:%d.%d.%d.%d\n",
			pIPHeader->versionAndihl>>4,
			(pIPHeader->versionAndihl&0x0F)*4,
			pIPHeader->tos,
			t_htons(pIPHeader->tot_len),
			t_htons(pIPHeader->id),
			IPH_MORE_FRAGMENT(pIPHeader),
			t_ntohs(IPH_OFFSET(pIPHeader))*8,
			pIPHeader->ttl,
			pIPHeader->protocol,
			t_htons(pIPHeader->chk_sum),
			pSrcAddr[0],pSrcAddr[1],pSrcAddr[2],pSrcAddr[3],
			pDstAddr[0],pDstAddr[1],pDstAddr[2],pDstAddr[3]
		));
	}

	

	if (Type == RECEIVE_PACKET)
	{
		if (IsMultipleFragments(pIPHeader))
		{
			//KdPrint(("receive IP packet have mutiple fragments\n"));
			PNDIS_PACKET    MyPacket;
			status = STORAGE_PACKET;

			MyPacket = pPacket;
			if (IPH_MORE_FRAGMENT(pIPHeader) == 0)
			{
				//ndisClonePacket(pPacket, &MyPacket, pAdapt->RecvPacketPoolHandle);
			}

			ipQueue = ip_reass(MyPacket, pIPHeader);
			if (ipQueue)
			{
				//通知上层收包
				NdisAcquireSpinLock(&g_ReassSpinLock);
				RemoveEntryList(&ipQueue->ipqueue);
				NdisReleaseSpinLock(&g_ReassSpinLock);

				status = RECEIVE_STORAGE_PACKET;
				if (MiniportHandle != NULL)
				{
					while(!IsListEmpty(&ipQueue->datagram))
					{
						pNode = RemoveHeadList(&ipQueue->datagram);
						pSendPack = CONTAINING_RECORD(pNode, ipasfrag, ipf);
						KdPrint(("delete totlen=%x  offset=%x len=%x\n", ipQueue->datagram_len, pSendPack->datagram_offset, pSendPack->datagram_len));
						NdisMIndicateReceivePacket(MiniportHandle, &pSendPack->pPacket, 1);
						//NdisDprFreePacket(pSendPack->pPacket);//删除自建的包

						if (KeGetCurrentIrql() < DISPATCH_LEVEL)
						{
							NdisFreeMemory(pSendPack, sizeof(ipasfrag), 0);
						}
					}

					

					if (KeGetCurrentIrql() < DISPATCH_LEVEL)
					{
						NdisFreeMemory(ipQueue, sizeof(ipq), 0);
					}
					
				}	
			}
			
			return status;
		}
	}

	//pPointer += 20;//(pIPHeader->versionAndihl & 0x0F)*32/8;
	pPointer += (pIPHeader->versionAndihl & 0x0F)*32/8;
	nDataLen = length - (pIPHeader->versionAndihl & 0x0F)*32/8;


	if (pIPHeader->protocol == 1)
	{
		//KdPrint(("ICMP 协议"));
		//status = ICMPAnalysis((PVOID)pPointer);
	}
	if (pIPHeader->protocol == 6)
	{//tcp
		
		//status = TCPAnalysis(pIPHeader, (PVOID)pPointer, nDataLen);
		pTCPHdr = (PTCP_HDR)pPointer;
		if (pTCPHdr->dst_port == 0x5000)
		{
			//chk_sum = chksum_tcp(pBuffer, pPointer, nDataLen);
			//pTCPHdr->chk_sum = chk_sum;
		}
		
		
	}
	if (pIPHeader->protocol == 17)
	{
		//KdPrint(("UDP协议"));
		//status = UDPAnalysis((PVOID)pPointer);
		//if (Type == RECEIVE_PACKET)
		//{
		//	KdPrint(("udp rec datalen=%d\n", nDataLen));
		//}
		//else
		//{
		//	KdPrint(("udp send datalen=%d\n", nDataLen));
		//}
		
	}

	return status;
}

/*
分析TCP包
*/
NDIS_STATUS TCPAnalysis(PVOID IpHeader, PVOID pBuffer, UINT length)
{
	PIP_HDR pIpHeader = IpHeader;
	PTCP_HDR pTCPHdr = (PTCP_HDR)pBuffer;
	PUCHAR pPointer = (PUCHAR)pTCPHdr;
	UINT nDateLen = 0;
	NDIS_STATUS status = 0;
	unsigned short chk_sum;
	PUCHAR pTemp;
	PSD_HEADER psdTcp;
	//KdPrint((
	//	"源端口：%d 目的端口：%d 序列号：%d 应答号：%d 其他：%x 窗口大小：%x 校验和：%x 应急指针：%x\n",
	//	pTCPHdr->src_port,
	//	pTCPHdr->dst_port,
	//	pTCPHdr->seq_no,
	//	pTCPHdr->ack_no,
	//	pTCPHdr->nHLenAndFlag,
	//	pTCPHdr->wnd_size,
	//	pTCPHdr->chk_sum,
	//	pTCPHdr->urgt_p));
	//if (pTCPHdr->src_port != 0)
	//{
	//	KdPrint(("TCP:源端口：%d 目的端口:%d\n", pTCPHdr->src_port, pTCPHdr->dst_port));
	//}
	//if (IsForbidTcpPort(pTCPHdr->src_port, NULL))
	//{
	//	status = FORBID_NET_WORK;
	//}

	pPointer+= sizeof(TCP_HDR);
	nDateLen = length - sizeof(TCP_HDR);

	if (pTCPHdr->dst_port == 80 ||
		pTCPHdr->dst_port == 8080 ||
		pTCPHdr->dst_port == 0x5000) //大尾0x5000 -->80
	{
		if (nDateLen != 0)
		{
			if (HttpAnalysis(pPointer, nDateLen))
			{
				//修改测试
				//psdTcp.saddr = *((unsigned long*)pIpHeader->srcaddr);
				//psdTcp.saddr[0] = pIpHeader->srcaddr[0];
				//psdTcp.saddr[1] = pIpHeader->srcaddr[1];
				//psdTcp.saddr[2] = pIpHeader->srcaddr[2];
				//psdTcp.saddr[3] = pIpHeader->srcaddr[3];
				psdTcp.saddr = pIpHeader->srcaddr;

				//psdTcp.daddr = *((unsigned long*)pIpHeader->dstaddr);
				//psdTcp.daddr[0] = pIpHeader->dstaddr[0];
				//psdTcp.daddr[1] = pIpHeader->dstaddr[1];
				//psdTcp.daddr[2] = pIpHeader->dstaddr[2];
				//psdTcp.daddr[3] = pIpHeader->dstaddr[3];
				psdTcp.daddr = pIpHeader->dstaddr;

				psdTcp.mbz = 0;
				psdTcp.ptcl = pIpHeader->protocol;
				psdTcp.tcpl = t_htons((USHORT)length);//htons(sizeof(TCP_HDR));

				//pTemp = strstr(pPointer, "123123123");
				//if (pTemp)
				//{
					//*pTemp='9';
					pTCPHdr->chk_sum = 0;
					//chk_sum = checksum(pTCPHdr, length);
					chk_sum = chksum_tcp((unsigned short *)&psdTcp, (unsigned short *)pBuffer, length);
					//chk_sum = myChecksum(pTCPHdr, sizeof(TCP_HDR));
					//chk_sum = myChecksum2(&psdTcp, sizeof(PSD_HEADER), pTCPHdr, length);
					pTCPHdr->chk_sum = chk_sum;
				//}
			}

			
		}
	}

	

	
	return status;
}

/*
分析UDP包
*/
NDIS_STATUS UDPAnalysis(PVOID pBuffer)
{
	PUDP_HDR pUDPHdr = (PUDP_HDR)pBuffer;
	NDIS_STATUS status = 0;
	//KdPrint(("源端口：%d 目的端口：%d 校验和：%d 头部长度：%d\n",
	//	pUDPHdr->src_port,
	//	pUDPHdr->dst_port,
	//	pUDPHdr->chk_sum,
	//	pUDPHdr->uhl
	//	));

	if (pUDPHdr->src_port != 0)
	{
		//KdPrint(("UDP 源端口：%d 目的端口：%d\n", pUDPHdr->src_port, pUDPHdr->dst_port));
	}
	if (IsForbidUdpPort(pUDPHdr->src_port, NULL))
	{
		status = FORBID_NET_WORK;
	}
	return status;
}

/*
分析ICMP包
*/
NDIS_STATUS ICMPAnalysis(PVOID pBuffer)
{
	PICMP_HDR pICMPHdr = (PICMP_HDR)pBuffer;
	NDIS_STATUS status = 0;
	//KdPrint(("类型：%d 校验和：%x 代码：%d\n",
	//	pICMPHdr->icmp_type,
	//	pICMPHdr->chk_sum,
	//	pICMPHdr->code
	//	));
	return status;
}





BOOLEAN ndisClonePacket(PNDIS_PACKET OldPacket, PNDIS_PACKET* pNewPacket, NDIS_HANDLE PacketPoolHandle)
{
	BOOLEAN bRtn = FALSE;
	NDIS_STATUS Status;
	PRECV_RSVD            RecvRsvd;

	do 
	{
		NdisDprAllocatePacket(&Status,
			pNewPacket,
			PacketPoolHandle);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}
		

		RecvRsvd = (PRECV_RSVD)((*pNewPacket)->MiniportReserved);
		RecvRsvd->OriginalPkt = OldPacket;

		NDIS_PACKET_FIRST_NDIS_BUFFER(*pNewPacket) = NDIS_PACKET_FIRST_NDIS_BUFFER(OldPacket);
		NDIS_PACKET_LAST_NDIS_BUFFER(*pNewPacket) = NDIS_PACKET_LAST_NDIS_BUFFER(OldPacket);

		//
		// Get the original packet (it could be the same packet as the one
		// received or a different one based on the number of layered miniports
		// below) and set it on the indicated packet so the OOB data is visible
		// correctly to protocols above us.
		//
		NDIS_SET_ORIGINAL_PACKET(*pNewPacket, NDIS_GET_ORIGINAL_PACKET(OldPacket));

		//
		// Set Packet Flags
		//
		NdisGetPacketFlags(*pNewPacket) = NdisGetPacketFlags(OldPacket);

		NDIS_SET_PACKET_STATUS(*pNewPacket, Status);
		NDIS_SET_PACKET_HEADER_SIZE(*pNewPacket, NDIS_GET_PACKET_HEADER_SIZE(OldPacket));

		bRtn = TRUE;

	} while (FALSE);

	return bRtn;
}