/*
 *  field_server.c
 *  FluidApp
 */

#include "field_pvt.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fieldServerFree(void *i)
{
	fieldServer *r = (fieldServer*)i;
	netServer *ns = r->server;
	
	pthread_mutex_lock(&r->mtx);
	r->server = NULL;
	x_pthread_cond_signal(&r->cnd);
	pthread_mutex_unlock(&r->mtx);
	
	if (ns)		x_free(ns);
	
	if (r->fld_net)		x_free(r->fld_net);
	if (r->fld_local)	x_free(r->fld_local);
	
	pthread_cond_destroy(&r->cnd);
	pthread_mutex_destroy(&r->mtx);
}


int fieldServerOnConnect(void *i, netServer *s, netClient *c)
{
	fieldServer *r = (fieldServer*)i;
	struct fieldServerJitHeader header;
	
nextPacket:
	netClientGetBinary(c, &header, sizeof(header), 10);
	header.id = ntohl(header.id);
	
	if (header.id == 'JMTX')
	{
		struct fieldServerJitLatency latency;
		latency.id = htonl('JMLP');
		latency.parsed_header = x_time()*1000;
		
		//printf("FieldServer: Jitter Matrix!\n");
		struct fieldServerJitMatrix matrixInfo;
		int invByteOrder = 0;
		if (sizeof(matrixInfo) != header.size)
		{
			header.size = ntohl(header.size);
			if (header.size != sizeof(matrixInfo))
			{
				printf("FieldServer: ERROR: More data sent than expected! (sent %i expected %u)\n",
							header.size, (unsigned int)sizeof(matrixInfo));
				return 0;
			}
			else
				invByteOrder = 1;
		}
		
		netClientGetBinary(c, &matrixInfo, sizeof(matrixInfo), 10);
		matrixInfo.planeCount = ntohl(matrixInfo.planeCount);
		matrixInfo.type = ntohl(matrixInfo.type);
		matrixInfo.dimCount = ntohl(matrixInfo.dimCount);
		
		if (matrixInfo.dimCount < 2 || matrixInfo.dimCount > 32)
		{
			printf("FieldServer: ERROR: Insane dim count! (sent %i)\n", matrixInfo.dimCount);
			return 0;
		}
		
		int x;
		for (x=0; x<matrixInfo.dimCount; x++)
			matrixInfo.dim[x] = ntohl(matrixInfo.dim[x]);
		
		for (x=0; x<matrixInfo.dimCount; x++)
			matrixInfo.dimStride[x] = ntohl(matrixInfo.dimStride[x]);
		
		matrixInfo.dataSize = ntohl(matrixInfo.dataSize);
		
		//printf(" - planeCount: %u\n", matrixInfo.planeCount);
		//printf(" - dimCount: %u\n", matrixInfo.dimCount);
		//printf(" - time: %f\n", matrixInfo.time);
		
		//Loop over and receive!!!
		if (matrixInfo.dimCount == 2
			&& matrixInfo.type == FIELD_JIT_FLOAT32
			&& matrixInfo.planeCount == fieldComponents(r->fld_net)
			&& matrixInfo.dim[0] == fieldWidth(r->fld_net)
			&& matrixInfo.dim[1] == fieldHeight(r->fld_net)
			&& matrixInfo.dimStride[0] == fieldStrideX(r->fld_net)
			&& matrixInfo.dimStride[1] == fieldStrideY(r->fld_net)
			&& matrixInfo.dataSize ==
						matrixInfo.dim[0]*matrixInfo.dim[1]*
						matrixInfo.planeCount*4)
		{
			//printf(" - OPTIMAL!\n");
			float *d = fieldData(r->fld_net);
			int *invD = (int*)d;
			
			netClientGetBinary(c, d, matrixInfo.dataSize, 10);
			
			//if (invByteOrder)
			{
				for (x=0; x<matrixInfo.dataSize/4; x++)
				{
					invD[x] = ntohl(invD[x]);
				}
			}
			
			pthread_mutex_lock(&r->mtx);
			r->needSwap = 1;
			if (r->server == NULL)
			{
				pthread_mutex_unlock(&r->mtx);
				return 0;
			}
			x_pthread_cond_wait(&r->cnd, &r->mtx);
			
			if (r->server == NULL)
			{
				pthread_mutex_unlock(&r->mtx);
				return 0;
			}
			//printf("AWAKE!!\n");
			pthread_mutex_unlock(&r->mtx);
			
			latency.client_time = matrixInfo.time;
			latency.parsed_done = x_time()*1000;
			
			double diff = latency.parsed_header - matrixInfo.time;
			
			latency.parsed_header -= diff;
			latency.parsed_done -= diff;
			
			diff = (latency.parsed_done-latency.parsed_header)/2;
			latency.parsed_header += diff;
			latency.parsed_done += diff;
			
			//printf("LATENCY (%f,%f,%f)\n",latency.client_time,
			//							latency.parsed_done,
			//							latency.parsed_header);
			header.id = 'JMLP';
			header.size = sizeof(latency);
			//netClientSendBinary(c, &header, sizeof(header));
			netClientSendBinary(c, &latency, sizeof(latency));
			
			goto nextPacket;
		}
		else
		{
			printf(" - Backup data fetcher = discard...\n");
			printf(" --> Type: %u\n", matrixInfo.type);
			printf(" --> Dims: %u\n", matrixInfo.dimCount);
			printf(" --> Planes: %u\n", matrixInfo.planeCount);
			printf(" --> Width: %u\n", matrixInfo.dim[0]);
			printf(" --> Height: %u\n", matrixInfo.dim[1]);
			printf(" --> StrideX: %u\n", matrixInfo.dimStride[0]);
			printf(" --> StrideY: %u\n", matrixInfo.dimStride[1]);
			printf(" --> dataSize: %u\n", matrixInfo.dataSize);
		}
		
	}
	else if (header.id == 'JMLP')
	{
		printf("FieldServer: Jitter Latency!\n");
	}
	else if (header.id == 'JMMP')
	{
		printf("FieldServer: Jitter Message!\n");
	}
	else
	{
		printf("FieldServer: ERROR: Unknown Packet!\n");
		return 0;
	}
	
	printf("FieldServer: Completed Processing!\n");
	return 0;
}


fieldServer *fieldServerCreate(int in_width, int in_height, int in_components,
							   int in_port)
{
	fieldServer *r = x_malloc(sizeof(fieldServer), fieldServerFree);
	
	memset(r, 0, sizeof(fieldServer));
	
	r->fld_net = fieldCreate(in_width, in_height, in_components);
	r->fld_local = fieldCreate(in_width, in_height, in_components);
	
	pthread_mutex_init(&r->mtx, NULL);
	x_pthread_cond_init(&r->cnd, NULL);
	
	char szPort[64];
	snprintf(szPort, 64, "%i", in_port);
	
	r->server = netServerCreate(szPort, NETS_TCP | NETS_SINGLE_CLIENT, r, fieldServerOnConnect);
	
	return r;
}

field *fieldServerLock(fieldServer *fs)
{
	return fs->fld_local;
}

void fieldServerUnlock(fieldServer *fs)
{
	pthread_mutex_lock(&fs->mtx);
	if (fs->needSwap == 1)
	{
		field *tmp = fs->fld_net;
		fs->fld_net = fs->fld_local;
		fs->fld_local = tmp;
		fs->needSwap = 0;
		pthread_cond_signal(&fs->cnd);
	}
	pthread_mutex_unlock(&fs->mtx);
}


void fieldServerSetDelegate(fieldServer *fs, netServerDelegate *d)
{
	netServerSetDelegate(fs->server, d);
}
