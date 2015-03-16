/*
 * libds
 * Copyright (c) Chunfeng Zhang <CrazyPandar@gmail.com>, All rights reserved.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FIDSESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>

#include "ds_plat_basic.h"
#include "ds_utils.h"
#include "ds_event.h"
#include "ds_stream.h"
#include "ds_tcp_client.h"
#include "lds_log.h"

struct _DSTcpClient {
    DSStream _strm;
    uint8_t ip[4];
    uint16_t port;
    
    int sock;
    struct event *connEvt;
    struct event *rdEvt;
    struct event *wrtEvt;
    
    enum {
        DS_TCP_STRM_ST_NONE,
        DS_TCP_STRM_ST_CONNECTING,
        DS_TCP_STRM_ST_CONNECTED,
    }st;
    
    uint8_t buf[1024*4];
};

static void _DSTcpClientDestroy(DSTcpClient* tc);

static int DSTcpClientSend(DSStream* strm, uint8_t* buf, int bufSz)
{
    DSTcpClient *tc;
    int writed;
    
    LDS_DBG_IN_FUNC();
    tc = (DSTcpClient*)strm;
    
    writed = write(tc->sock, buf, bufSz);
    
    if (writed > 0) {
        if (event_add(tc->wrtEvt, NULL)) {
            LDS_ERR("event_add() failed\n");
        }
    } else if (writed == 0) {
        
    } else {
        
    }
    return writed;
}

static int setnonblock(int fd)
{
	int flags;
	
	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		return -1;
	}
	return 0;
}


static void ReadCb(int fd, short ev, void *arg)
{
    DSTcpClient *tc;
    int readed;
    
    LDS_DBG_IN_FUNC();
    tc = arg;
    
    readed = read(fd, tc->buf, sizeof(tc->buf));
    
    if (readed > 0) {
        struct DSConstBuf rd = {
            .buf = tc->buf,
            .size = readed
        };
        
        DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_RECVED, &rd);
    } else if (readed == 0) {
        DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_DISCONNECTED, NULL);
    } else {
        DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_ERROR, NULL);
    }
}

static void WriteCb(int fd, short ev, void *arg)
{
    DSTcpClient *tc;
    
    LDS_DBG_IN_FUNC();
    tc = arg;
    
    if (event_del(tc->wrtEvt)) {
        LDS_ERR("devent_del failed\n");
    }
    DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_SENT, NULL);
}


static void ConnCb(int fd, short ev, void *arg)
{
    DSTcpClient *tc;
    DSEventBase *evtBase;
    
    LDS_DBG_IN_FUNC();
    tc = arg;

    event_del(tc->connEvt);
    event_free(tc->connEvt);
    
    evtBase = DSStreamGetEventBase((DSStream*)tc);
    
    if (!(tc->rdEvt = event_new(evtBase, tc->sock, EV_READ | EV_PERSIST, ReadCb, tc))) {
        LDS_ERR_OUT(ERR_OUT, "event_new(EV_READ) failed\n");
    }

    if (!(tc->wrtEvt = event_new(evtBase, tc->sock, EV_WRITE | EV_PERSIST, WriteCb, tc))) {
        LDS_ERR_OUT(ERR_FREE_RD_EVT, "event_new(EV_WRITE) failed\n");
    }
    
    if (event_add(tc->rdEvt, NULL)) {
         DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_ERROR, "event_add(&tc->rdEvt, NULL) failed");
         return;
    }
    
    tc->st = DS_TCP_STRM_ST_CONNECTED;
    DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_CONNECTED, NULL);
    
    return;
ERR_DEL_RD_EVT:
    event_del(tc->rdEvt);
ERR_FREE_WRT_EVT:
    event_free(tc->wrtEvt);
ERR_FREE_RD_EVT:
    event_free(tc->rdEvt);
ERR_OUT:
    return;
}

#define DS_MAKE_S_ADDR_FROM_ARRAY(__ip)    (__ip[0]|__ip[1]<<8|__ip[2]<<16|__ip[3]<<24)

static int DSTcpClientConnect(DSStream* strm)
{
    struct sockaddr_in sa;
    DSTcpClient *tc;
    DSEventBase *evtBase;
    
    LDS_DBG_IN_FUNC();
    tc = (DSTcpClient*)strm;
    
    evtBase = DSStreamGetEventBase((DSStream*)tc);
	if ((tc->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LDS_ERR_OUT(ERR_OUT, "socket() failed\n");
	}

	if (setnonblock(tc->sock)) {
		LDS_ERR_OUT(ERR_CLOSE_SOCK, "setnonblock() failed\n");
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(tc->port);
    
	sa.sin_addr.s_addr = DS_MAKE_S_ADDR_FROM_ARRAY(tc->ip);    /* inet_addr("127.0.0.1");*/

    if (connect(tc->sock, (struct sockaddr *)&sa, sizeof(sa)) && errno != EINPROGRESS) {
        LDS_ERR_OUT(ERR_CLOSE_SOCK, "connect() failed, erro: %s\n", strerror(errno));
    }
	
    if (!(tc->connEvt = event_new(evtBase, tc->sock, EV_WRITE, ConnCb, tc))) {
        LDS_ERR_OUT(ERR_CLOSE_SOCK, "event_new() failed\n");
    }
    
    if (event_add(tc->connEvt, NULL)) {
		LDS_ERR_OUT(ERR_FREE_CONN_EVT, "connect() failed\n");
	}

    tc->st = DS_TCP_STRM_ST_CONNECTING;
    
	return 0;
    
