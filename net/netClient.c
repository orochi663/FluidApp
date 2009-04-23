/*
 *  netClient.c
 *  FluidApp
 */

#include "netClient.h"

#include <stdlib.h>

#include <string.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "memory.h"


error *netClientSendBinary(netClient *client, const void *base, int cnt)
{
	int totalSent = 0;
	
	while (totalSent < cnt)
	{
		int sent = send(client->m_socket, (char*)base+totalSent, cnt-totalSent, 0);
		
		if (sent == -1)
			return errorCreate(NULL, error_net, "failed sending data over network");
		
		totalSent += sent;
	}
	
	return NULL;
}


error *netClientReadBinary(netClient *client, void *base, int *cnt, int timeout)
{
	fd_set selectSet;
	fd_set copySet;
	
	FD_ZERO(&selectSet);
	FD_SET(client->m_socket, &selectSet);
	
	int bufferSize = *cnt;
	*cnt = 0;
	
	FD_COPY(&selectSet, &copySet);
	
	struct timeval to;
	to.tv_sec = timeout;
	to.tv_usec = 0;
	
	int sel = select(client->m_socket+1, &copySet, NULL, NULL, &to);
	
	if (sel == -1)
		return errorCreate(NULL, error_net, "Network error while waiting for data");
	
	if (sel != 0)
	{
		*cnt = read(client->m_socket, base, bufferSize);
	}
	
	if (*cnt == -1)
		return errorCreate(NULL, error_net, "Network error while reading data");
	
	return NULL;
}


error *netClientGetBinary(netClient *client, void *dest, int cnt, int timeout)
{
	int curRead = 0;
	
	for (;;)
	{
		int curCnt = cnt - curRead;
		
		error *tmp = netClientReadBinary(client,
							 (char*)dest + curRead, &curCnt, timeout);
		
		if (tmp != NULL)
			return tmp;
		
		if (curCnt == 0 && curRead == 0)
			return errorCreate(NULL, error_timeout,
							   "Timed out while receiving data...");
		else if (curCnt == 0)
			return errorCreate(NULL, error_net,
							   "Timed out while waiting for remaining data");
		
		curRead += curCnt;
		if (curCnt == cnt)
			return NULL;
	}
}


void netClientFree(void *in_o)
{
	netClient *client = (netClient*)in_o;
	if (client->m_socket != -1)
		close(client->m_socket);
	
	memset(client, 0, sizeof(netClient));
}

netClient *netClientFromSocket(int in_socket)
{
	netClient *toRet = x_malloc(sizeof(netClient), netClientFree);
	if (toRet)
	{
		memset(toRet, 0, sizeof(netClient));
		toRet->m_socket = in_socket;
	}
	return toRet;
}


netClient *netClientCreate(const char *address, char *port, int flags,
						   error **out_e)
{
	if (!(flags&NETS_UDP) && !(flags&NETS_TCP))
	{
		*out_e = errorCreate(NULL, error_flags,
					"Client needs at least UDP or TCP communication");
		return NULL;
	}
	
	struct addrinfo hints;
	struct addrinfo *servinfo;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = (flags&NETS_UDP)?SOCK_DGRAM:SOCK_STREAM;
	hints.ai_protocol = (flags&NETS_UDP)?IPPROTO_UDP:IPPROTO_TCP;
	
	if (getaddrinfo(address, port, &hints, &servinfo) != 0)
	{
		*out_e = errorCreate(NULL, error_net,
					"Client unable to resolve remote host");
		return NULL;
	}
	
	int mySocket = socket(	servinfo->ai_family,
							servinfo->ai_socktype,
							servinfo->ai_protocol);
	if (mySocket == -1)
	{
		*out_e = errorCreate(NULL, error_create,
					"Client unable to create socket");
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	if (connect(mySocket, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		*out_e = errorCreate(NULL, error_net,
					"Client unable to connect to remote server");
		freeaddrinfo(servinfo);
		close(mySocket);
		return NULL;
	}
	
	freeaddrinfo(servinfo);
	
	return netClientFromSocket(mySocket);
}
