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
#include "ds_plat_basic.h"
#include "ds_stream.h"

typedef struct _DSTcpStream DSTcpStream;

struct _DSTcpStream {
    DSStream _strm;
    struct espconn *ec;
    BOOL remoteDisconn;
    BOOL didDisconn;
};

DSTcpStream* DSTcpStreamNew(const uint8_t host[4], const uint16_t port);

void DSTcpStreamDestroy(DSTcpStream* ets);
#endif
