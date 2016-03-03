#include <ndis.h>


#define PASS_NET_WORK 0x122
#define FORBID_NET_WORK 0x123
#define STORAGE_PACKET 0x124 //¡Ÿ ±¥Ê¥¢
#define RECEIVE_STORAGE_PACKET 0x125

#define SEND_PACKET 0
#define RECEIVE_PACKET 1

NDIS_STATUS PacketAnalysis(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET pPacket, INT Type);
NDIS_STATUS EthAnalysis(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET pPacket, INT Type, PVOID pBuffer, INT length);
NDIS_STATUS IPAnalysis(PADAPT pAdapt, NDIS_HANDLE MiniportHandle, PNDIS_PACKET pPacket, INT Type, PVOID pBuffer, UINT length);
NDIS_STATUS TCPAnalysis(PVOID IpHeader, PVOID pBuffer, UINT length);
NDIS_STATUS UDPAnalysis(PVOID pBuffer);
NDIS_STATUS ICMPAnalysis(PVOID pBuffer);
NDIS_STATUS GetCurrentEnvironment();

BOOLEAN ndisClonePacket(PNDIS_PACKET OldPacket, PNDIS_PACKET* NewPacket, NDIS_HANDLE PacketPoolHandle);