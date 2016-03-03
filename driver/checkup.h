#ifndef _CHECK_UP_H
#define _CHECK_UP_H
#include "precomp.h"

//对数据包进行校验

//传入以太网包要校验的数据
ULONG EthCheckup(PUCHAR buffer, ULONG length);

//传入要校验的Tcp数据
//USHORT TcpCheckup(PUCHAR buffer, UINT length);

unsigned short chksum_tcp(unsigned short *h, unsigned short *d, int dlen);

USHORT checksum(USHORT* buffer, int size);

unsigned short myChecksum(PUCHAR buffer, UINT size);

unsigned short myChecksum2(PUCHAR h, UINT hsize,PUCHAR t, UINT tsize);

#endif