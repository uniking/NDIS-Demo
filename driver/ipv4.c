#include "ipv4.h"
#include "nh.h"
#include "PacketManager.h"
#include "tcp.h"
#include "nh.h"

LIST_ENTRY g_reassdatagrams;
NDIS_SPIN_LOCK  g_ReassSpinLock;

void InitalizeIpv4()
{
	NdisInitializeListHead(&g_reassdatagrams);
	NdisAllocateSpinLock(&g_ReassSpinLock);
}

//判断一个IP包是否分片
BOOLEAN IsMultipleFragments(PIP_HDR iphdr)
{
	if (IPH_OFFSET(iphdr) ==0 &&
		IPH_MORE_FRAGMENT(iphdr) == 0)
	{
		return FALSE;
	}
	return TRUE;
}


//插入，并检测是否组装完成
BOOLEAN ip_reass_chain_frag_into_datagram_and_validate(pipq ipr, pipasfrag ipf)
{
	BOOLEAN bRtn = FALSE;
	ULONG offset, len;
	PIP_HDR fraghdr = &ipf->pIpHdr;
	//pipasfrag pInsertHdr;
	PLIST_ENTRY pNode;
	pipasfrag pTempFrag;

	len =t_ntohs(IPH_LEN(fraghdr)) - IPH_HL(fraghdr)*4;
	offset = t_ntohs(IPH_OFFSET(fraghdr))*8;

	if (IsListEmpty(&ipr->datagram))
	{
		NdisInterlockedInsertHeadList(&ipr->datagram, &ipf->ipf, &g_ReassSpinLock);

		if ((ipr->flags & IP_REASS_FLAG_LASTFRAG) != 0)
		{//已经是最后一个分片
			KdPrint(("ip last frag\n"));
			bRtn = TRUE;
		}

		return bRtn;
	}

	//搜索链表，找到合适的位置
	// a->b->c 偏移一次递增，相当于排序操作，队列本身有序，找到偏移较大的，然后插到左边
	//pInsertHdr->datagram_offset = ipf->datagram_offset;
	for(pNode = ipr->datagram.Flink; pNode != &ipr->datagram; pNode = pNode->Flink)
	{
		pTempFrag = CONTAINING_RECORD(pNode, ipasfrag, ipf);
		if (pTempFrag->datagram_offset > ipf->datagram_offset)
		{//找到比自己大的，插入
			NdisInterlockedInsertTailList(&pTempFrag->ipf, &ipf->ipf, &g_ReassSpinLock);
			//KdPrint(("insert frag offset=%x\n", ipf->datagram_offset));
			break;
		}

		if (pTempFrag->datagram_offset == ipf->datagram_offset)
		{//分配重复
			KdPrint(("ip fragment overlap\n"));
		}
	}

	if (pNode == &ipr->datagram)
	{//没找到比自己大的
		ipr->datagram_len += ipf->datagram_len;
		NdisInterlockedInsertTailList(&ipr->datagram, &ipf->ipf, &g_ReassSpinLock);
	}

	if ((ipr->flags & IP_REASS_FLAG_LASTFRAG) != 0)
	{//已经是最后一个分片
		KdPrint(("ip last frag\n"));
		bRtn = TRUE;
	}


	return bRtn;
}

BOOLEAN ip_address_and_id_match(PIP_HDR a, PIP_HDR b)
{
	if (a->id == b->id &&
		a->srcaddr == b->srcaddr &&
		a->dstaddr == b->dstaddr)
	{
		return TRUE;
	}

	return FALSE;
}

