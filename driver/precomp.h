#ifndef _PRE_COMP_H
#define _PRE_COMP_H


#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4047 4024 4996 4200 4100 4189)
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4054)   // cast of function pointer to PVOID
#pragma warning(disable:4244)   // conversion from 'int' to 'BOOLEAN', possible loss of data

#include <ndis.h>
#include <inaddr.h>
#include "passthru.h"


#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN 6
#endif

#define MAX_NDIS_DEVICE_NAME_LEN        256


#pragma pack(1)


// i386 is little_endian.
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN   (1)   //BYTE ORDER
#else
#error Redefine LITTLE_ORDER
#endif

//Mac头部，总长度14字节
typedef struct _ETH_HEADER
	{
	UCHAR DstAddr[MAC_ADDR_LEN];
	UCHAR SrcAddr[MAC_ADDR_LEN];
	USHORT      EthType;
	}ETH_HEADER, *PETH_HEADER;


//IP头部，总长度20字节
typedef struct _IP_HDR
{
	//#if LITTLE_ENDIAN
	//	UCHAR ihl;   //首部长度
	//	UCHAR version; //版本 
	//#else
	UCHAR versionAndihl; //版本（前四位）和首部长度（后四位）
	//#endif

	UCHAR tos;   //服务类型
	unsigned short tot_len; //总长度
	unsigned short id;    //标志
	unsigned short frag_off; //分片偏移
	UCHAR ttl;   //生存时间
	UCHAR protocol; //协议
	unsigned short chk_sum; //检验和
	//UCHAR srcaddr[4]; //源IP地址
	//UCHAR dstaddr[4]; //目的IP地址
	unsigned long srcaddr;
	unsigned long dstaddr;
}IP_HDR, *PIP_HDR;

typedef struct _PSD_HEADER

{

	unsigned long saddr; //源地址
	//UCHAR saddr[4];

	unsigned long daddr; //目的地址
	//UCHAR daddr[4];

	char mbz;//置空

	char ptcl; //协议类型

	unsigned short tcpl; //TCP长度

}PSD_HEADER,*PPSD_HEADER
;


//TCP头部，总长度20字节
typedef struct _TCP_HDR
	{
	unsigned short src_port;   //源端口号
	unsigned short dst_port;   //目的端口号
	unsigned int seq_no;    //序列号
	unsigned int ack_no;    //确认号

	//unsigned short nHLenAndFlag ;  // 前4位：TCP头长度；中6位：保留；后6位：标志位16bit
	unsigned char nHLen;
	unsigned char Flag;//后6位

	unsigned short wnd_size;   //16位窗口大小
	unsigned short chk_sum;   //16位TCP检验和
	unsigned short urgt_p;    //16为紧急指针
	}TCP_HDR, *PTCP_HDR;


//UDP头部，总长度8字节
typedef struct _UDP_HDR
	{
	unsigned short src_port; //源端口号
	unsigned short dst_port; //目的端口号
	unsigned short uhl;   //udp头部长度
	unsigned short chk_sum; //16位udp检验和
	}UDP_HDR, *PUDP_HDR;


//ICMP头部，总长度4字节
typedef struct _ICMP_HDR
	{
	UCHAR icmp_type;   //类型
	UCHAR code;    //代码
	unsigned short chk_sum;   //16位检验和
	}ICMP_HDR, *PICMP_HDR;

#pragma pack()





#endif//_PRE_COMP_H
