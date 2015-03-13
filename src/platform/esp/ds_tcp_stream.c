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
#include "ds_plat_basic.h"
#include "ds_utils.h"
#include "user_interface.h"
#include "espconn.h"
#include "ds_stream.h"
#include "ds_tcp_stream.h"
#include "lds_log.h"

#define ESP_CONN_SENT_MAX_SIZE  768

static void _DSTcpStreamDestroy(DSTcpStream* ets);

static int DSTcpStreamSend(DSStream* strm, uint8_t* buf, int bufSz)
{
    #define ets ((DSTcpStream*)strm)
    
    if (bufSz > ESP_CONN_SENT_MAX_SIZE) {
        bufSz = ESP_CONN_SENT_MAX_SIZE;
    }

    if (espconn_sent(ets->ec, buf, bufSz)) {
        return 0;
    } else {
        return bufSz;
    }
    
    #undef ets
}

static int DSTcpStreamConnect(DSStream* strm)
{
    #define ets ((DSTcpStream*)strm)
    
    ets->remoteDisconn = FALSE;
    BOOL didDisconn = FALSE;
    return espconn_connect(ets->ec);
    
    #undef ets
}

void DSTcpStreamDestroy(DSTcpStream* strm)
{
    #define ets ((DSTcpStream*)strm)
    
    if (!ets->remoteDisconn) {  /* we do disconn */
        ets->didDisconn = TRUE;
        espconn_disconnect(ets->ec);
    } else {    /* remote did disconn */
        _DSTcpStreamDestroy(ets);
    }

    #undef ets
}

static void EspConnRecvCb(void *arg, char *pdata, unsigned short len)
{
    #define ec ((struct espconn*)arg)
    #define strm    ((DSStream*)ets)
    DSTcpStream* ets;

    ets = (DSTcpStream*)ec->reverse;
    struct DSConstBuf rd = {
        .buf = pdata,
        .size = len
    };
    
    strm->cb(strm, DS_STREAM_CB_RECVED, &rd, strm->userData);
    
    #undef strm
    #undef ec
}

static void ICACHE_FLASH_ATTR EspConnSentCb(void *arg)
{	
	#define ec ((struct espconn*)arg)
    #define strm    ((DSStream*)ets)
    DSTcpStream* ets;

    ets = (DSTcpStream*)ec->reverse;
    
    strm->cb(strm, DS_STREAM_CB_SENT, NULL, strm->userData);
    
    #undef strm
    #undef ec
}

static void ICACHE_FLASH_ATTR EspConnConnectedCb(void *arg)
{	
	#define ec ((struct espconn*)arg)
    #define strm    ((DSStream*)ets)
    DSTcpStream* ets;

    ets = (DSTcpStream*)ec->reverse;
    
    strm->cb(strm, DS_STREAM_CB_CONNECTED, NULL, strm->userData);
    
    #undef strm
    #undef ec
}

static void ICACHE_FLASH_ATTR EspConnDisconnectedCb(void *arg)
{
    #define ec ((struct espconn*)arg)
    #define strm    ((DSStream*)ets)
    DSTcpStream* ets;

    ets = (DSTcpStream*)ec->reverse;
    
    if (!ets->didDisconn) { /* remote did disconn */
        strm->cb(strm, DS_STREAM_CB_DISCONNECTED, NULL, strm->userData);
        ets->remoteDisconn = TRUE;
    } else {    /* we disconn */
        _DSTcpStreamDestroy(ets);
    }
    
    #undef strm
    #undef ec
}

 void ICACHE_FLASH_ATTR EspConnReconnectCb(void *arg, sint8 err)
{	
	#define ec ((struct espconn*)arg)
    DSTcpStream* ets;
    
    #undef ec
}

DSTcpStream* DSTcpStreamNew(const uint8_t host[4], const uint16_t port) 
{
    DSTcpStream* ets;
    
    if (!(ets = (DSTcpStream*)DSZalloc(sizeof(DSTcpStream)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (DSStreamInit((DSStream*)ets)) {
        LDS_ERR_OUT(ERR_FREE_ETS, "\n");
    }
    
    if (!(ets->ec = (struct espconn *)DSZalloc(sizeof(struct espconn)))) {
        LDS_ERR_OUT(ERR_EXIT_STRM, "\n");
    }
	ets->ec->type = ESPCONN_TCP;
	ets->ec->state = ESPCONN_CONNECT;
	if (!(ets->ec->proto.tcp = (esp_tcp *)DSZalloc(sizeof(esp_tcp)))) {
        LDS_ERR_OUT(ERR_FREE_EC, "\n");
    }
	ets->ec->proto.tcp->local_port = espconn_port();
	ets->ec->proto.tcp->remote_port = port;
	memcpy(ets->ec->proto.tcp->remote_ip, host, 4);
	
    espconn_regist_connectcb(ets->ec, EspConnConnectedCb);
	espconn_regist_sentcb(ets->ec, EspConnSentCb);
	espconn_regist_reconcb(ets->ec, EspConnReconnectCb);
    espconn_regist_disconcb(ets->ec, EspConnDisconnectedCb);
    espconn_regist_recvcb(ets->ec, EspConnRecvCb);
	ets->ec->reverse = ets;
    
    DSRewriteVFunc(DSStream, ets, Send, DSTcpStreamSend);
    DSRewriteVFunc(DSStream, ets, Connect, DSTcpStreamConnect);
    
    return ets;
    
ERR_FREE_TCP:
    DSFree(ets->ec->proto.tcp);
ERR_FREE_EC:
    DSFree(ets->ec);
ERR_EXIT_STRM:
    DSStreamExit((DSStream*)ets);
ERR_FREE_ETS:
    DSFree(ets);
ERR_OUT:
    return NULL;
}

static void _DSTcpStreamDestroy(DSTcpStream* ets)
{
    
ERR_FREE_TCP:
    DSFree(ets->ec->proto.tcp);
ERR_FREE_EC:
    DSFree(ets->ec);
ERR_EXIT_STRM:
    DSStreamExit((DSStream*)ets);
ERR_FREE_ETS:
    DSFree(ets);
}
