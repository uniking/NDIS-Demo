#ifndef _NET_HOST_H
#define _NET_HOST_H
#include <ndis.h>

typedef unsigned short int uint16;

typedef unsigned long int uint32;

unsigned long int t_htonl(unsigned long int h);
unsigned long int t_ntohl(unsigned long int n);
unsigned short int t_htons(unsigned short int h);
unsigned short int t_ntohs(unsigned short int n);

#endif