ERR_DEL_CONN_EVT:
    event_del(tc->connEvt);
ERR_FREE_CONN_EVT:
    event_free(tc->connEvt);
    tc->connEvt = NULL;
ERR_CLOSE_SOCK:
    close(tc->sock);
ERR_OUT:
    return -1;
}

void DSTcpClientDestroy(DSTcpClient* strm)
{
    DSTcpClient *tc;
    
    LDS_DBG_IN_FUNC();
    tc = (DSTcpClient*)strm;
    switch (tc->st) {
        case DS_TCP_STRM_ST_CONNECTED:
             /* todo: wrtEvt maybe need delete*/
            ERR_DEL_WRT_EVT:
                event_del(tc->wrtEvt);
            ERR_DEL_RD_EVT:
                event_del(tc->rdEvt);
            ERR_FREE_WRT_EVT:
                event_free(tc->wrtEvt);
            ERR_FREE_RD_EVT:
                event_free(tc->rdEvt);
            break;
        case DS_TCP_STRM_ST_CONNECTING:
            ERR_DEL_CONN_EVT:
                event_del(tc->connEvt);
            ERR_FREE_CONN_EVT:
                event_free(tc->connEvt);
                tc->connEvt = NULL;
            break;
        case DS_TCP_STRM_ST_NONE:
            break;
    }
    
    ERR_CLOSE_SOCK:
        close(tc->sock);
    ERR_EXIT_STRM:
        DSStreamExit((DSStream*)tc);
    ERR_FREE_ETS:
        DSFree(tc);
}

DSTcpClient* DSTcpClientNew(const uint8_t* bind, const uint16_t port, const uint8_t* toIp, const uint16_t toPort)
{
    DSTcpClient* tc;
    
    LDS_DBG_IN_FUNC();
    if (!(tc = (DSTcpClient*)DSZalloc(sizeof(DSTcpClient)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (DSStreamInit((DSStream*)tc)) {
        LDS_ERR_OUT(ERR_FREE_ETS, "\n");
    }
    
    memcpy(tc->ip, toIp, sizeof(tc->ip));
    
    tc->port = toPort;
    
    tc->st = DS_TCP_STRM_ST_NONE;
    
    DSRewriteVFunc(DSStream, tc, Send, DSTcpClientSend);
    DSRewriteVFunc(DSStream, tc, Connect, DSTcpClientConnect);
    
    return tc;
    
ERR_EXIT_STRM:
    DSStreamExit((DSStream*)tc);
ERR_FREE_ETS:
    DSFree(tc);
ERR_OUT:
    return NULL;
}
