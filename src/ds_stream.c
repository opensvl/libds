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
#include "ds_stream.h"

void DSStreamInit(DSStream* strm)
{
    DSObjectInit(strm);
}

void DSStreamExit(DSStream* strm)
{
    DSObjectExit(strm);
}

int DSStreamSend(DSStream* strm, uint8_t* buf, int bufSz)
{
    return strm->Write(strm, buf, bufSz);
}

int DSStreamConnect(DSStream* strm)
{
    return strm->Connect(strm);
}

int DSStreamClose(DSStream* strm)
{
    return strm->Close(strm);
}

void DSStreamSetCb(DSStream* strm, DSStreamCb cb, void* userData)
{
    strm->cb = cb;
    strm->userData = userData;
}

/* for child classes(drivers) */
void DSStreamCallCb(DSStream* strm, DSStreamCbReason reas, void* data)
{
    strm->cb(strm, reas, data, strm->userData);
}
