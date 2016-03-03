#include "tcp.h"
#include "nh.h"
#include "app.h"
#include "Function.h"

LIST_ENTRY g_tcp_pcb;
NDIS_SPIN_LOCK  g_tcp_pcb_SpinLock;

void tcp_init()
{
	NdisInitializeListHead(&g_tcp_pcb);
	NdisAllocateSpinLock(&g_tcp_pcb_SpinLock);
}

BOOLEAN tcp_ip_and_port_match(PTCP_PCB a, PTCP_PCB b)
{
	if (a->srcaddr == b->srcaddr &&
		a->src_port == b->src_port &&
		a->dstaddr == b->dstaddr &&
		a->dst_port == b->dst_port)
	{
		return TRUE;
	}

	return FALSE;
}


int tcp_send(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_TCP pPacket, INT Type, PVOID pBuffer, UINT length)
{
	return 0;
}


int tcp_recevie(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_TCP pPacket, INT Type, PVOID pBuffer, UINT length)
{
	PTCP_HDR pTcpHdr = (PTCP_HDR)pBuffer;
	int status = 0;
	UINT hlen = 0;
	UINT tcplen = 0;//tcp数据部分的长度
	unsigned int seqno;
	unsigned int ack_no;
	unsigned short wnd_size;
	PTCP_PCB pcb = NULL;
	PTCP_PCB TempPcb = NULL;
	PLIST_ENTRY TempNode = NULL;
	PLIST_ENTRY PreNode = NULL;
	PNDIS_PACKET_TCP TempPacket = NULL;
	PNDIS_PACKET_APP AppPacket = NULL;
	PCHAR pData = NULL;

	if (NDIS_STATUS_SUCCESS != NdisAllocateMemoryWithTag(&pcb, sizeof(TCP_PCB), PM_TAG))
	{
		return 0;
	}

	hlen = TCPH_LEN(pTcpHdr)*4;
	pPacket->tcplen = tcplen = pPacket->TotLen - hlen;
	pPacket->seq_no = seqno = t_ntohl(TCPH_SEQNO(pTcpHdr));
	pPacket->ack_no = ack_no = t_ntohl(TCPH_ACKNO(pTcpHdr));
	
	wnd_size = t_ntohs(TCPH_WND(pTcpHdr));

	pcb->srcaddr = pPacket->srcaddr;
	pcb->src_port = t_ntohs(TCPH_SRCPORT(pTcpHdr));
	pcb->dstaddr = pPacket->dstaddr;
	pcb->dst_port = t_ntohs(TCPH_DSTPORT(pTcpHdr));
	InitializeListHead(&pcb->ooseq);
	InitializeListHead(&pcb->pcbList);
	pcb->lastack = 0;
	
	pcb->rcv_wnd = wnd_size;

	//先找对应的PCB
	TempNode = g_tcp_pcb.Flink;
	for (;TempNode != &g_tcp_pcb; TempNode = TempNode->Flink)
	{
		TempPcb = CONTAINING_RECORD(TempNode, TCP_PCB, pcbList);
		if (tcp_ip_and_port_match(TempPcb, pcb))
		{
			break;
		}
	}

	if (TempNode == &g_tcp_pcb)
	{//没找到对应pcb，使用pcb
		pcb->status = CLOSED;
		NdisInterlockedInsertHeadList(
			&g_tcp_pcb,
			&pcb->pcbList,
			&g_tcp_pcb_SpinLock
			);
	}
	else
	{//找到对应PCB，
		PMFreeMemery(pcb, sizeof(TCP_PCB));
		pcb = TempPcb;
	}


	//////////////////////////////////////////////////////////////////////////
	KdPrint(("rst=%x psh=%x ack=%x fin=%x syn=%x\n", 
		TCPH_RST(pTcpHdr),
		TCPH_PSH(pTcpHdr),
		TCPH_ACK(pTcpHdr),
		TCPH_FIN(pTcpHdr),
		TCPH_SYN(pTcpHdr)
		));

	if (TCPH_SYN(pTcpHdr)&& !TCPH_ACK(pTcpHdr))
	{//服务器，接收到连接请求
		pcb->rcv_wnd = wnd_size;
		pcb->rcv_nxt = t_ntohl(TCPH_SEQNO(pTcpHdr));
		pcb->rcv_nxt++;

		//下一步应该是，收到ack确认
		pcb->status = SYN_RCVD;
		return 0;
	}

	if (TCPH_SYN(pTcpHdr) && TCPH_ACK(pTcpHdr))
	{//客户端，接收到连接确认
		pcb->rcv_wnd = wnd_size;
		pcb->rcv_nxt = t_ntohl(TCPH_SEQNO(pTcpHdr));
		pcb->rcv_nxt++;

		//下一步应该是，收到服务器发来的确认
		pcb->status = ESTABLISHED;
		return 0;
	}

	if (TCPH_FIN(pTcpHdr) && TCPH_ACK(pTcpHdr))
	{
		pcb->status = LAST_ACK;
		return 0;
	}

	if (pcb->status == LAST_ACK)
	{
		pcb->status = CLOSED;
		return 0;
	}

	if (pcb->status == SYN_RCVD)
	{//服务器收到，客户端的确认
		pcb->rcv_nxt = t_ntohl(TCPH_SEQNO(pTcpHdr));
		pcb->status = ESTABLISHED;
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	if (pcb->status != ESTABLISHED)
	{
		return 0;
	}


	//if (tcplen >=6)
	//{
	//	pData = (PCHAR)pTcpHdr;
	//	pData+=hlen;

	//	KdPrint(("seqno=%x  tcplen=%x  rcv_nxt=%x  rcv_wnd=%x  h[6]=%x %x %x %x %x %x t[6]=%x %x %x %x %x %x\n",
	//		seqno, tcplen, pcb->rcv_nxt, pcb->rcv_wnd,
	//		pData[0], pData[1], pData[2], pData[3], pData[4], pData[5],
	//		pData[tcplen-6], pData[tcplen-5], pData[tcplen-4], pData[tcplen-3], pData[tcplen-2], pData[tcplen-1]));
	//}
	//else
	//{
	//	KdPrint(("seqno=%x  tcplen=%x  rcv_nxt=%x  rcv_wnd=%x\n",
	//		seqno, tcplen, pcb->rcv_nxt, pcb->rcv_wnd));
	//}

	//if (tcplen == 6)
	//{
	//	if (TCPH_PSH(pTcpHdr))
	//	{//为什么呢？？？
	//		tcplen-=2;
	//		pPacket->tcplen-=2;
	//		KdPrint(("666\n"));
	//	}
	//}

	//处理接收的数据
	if (tcplen > 0)
	{
		if (TCP_SEQ_BETWEEN(pcb->rcv_nxt, seqno + 1, seqno + tcplen - 1))
		{//必须消除重叠数据
			KdPrint(("--between\n"));
		}
		else
		{
			if (TCP_SEQ_LT(seqno, pcb->rcv_nxt))
			{
				//释放pcb
				return 0;
			}
		}

		
		
		if (TCP_SEQ_BETWEEN(seqno, pcb->rcv_nxt, 
			pcb->rcv_nxt + pcb->rcv_wnd - 1))
		{
			if (pcb->rcv_nxt == seqno)
			{//有序段
				if (pcb->ooseq.Flink != pcb->ooseq.Blink)
				{//这段代码没用，
					TempPacket = CONTAINING_RECORD(&(pcb->ooseq.Flink), NDIS_PACKET_TCP, TcpDataList);
					ASSERT((pPacket->seq_no+tcplen) < TempPacket->seq_no);
				}
				

				NdisInterlockedInsertHeadList(&pcb->ooseq, &pPacket->TcpDataList, &g_tcp_pcb_SpinLock);

				//向上层提交数据包
				TempNode = pcb->ooseq.Flink;
				for (;TempNode != &pcb->ooseq; TempNode = PreNode)
				{
					TempPacket = CONTAINING_RECORD(TempNode, NDIS_PACKET_TCP, TcpDataList);
					PreNode = TempNode->Flink;
					if (pcb->rcv_nxt != TempPacket->seq_no)
					{
						break;
					}

					AppPacket = PMAllocatePacket(PM_PACKET_APP);
					if (AppPacket == NULL)
					{
						break;
					}

					AppPacket->srcIp = pcb->srcaddr;
					AppPacket->srcPort = pcb->src_port;
					AppPacket->desIp = pcb->dstaddr;
					AppPacket->desPort = pcb->dst_port;
					InitializeListHead(&AppPacket->appList);
					AppPacket->data = pPacket;
					
					//移除第一个，发给上层
					RemoveEntryList(TempNode);
					pcb->rcv_nxt = TempPacket->seq_no+TempPacket->tcplen;
					KdPrint(("tcp_receive -> app_receive\n"));
					status = app_receive(pAdapt, MiniportHandle, AppPacket, Type, pBuffer, length);

				}


			}
			else
			{//插入到无序队列
				//找到合适位置插入包，暂不处理数据包重叠
				TempNode = pcb->ooseq.Flink;
				for(;TempNode != &pcb->ooseq; TempNode = TempNode->Flink)
				{
					TempPacket = CONTAINING_RECORD(TempNode, NDIS_PACKET_TCP, TcpDataList);
					if (pPacket->seq_no < TempPacket->seq_no)
					{
						break;
					}
				}

				if (TempNode != &pcb->ooseq)
				{
					ASSERT((pPacket->seq_no+pPacket->tcplen) <= TempPacket->seq_no);

					NdisInterlockedInsertTailList(
						&TempPacket->TcpDataList,
						&pPacket->TcpDataList,
						&g_tcp_pcb_SpinLock
						);
				}
				else
				{//没找到
					NdisInterlockedInsertTailList(
						&pcb->ooseq,
						&pPacket->TcpDataList,
						&g_tcp_pcb_SpinLock
						);
				}

				KdPrint(("tcp_recevie STORAGE_PACKET\n"));
				status = STORAGE_PACKET;
			}
		}
		else
		{
			status = 0;
		}
	}

	return status;
}

//创建pcb，处理tcp连接状态
int tcp_process(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_TCP pPacket, INT Type, PVOID pBuffer, UINT length)
{
	PTCP_PCB pTcpPcb = NULL;
	INT Status = 0;

	switch(pTcpPcb->status)
	{
	case SYN_SENT:
		break;
	case SYN_RCVD:
		break;
	case ESTABLISHED:
		break;
	case FIN_WAIT_1:
		break;
	case FIN_WAIT_2:
		break;
	case CLOSE_WAIT:
		break;
	case CLOSING:
		break;
	case LAST_ACK:
		break;
	case TIME_WAIT:
		break;
	case LISTEN:
		break;
	}

	return Status;
}