//返回一个正确排序的链，NULL表示组装未完成
pipq ip_reass(PNDIS_PACKET p, PVOID buffer)
{
	PIP_HDR pFraghdr = (PIP_HDR)buffer;
	ULONG offset=0, len=0;
	PLIST_ENTRY pNode = NULL;
	pipq ipr = NULL;
	pipasfrag ipf = NULL;
	NDIS_STATUS nStatus;

	if ((IPH_HL(pFraghdr)*4) != IP_HLEN)
	{
		goto nullreturn;
	}

	offset = t_ntohs(IPH_OFFSET(pFraghdr))*8;
	len = t_ntohs(IPH_LEN(pFraghdr)) - IPH_HL(pFraghdr)*4;

	for (pNode = g_reassdatagrams.Flink; pNode != &g_reassdatagrams; pNode = pNode->Flink)
	{
		ipr = CONTAINING_RECORD(pNode, ipq, ipqueue);
		if (ip_address_and_id_match(pFraghdr, &ipr->iphdr))
		{//匹配完成
			break;
		}

	}

	if (pNode == &g_reassdatagrams)
	{//没有找到节点
		//插入一个新的节点
		nStatus = NdisAllocateMemoryWithTag(
			&ipr,
			sizeof(ipq),
			TAG_IPV4
			);
		if (nStatus != NDIS_STATUS_SUCCESS)
		{
			goto nullreturn;
		}
		NdisInitializeListHead(&ipr->ipqueue);
		NdisInitializeListHead(&ipr->datagram);
		ipr->flags = 0;

		NdisMoveMemory(&ipr->iphdr, pFraghdr, sizeof(IP_HDR));
		NdisInterlockedInsertHeadList(
			&g_reassdatagrams,
			&ipr->ipqueue,
			&g_ReassSpinLock
			);
	}
	else
	{
	}

	//现在有了正确的ipr，找到数据报的位置，插入包
	nStatus = NdisAllocateMemoryWithTag(
		&ipf,
		sizeof(ipasfrag),
		TAG_IPV4
		);
	if (nStatus != NDIS_STATUS_SUCCESS)
	{
		goto nullreturn;
	}
	NdisInitializeListHead(&ipf->ipf);
	NdisMoveMemory(&ipf->pIpHdr, pFraghdr, sizeof(IP_HDR));
	ipf->pPacket = p;
	ipf->datagram_offset = offset;
	ipf->datagram_len = len;

	if (IPH_MORE_FRAGMENT(pFraghdr) == 0) 
	{
		ipr->flags |= IP_REASS_FLAG_LASTFRAG;
		ipr->datagram_len = offset + len;
	}
	if (ip_reass_chain_frag_into_datagram_and_validate(ipr, ipf))
	{//组装完成，返回排序号的链
		return ipr;
	}

nullreturn:
	return NULL;
}

int ip_receive(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_IP pPacket, INT Type, PVOID pBuffer, UINT length)
{
	PNDIS_PACKET_TCP pTcpPacket;
	PIP_HDR pIPHeader = pBuffer;
	PCHAR pTcpHdr = (PCHAR)pBuffer;
	UINT IpDataLen = 0;
	INT status = 0;

	switch(pIPHeader->protocol)
	{
	case 1://ICMP
		break;
	case 6://TCP
		pTcpPacket = PMAllocatePacket(PM_PACKET_TCP);
		InitializeListHead(&pTcpPacket->data);
		InsertHeadList(&pTcpPacket->data, &pPacket->IpList);
		pTcpPacket->SonNum = 1;
		pTcpPacket->tcplen = 0;
		pTcpPacket->srcaddr = pIPHeader->srcaddr;
		pTcpPacket->dstaddr = pIPHeader->dstaddr;
		NdisInitializeListHead(&pTcpPacket->TcpDataList);

		pTcpHdr +=  (pIPHeader->versionAndihl & 0x0F)*32/8;
		IpDataLen = length -  (pIPHeader->versionAndihl & 0x0F)*32/8;
		pTcpPacket->TotLen = t_ntohs(pIPHeader->tot_len)-((pIPHeader->versionAndihl & 0x0F)*32/8);
		status = tcp_recevie(pAdapt, MiniportHandle, pTcpPacket, Type, pTcpHdr, IpDataLen);
		break;
	case 17://UDP
		break;
	default:
		;
	}

	return status;
}