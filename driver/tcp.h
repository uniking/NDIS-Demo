#ifndef _TCP_H
#define _TCP_H
#include <ndis.h>
#include "passthru.h"
#include "precomp.h"
#include "PacketManager.h"


enum tcp_state {
	CLOSED      = 0,
	LISTEN      = 1,
	SYN_SENT    = 2,
	SYN_RCVD    = 3,
	ESTABLISHED = 4,
	FIN_WAIT_1  = 5,
	FIN_WAIT_2  = 6,
	CLOSE_WAIT  = 7,
	CLOSING     = 8,
	LAST_ACK    = 9,
	TIME_WAIT   = 10
};


typedef struct _TCP_PCB//协议控制块
{
	LIST_ENTRY pcbList;
	unsigned long srcaddr;
	unsigned long dstaddr;
	unsigned short src_port;   //源端口号
	unsigned short dst_port;   //目的端口号

	LIST_ENTRY ooseq;//数据段无序队列

	unsigned int rcv_nxt;//期望接收的下一个字节
	ULONG lastack;//最近一次的确认序列
	ULONG rcv_wnd;

	int status;//tcp连接状态
	ULONG deletetimer;//定时器，用于删除pcb


}TCP_PCB, *PTCP_PCB;

#define TCP_ACK 0x10
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_HLEN 0xF0

#define TCPH_ACK(h) (h->Flag & TCP_ACK)
#define TCPH_PSH(h) (h->Flag & TCP_PSH)
#define TCPH_RST(h) (h->Flag & TCP_RST)
#define TCPH_SYN(h) (h->Flag & TCP_SYN)
#define TCPH_FIN(h) (h->Flag & TCP_FIN) 

#define TCPH_LEN(h) ((h->nHLen & TCP_HLEN)>>4)
#define TCPH_SEQNO(h) (h->seq_no)
#define TCPH_ACKNO(h) (h->ack_no)
#define TCPH_WND(h) (h->wnd_size)
#define TCPH_SRCPORT(h) (h->src_port)
#define TCPH_DSTPORT(h) (h->dst_port)

typedef  UINT u32_t;
typedef INT s32_t;
#define TCP_SEQ_LT(a,b)     ((s32_t)((u32_t)(a) - (u32_t)(b)) < 0)
#define TCP_SEQ_LEQ(a,b)    ((s32_t)((u32_t)(a) - (u32_t)(b)) <= 0)
#define TCP_SEQ_GT(a,b)     ((s32_t)((u32_t)(a) - (u32_t)(b)) > 0)
#define TCP_SEQ_GEQ(a,b)    ((s32_t)((u32_t)(a) - (u32_t)(b)) >= 0)
#define TCP_SEQ_BETWEEN(a,b,c) (TCP_SEQ_GEQ(a,b) && TCP_SEQ_LEQ(a,c))

void tcp_init();
int tcp_process(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_TCP pPacket, INT Type, PVOID pBuffer, UINT length);
int tcp_recevie(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_TCP pPacket, INT Type, PVOID pBuffer, UINT length);


#endif//_TCP_H