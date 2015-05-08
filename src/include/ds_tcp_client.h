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
#ifndef _DS_TCP_CLIENT_
#define _DS_TCP_CLIENT_
#include "ds_stream.h"
//#include "ds_tcp_client_.h"

typedef struct _DSTcpClient DSTcpClient;

DSTcpClient* DSTcpClientNew(const uint8_t* bind, const uint16_t port, const uint8_t* toIp, const uint16_t toPort);

void DSTcpClientDestroy(DSTcpClient* ets);
#endif
