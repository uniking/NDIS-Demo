#include "checkup.h"



ULONG crc32(PUCHAR ptr, UINT len)
{
	ULONG g_crc =0;
	UCHAR i;
	while(len--)
	{
		for (i=0x01; i!=0; i<<=1)
		{
			if ((g_crc&80000000) != 0)
			{
				g_crc<<=1;
				g_crc^=0x04c11db7;
			}
			else
			{
				g_crc<<=1;
			}
		}
		ptr++;
	}

	return g_crc;
}

//传入以太网包要校验的数据
//ULONG EthCheckup(PUCHAR buffer, ULONG length)
//{
//	PUCHAR ptr;
//	//g_crc = 0xffffffff;
//
//	return crc32(buffer, length);
//}

//USHORT checksum (USHORT *buffer,int size)
//{
//	ULONG cksum=0;
//	while(size>1)
//	{
//		Cksum +=*buffer++;
//		size -=sizeof(USHORT);
//	}
//	if(size)
//	{
//		Cksum +=*(UCHAR *) buffer;
//	}
//	//将32位转换为16位
//	while (cksum>>16)
//	{
//		Cksum = (cksum>>16) + (cksum & 0xffff);
//	}
//	return (USHORT) (~cksum);
//}

////传入要校验的Tcp数据
//USHORT TcpCheckup(PUCHAR buffer, UINT length)
//{
//	return checksum (buffer,length);
//}


unsigned short chksum_tcp(unsigned short *h, unsigned short *d, int dlen)
{
	unsigned int cksum = 0;
	unsigned short answer=0;

	/* PseudoHeader must have 12 bytes */
	cksum  = h[0];
	cksum += h[1];
	cksum += h[2];
	cksum += h[3];
	cksum += h[4];
	cksum += h[5];

	/* TCP hdr must have 20 hdr bytes */
	cksum += d[0];
	cksum += d[1];
	cksum += d[2];
	cksum += d[3];
	cksum += d[4];
	cksum += d[5];
	cksum += d[6];
	cksum += d[7];
	cksum += d[8];
	cksum += d[9];

	dlen  -= 20; /* bytes   */
	d     += 10; /* short's */ 

	while(dlen >=32)
	{
		cksum += d[0];
		cksum += d[1];
		cksum += d[2];
		cksum += d[3];
		cksum += d[4];
		cksum += d[5];
		cksum += d[6];
		cksum += d[7];
		cksum += d[8];
		cksum += d[9];
		cksum += d[10];
		cksum += d[11];
		cksum += d[12];
		cksum += d[13];
		cksum += d[14];
		cksum += d[15];
		d     += 16;
		dlen  -= 32;
	}

	while(dlen >=8)  
	{
		cksum += d[0];
		cksum += d[1];
		cksum += d[2];
		cksum += d[3];
		d     += 4;   
		dlen  -= 8;
	}

	while(dlen > 1)
	{
		cksum += *d++;
		dlen  -= 2;
	}

	if( dlen == 1 ) 
	{ 
		/* printf("new checksum odd byte-packet\n"); */
		*(unsigned char*)(&answer) = (*(unsigned char*)d);

		/* cksum += (u_int16_t) (*(u_int8_t*)d); */

		cksum += answer;
	}

	cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
	cksum += (cksum >> 16);

	return (unsigned short)(~cksum);

}

USHORT checksum(USHORT* buffer, int size)
{
	unsigned long cksum = 0;
	while(size>1)
	{
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}

	if(size)
	{
		cksum += *(UCHAR*)buffer;
	}

	//将32位数转换成16
	while(cksum>>16)
		cksum = (cksum>>16) + (cksum&0xffff);  //将高16bit与低16bit相加
	//cksum += (cksum>>16);             //将进位到高位的16bit与低16bit 再相加
	return (USHORT)(~cksum);
}

unsigned short myChecksum(PUCHAR buffer, UINT size)
{
	UINT index = 0;
	unsigned short uschecksum = 0;
	while(index<size)
	{
		uschecksum += ~buffer[index];
		index++;
	}

	return uschecksum;
}


unsigned short myChecksum2(PUCHAR h, UINT hsize,PUCHAR t, UINT tsize)
{
	NDIS_HANDLE pPoo;
	NDIS_STATUS nStatus;
	PUCHAR myBuffer;
	unsigned short uschecksum = 0;
	NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;

	HighestAcceptableMax.QuadPart = -1;
	nStatus = NdisAllocateMemory(&myBuffer, hsize+tsize, 0, HighestAcceptableMax);
	if (nStatus != NDIS_STATUS_SUCCESS)
	{
		return 0;
	}

	NdisMoveMemory(myBuffer, h, hsize);
	NdisMoveMemory(myBuffer+hsize, t, tsize);

	uschecksum = checksum((USHORT *)myBuffer, hsize+tsize);
	NdisFreeMemory(myBuffer, hsize+tsize, 0);
	return uschecksum;
}