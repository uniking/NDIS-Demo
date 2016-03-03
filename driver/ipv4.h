#ifndef _IP_V4_H
#define _IP_V4_H

#include <ndis.h>
#include "precomp.h"
#include "PacketManager.h"

#define TAG_IPV4 'ytty'
#define IP_HLEN 20

#define IP_REASS_FLAG_LASTFRAG 0x01
#define IP_RF 0x8000U        /* reserved fragment flag */
#define IP_DF 0x4000U        /* dont fragment flag */
#define IP_MF 0x2000U        /* more fragments flag */
#define IP_MF_H 0x20  //主机字节序的掩码
#define IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */
#define IP_OFFMASK_H 0xff1fU   /* 主机字节序的掩码 */

typedef struct _ipasfrag
{
	//struct _ipasfrag* ipf_next;
	//struct _ipasfrag* ipf_prev;
	LIST_ENTRY ipf;

	PNDIS_PACKET pPacket;//是以太网包
	IP_HDR pIpHdr;//指向ip头
	int datagram_offset;
	int datagram_len;
	char flags;
	char timer;
}ipasfrag, *pipasfrag;

typedef struct _ipq
{
	

	LIST_ENTRY ipqueue;//指向一类IP

	LIST_ENTRY datagram;//IP的id相同的数组
	
	//pipasfrag ipq_next;
	//pipasfrag ipq_prev;

	//struct _ipq* next;
	//struct _ipq* prev;

	IP_HDR iphdr;
	ULONG datagram_len;
	CHAR flags;
	CHAR timer;


}ipq, *pipq;




#define IPH_HL(hdr) ((hdr)->versionAndihl & 0x0f)
#define IPH_TOS(hdr) ((hdr)->tos)
#define IPH_LEN(hdr) ((hdr)->tot_len)
#define IPH_MORE_FRAGMENT(hdr)( (((hdr)->frag_off)&IP_MF_H)==0?0:1 )
#define IPH_OFFSET(hdr) (((hdr)->frag_off)&IP_OFFMASK_H) 
#define IPH_TTL(hdr) ((hdr)->ttl)
#define IPH_PROTO(hdr) ((hdr)->protocol)
#define IPH_CHKSUM(hdr) ((hdr)->chk_sum)
#define IPH_ID(hdr)((hdr)->id)


#define PP_HTONS(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define PP_NTOHS(x) PP_HTONS(x)

#define ip_addr_cmp(addr1, addr2) (addr1 == addr2)

#define IP_ADDRESSES_AND_ID_MATCH(iphdrA, iphdrB) \
	(ip_addr_cmp(&(iphdrA)->srcaddr, &(iphdrB)->srcaddr) && \
	ip_addr_cmp(&(iphdrA)->dstaddr, &(iphdrB)->dstaddr) && \
	IPH_ID(iphdrA) == IPH_ID(iphdrB)) ? 1 : 0

void InitalizeIpv4();
BOOLEAN IsMultipleFragments(PIP_HDR iphdr);
pipq ip_reass(PNDIS_PACKET p, PVOID buffer);

int ip_receive(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET_IP pPacket, INT Type, PVOID pBuffer, UINT length);

#endif//_IP_V4_H