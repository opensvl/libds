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
#include "ds_plat_basic.h"
#include "ds_utils.h"
#include "ds_stream.h"
#include "ds_tcp_stream.h"
#include "lds_log.h"

static void _DSTcpStreamDestroy(DSTcpStream* tcpStrm);

static int DSTcpStreamSend(DSStream* strm, uint8_t* buf, int bufSz)
{
    DSTcpStream *tcpStrm;
    int writed;
    
    LDS_DBG_IN_FUNC();
    tcpStrm = (DSTcpStream*)strm;
    
    writed = write(tcpStrm->sock, buf, bufSz);
    
    if (writed > 0) {
        if (event_add(tcpStrm->wrtEvt, NULL)) {
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
    DSTcpStream *tcpStrm;
    int readed;
    
    LDS_DBG_IN_FUNC();
    tcpStrm = arg;
    
    readed = read(fd, tcpStrm->buf, sizeof(tcpStrm->buf));
    
    if (readed > 0) {
        struct DSConstBuf rd = {
            .buf = tcpStrm->buf,
            .size = readed
        };
        
        DSStreamCallCb((DSStream*)tcpStrm, DS_STREAM_CB_RECVED, &rd);
    } else if (readed == 0) {
        DSStreamCallCb((DSStream*)tcpStrm, DS_STREAM_CB_DISCONNECTED, NULL);
    } else {
        DSStreamCallCb((DSStream*)tcpStrm, DS_STREAM_CB_ERROR, NULL);
    }
}

static void WriteCb(int fd, short ev, void *arg)
{
    DSTcpStream *tcpStrm;
    
    LDS_DBG_IN_FUNC();
    tcpStrm = arg;
    
    if (event_del(tcpStrm->wrtEvt)) {
        LDS_ERR("devent_del failed\n");
    }
    DSStreamCallCb((DSStream*)tcpStrm, DS_STREAM_CB_SENT, NULL);
}


static void ConnCb(int fd, short ev, void *arg)
{
    DSTcpStream *tcpStrm;

    LDS_DBG_IN_FUNC();
    tcpStrm = arg;

    event_del(tcpStrm->connEvt);
    event_free(tcpStrm->connEvt);
    
    if (!(tcpStrm->rdEvt = event_new(tcpStrm->evtBase, tcpStrm->sock, EV_READ | EV_PERSIST, ReadCb, tcpStrm))) {
        LDS_ERR_OUT(ERR_OUT, "event_new(EV_READ) failed\n");
    }

    if (!(tcpStrm->wrtEvt = event_new(tcpStrm->evtBase, tcpStrm->sock, EV_WRITE | EV_PERSIST, WriteCb, tcpStrm))) {
        LDS_ERR_OUT(ERR_FREE_RD_EVT, "event_new(EV_WRITE) failed\n");
    }
    
    if (event_add(tcpStrm->rdEvt, NULL)) {
         DSStreamCallCb((DSStream*)tcpStrm, DS_STREAM_CB_ERROR, "event_add(&tcpStrm->rdEvt, NULL) failed");
         return;
    }
    
    tcpStrm->st = DS_TCP_STRM_ST_CONNECTED;
    DSStreamCallCb((DSStream*)tcpStrm, DS_STREAM_CB_CONNECTED, NULL);
    
    return;
ERR_DEL_RD_EVT:
    event_del(tcpStrm->rdEvt);
ERR_FREE_WRT_EVT:
    event_free(tcpStrm->wrtEvt);
ERR_FREE_RD_EVT:
    event_free(tcpStrm->rdEvt);
ERR_OUT:
    return;
}

#define DS_MAKE_S_ADDR_FROM_ARRAY(__ip)    (__ip[0]|__ip[1]<<8|__ip[2]<<16|__ip[3]<<24)

static int DSTcpStreamConnect(DSStream* strm)
{
    struct sockaddr_in sa;
    DSTcpStream *tcpStrm;
    
    LDS_DBG_IN_FUNC();
    tcpStrm = (DSTcpStream*)strm;

	if ((tcpStrm->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LDS_ERR_OUT(ERR_OUT, "socket() failed\n");
	}

	if (setnonblock(tcpStrm->sock)) {
		LDS_ERR_OUT(ERR_CLOSE_SOCK, "setnonblock() failed\n");
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(tcpStrm->port);
    
	sa.sin_addr.s_addr = DS_MAKE_S_ADDR_FROM_ARRAY(tcpStrm->ip);    /* inet_addr("127.0.0.1");*/

    if (connect(tcpStrm->sock, (struct sockaddr *)&sa, sizeof(sa)) && errno != EINPROGRESS) {
        LDS_ERR_OUT(ERR_CLOSE_SOCK, "connect() failed, erro: %s\n", strerror(errno));
    }
	
    if (!(tcpStrm->connEvt = event_new(tcpStrm->evtBase, tcpStrm->sock, EV_WRITE, ConnCb, tcpStrm))) {
        LDS_ERR_OUT(ERR_CLOSE_SOCK, "event_new() failed\n");
    }
    
    if (event_add(tcpStrm->connEvt, NULL)) {
		LDS_ERR_OUT(ERR_FREE_CONN_EVT, "connect() failed\n");
	}

    tcpStrm->st = DS_TCP_STRM_ST_CONNECTING;
    
	return 0;
    
ERR_DEL_CONN_EVT:
    event_del(tcpStrm->connEvt);
ERR_FREE_CONN_EVT:
    event_free(tcpStrm->connEvt);
    tcpStrm->connEvt = NULL;
ERR_CLOSE_SOCK:
    close(tcpStrm->sock);
ERR_OUT:
    return -1;
}

void DSTcpStreamDestroy(DSTcpStream* strm)
{
    DSTcpStream *tcpStrm;
    
    LDS_DBG_IN_FUNC();
    tcpStrm = (DSTcpStream*)strm;
    switch (tcpStrm->st) {
        case DS_TCP_STRM_ST_CONNECTED:
             /* todo: wrtEvt maybe need delete*/
            ERR_DEL_WRT_EVT:
                event_del(tcpStrm->wrtEvt);
            ERR_DEL_RD_EVT:
                event_del(tcpStrm->rdEvt);
            ERR_FREE_WRT_EVT:
                event_free(tcpStrm->wrtEvt);
            ERR_FREE_RD_EVT:
                event_free(tcpStrm->rdEvt);
            break;
        case DS_TCP_STRM_ST_CONNECTING:
            ERR_DEL_CONN_EVT:
                event_del(tcpStrm->connEvt);
            ERR_FREE_CONN_EVT:
                event_free(tcpStrm->connEvt);
                tcpStrm->connEvt = NULL;
            break;
        case DS_TCP_STRM_ST_NONE:
            break;
    }
    
    ERR_CLOSE_SOCK:
        close(tcpStrm->sock);
    ERR_EXIT_STRM:
        DSStreamExit((DSStream*)tcpStrm);
    ERR_FREE_ETS:
        DSFree(tcpStrm);
}

DSTcpStream* DSTcpStreamNew(const uint8_t host[4], const uint16_t port)
{
    DSTcpStream* tcpStrm;
    
    LDS_DBG_IN_FUNC();
    if (!(tcpStrm = (DSTcpStream*)DSZalloc(sizeof(DSTcpStream)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (DSStreamInit((DSStream*)tcpStrm)) {
        LDS_ERR_OUT(ERR_FREE_ETS, "\n");
    }
    
    memcpy(tcpStrm->ip, host, sizeof(tcpStrm->ip));
    
    tcpStrm->port = port;
    
    tcpStrm->st = DS_TCP_STRM_ST_NONE;
    
    DSRewriteVFunc(DSStream, tcpStrm, Send, DSTcpStreamSend);
    DSRewriteVFunc(DSStream, tcpStrm, Connect, DSTcpStreamConnect);
    
    return tcpStrm;
    
ERR_EXIT_STRM:
    DSStreamExit((DSStream*)tcpStrm);
ERR_FREE_ETS:
    DSFree(tcpStrm);
ERR_OUT:
    return NULL;
}

void DSTcpStreamSetEventBase(DSTcpStream* tcpStrm, void* base)
{
    tcpStrm->evtBase = base;
}
