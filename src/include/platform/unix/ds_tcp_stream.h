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
#ifndef _DS_TCP_STREAM_
#define _DS_TCP_STREAM_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include "ds_event.h"
#include "ds_plat_basic.h"
#include "ds_stream.h"


typedef struct _DSTcpStream DSTcpStream;

struct _DSTcpStream {
    DSStream _strm;
    uint8_t ip[4];
    uint16_t port;
    
    int sock;
    struct event *connEvt;
    struct event *rdEvt;
    struct event *wrtEvt;
    struct event_base *evtBase;
    
    enum {
        DS_TCP_STRM_ST_NONE,
        DS_TCP_STRM_ST_CONNECTING,
        DS_TCP_STRM_ST_CONNECTED,
    }st;
    
    uint8_t buf[1024*4];
};

DSTcpStream* DSTcpStreamNew(const uint8_t host[4], const uint16_t port);

void DSTcpStreamSetEventBase(DSTcpStream* tcpStrm, void* base);

void DSTcpStreamDestroy(DSTcpStream* ets);

#endif
