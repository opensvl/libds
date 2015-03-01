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
 
#ifndef _DS_BUFFER_H_
#define _DS_BUFFER_H_

#include "ds_plat_basic.h"

typedef struct ds_buffer{
    size_t minSize;
    size_t unit;
    size_t curSz;
    size_t len;
    uint8_t* data;
}DSBuffer;

int DSBufferInit(DSBuffer* self, size_t minSize, size_t unit);

void DSBufferExit(DSBuffer* self);

DSBuffer* DSBufferNew(size_t minSize, size_t unit);

void DSBufferDestroy(DSBuffer* buf);

int DSBufferCp(DSBuffer* to, const void* source, size_t len);

int DSBufferCat(DSBuffer* to, const void* source, size_t len);

void* DSBufferGetPtr(DSBuffer* buf);

size_t DSBufferGetSize(DSBuffer* buf);

#endif

