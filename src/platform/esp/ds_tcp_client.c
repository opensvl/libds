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

#include "ets_sys.h"
#include "os_type.h"
#include "ds_utils.h"
#include "ds_stream.h"
#include "ds_tcp_client.h"
#include "lds_log.h"
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <netif/etharp.h>
#include <lwip/tcp.h>
#include <lwip/ip.h>
#include <lwip/init.h>
#include <lwip/tcp_impl.h>
#include <lwip/memp.h>
#include <lwip/mem.h>
struct _DSTcpClient {
    DSStream _strm;
    struct tcp_pcb *pcb;
    uint8_t host[4];
    uint16_t port;
};

static void _DSTcpClientDestroy(DSTcpClient* tc);

err_t sent(void * arg, struct tcp_pcb * tpcb, u16_t len)
{
    DSTcpClient *tc;
    
    tc = arg;
    
    DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_SENT, NULL);
    return ERR_OK;
}

static int DSTcpClientSend(DSStream* strm, uint8_t* buf, int bufSz)
{
    DSTcpClient *tc;
    u16_t canSend;
    
    tc = (DSTcpClient*)strm;
    
    canSend = tcp_sndbuf(tc->pcb);
    if (canSend <= 0) {
        return 0;
    }
    
    if (canSend > bufSz) {
        canSend = bufSz;
    }
    
    if (tcp_write(tc->pcb, buf, canSend, TCP_WRITE_FLAG_COPY)) {
        DSStreamCallCb(strm, DS_STREAM_CB_ERROR, "tcp_write() failed");
        return ERR_OK;
    }
    
    tcp_sent(tc->pcb, sent);
    
    return canSend;
}

static err_t recv(void * arg, struct tcp_pcb * tpcb,
                              struct pbuf * p, err_t err)
{
    DSTcpClient *tc;
    
    tc = arg;
    
    if (err != ERR_OK) {
        DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_ERROR, "recv() failed");
        return ERR_OK;
    }
    
    if (p) {    /* got data */
        struct DSConstBuf cBuf;
        
        tcp_recved(tc->pcb, p->tot_len);
        
        cBuf.buf = p->payload;
        cBuf.size = p->tot_len;
        DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_RECVED, &cBuf);
        pbuf_free(p);
    } else {    /* remote closed connection */
        DSStreamCallCb((DSStream*)tc, DS_STREAM_CB_DISCONNECTED, NULL);
    }
    
    return ERR_OK;
}

static err_t connected(void * arg, struct tcp_pcb * tpcb, err_t err)
{
    DSTcpClient *tc;
    
    tc = arg;
    
    tcp_recv(tc->pcb, recv);
    
    return ERR_OK;
}

static int DSTcpClientConnect(DSStream* strm)
{
    DSTcpClient *tc;
    struct ip_addr hostIp;
    
    tc = (DSTcpClient*)strm;
    
    IP4_ADDR(&hostIp, tc->host[0], tc->host[1], tc->host[2], tc->host[3]);
    
    if (tcp_connect(tc->pcb, &hostIp, tc->port, connected)) {
        DSStreamCallCb(strm, DS_STREAM_CB_ERROR, "tcp_connect() failed");
    }
}

DSTcpClient* DSTcpClientNew(const uint8_t* bind, const uint16_t port, const uint8_t* toIp, const uint16_t toPort) 
{
    DSTcpClient* tc;
    struct ip_addr bindIp;
    
    if (!(tc = (DSTcpClient*)DSZalloc(sizeof(DSTcpClient)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (DSStreamInit((DSStream*)tc)) {
        LDS_ERR_OUT(ERR_FREE_TC, "\n");
    }
    
    if (!(tc->pcb = tcp_new())) {
        LDS_ERR_OUT(ERR_EXIT_STRM, "tcp_new() failed\n");
    }
    
    tcp_arg(tc->pcb, tc);
    
    IP4_ADDR(&bindIp, bind[0], bind[1], bind[2], bind[3]);
    
    if (tcp_bind(tc->pcb, &bindIp, port)) {
        LDS_ERR_OUT(ERR_CLOSE_PCB, "tcp_bind() failed\n");
    }
    
    memcpy(tc->host, toIp, sizeof(tc->host));
    tc->port = toPort;
    
    DSRewriteVFunc(DSStream, tc, Send, DSTcpClientSend);
    DSRewriteVFunc(DSStream, tc, Connect, DSTcpClientConnect);
    
    return tc;
    
ERR_CLOSE_PCB:
    tcp_close(tc->pcb);
ERR_EXIT_STRM:
    DSStreamExit((DSStream*)tc);
ERR_FREE_TC:
    DSFree(tc);
ERR_OUT:
    return NULL;
}

void DSTcpClientDestroy(DSTcpClient* tc)
{
    tcp_recv(tc->pcb, NULL);
ERR_CLOSE_PCB:
    tcp_close(tc->pcb);
ERR_EXIT_STRM:
    DSStreamExit((DSStream*)tc);
ERR_FREE_TC:
    DSFree(tc);
}
