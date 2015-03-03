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
#ifndef _DS_STREAM_H_
#define _DS_STREAM_H_

#include "ds_plat_basic.h"
#include "ds_utils.h"
#include "ds_object.h"

typedef struct _DSStream DSStream;

typedef enum {
    DS_STREAM_CB_ERROR = -1,
    DS_STREAM_CB_CONNECTED = 0,
    DS_STREAM_CB_RECVED,   /* data: struct DSConstBuf* */
    DS_STREAM_CB_SENT,
    DS_STREAM_CB_DISCONNECTED,
}DSStreamCbReason;


typedef void(*DSStreamCb)(DSStream* strm, DSStreamCbReason reas, void* data, void* userData);

struct _DSStream {
    DSObject obj;
    int(*Send)(DSStream* strm, uint8_t* buf, int bufSz);
    int(*Connect)(DSStream* strm);
    int(*Close)(DSStream* strm);
    DSStreamCb cb;
    void *userData;
};

int DSStreamConnect(DSStream* strm);
int DSStreamClose(DSStream* strm);

int DSStreamSend(DSStream* strm, uint8_t* buf, int bufSz);

void DSStreamSetCb(DSStream* strm, DSStreamCb cb, void* userData);

/* for child classes(drivers) */
int DSStreamInit(DSStream* strm);

int DSStreamExit(DSStream* strm);

void DSStreamCallCb(DSStream* strm, DSStreamCbReason reas, void* data);

#endif
