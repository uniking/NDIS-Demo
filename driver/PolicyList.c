#include "precomp.h"
#include "PolicyList.h"


FORBIDE_TCP_PORT gForbidTcpPortList;
FORBIDE_UDP_PORT gForbidUdpPortList;

INT AddForbidTcpPort(INT iPort)
{
	INT iRnt = 0;
	INT iBadPortLacal = MAX_FORBID_PORT_NUM + 1;
	do 
	{
		if (gForbidTcpPortList.num >= MAX_FORBID_PORT_NUM)
		{
			break;
		}

		if (IsForbidTcpPort(iPort, NULL))
		{
			break;
		}

		iBadPortLacal = SearchBadTcpPortLocal();
		if (iBadPortLacal >= MAX_FORBID_PORT_NUM)
		{
			break;
		}

		if (iBadPortLacal >= gForbidTcpPortList.num)
		{
			gForbidTcpPortList.num++;
		}
		
		gForbidTcpPortList.TcpPortList[iBadPortLacal] = iPort;
	} while (FALSE);

	return iRnt;
}
INT DeleteForbidTcpPort(INT iPort)
{
	INT i = 0;
	if (!IsForbidTcpPort(iPort, &i))
	{
		return 1;
	}

	if (i >= gForbidTcpPortList.num)
	{
		gForbidTcpPortList.num--;
	}
	gForbidTcpPortList.TcpPortList[i] = BAD_PROT;
	return 0;
}
INT AddForbidUdpPort(INT iPort)
{
	INT iRnt = 0;
	INT iBadPortLacal = MAX_FORBID_PORT_NUM + 1;
	do 
	{
		if (gForbidUdpPortList.num >= MAX_FORBID_PORT_NUM)
		{
			break;
		}

		if (IsForbidUdpPort(iPort, NULL))
		{
			break;
		}

		iBadPortLacal = SearchBadUdpPortLocal();
		if (iBadPortLacal >= MAX_FORBID_PORT_NUM)
		{
			break;
		}

		if (iBadPortLacal >= gForbidUdpPortList.num)
		{
			gForbidUdpPortList.num++;
		}

		gForbidUdpPortList.UdpPortList[iBadPortLacal] = iPort;
	} while (FALSE);

	return iRnt;
}
INT DeleteForbidUdpPort(INT iPort)
{
	INT i = 0;
	if (!IsForbidUdpPort(iPort, &i))
	{
		return 1;
	}

	if (i >= gForbidUdpPortList.num)
	{
		gForbidUdpPortList.num--;
	}
	gForbidUdpPortList.UdpPortList[i] = BAD_PROT;
	return 0;
}

BOOLEAN IsForbidTcpPort(INT iPort, INT *iLocal)
{
	INT portnum = gForbidTcpPortList.num;
	INT i = 0;
	BOOLEAN bRnt = FALSE;

	while(portnum > 0)
	{
		if (gForbidTcpPortList.TcpPortList[i] == iPort)
		{
			if (iLocal != NULL)
			{
				*iLocal = i;
			}
			bRnt = TRUE;
			break;
		}
		portnum--;
		i++;
	}

	return bRnt;
}
BOOLEAN IsForbidUdpPort(INT iPort, INT *iLocal)
{
	INT portnum = gForbidUdpPortList.num;
	INT i = 0;
	BOOLEAN bRnt = FALSE;

	while(portnum > 0)
	{
		if (gForbidUdpPortList.UdpPortList[i] == iPort)
		{
			if (iLocal != NULL)
			{
				*iLocal = i;
			}
			bRnt = TRUE;
			break;
		}
		portnum--;
		i++;
	}

	return bRnt;
}

INT SearchBadTcpPortLocal()
{
	INT i = 0;
	while (i < MAX_FORBID_PORT_NUM)
	{
		if (gForbidTcpPortList.TcpPortList[i] == BAD_PROT)
		{
			break;
		}
		i++;
	}

	return i;
}

INT SearchBadUdpPortLocal()
{
	INT i = 0;
	while (i < MAX_FORBID_PORT_NUM)
	{
		if (gForbidUdpPortList.UdpPortList[i] == BAD_PROT)
		{
			break;
		}
		i++;
	}

	return i;
}