#ifndef _POLICY_LIST
#define _POLICY_LIST

//#include <ndis.h>


#define MAX_FORBID_PORT_NUM 512
#define BAD_PROT			-1

typedef struct _FORBIDE_TCP_PORT
{
	INT num;
	INT TcpPortList[MAX_FORBID_PORT_NUM];
}FORBIDE_TCP_PORT, *PFORBIDE_TCP_PORT;

typedef struct _FORBIDE_UDP_PORT
{
	INT num;
	INT UdpPortList[MAX_FORBID_PORT_NUM];
}FORBIDE_UDP_PORT, *PFORBIDE_UDP_PORT;

INT AddForbidTcpPort(INT iPort);
INT DeleteForbidTcpPort(INT iPort);
INT AddForbidUdpPort(INT iPort);
INT DeleteForbidUdpPort(INT iPort);

BOOLEAN IsForbidTcpPort(INT iPort, INT *iLocal);
BOOLEAN IsForbidUdpPort(INT iPort, INT *iLocal);

INT SearchBadTcpPortLocal();
INT SearchBadUdpPortLocal();
#endif//_POLICY_LIST







