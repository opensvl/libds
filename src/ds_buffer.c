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

#include "lds_log.h"
#include "ds_utils.h"
#include "ds_buffer.h"

int DSBufferInit(DSBuffer* self, size_t minSize, size_t unit)
{
    self->data = (uint8_t*)DSMalloc(minSize);
    if(!self->data){
        LDS_ERR("DSMalloc failed on new data for new buffer\n");
        return -1;
    }
    self->minSize = minSize;
    self->unit = unit;
    self->len = 0;
    self->curSz = minSize;
    return 0;
}

void DSBufferExit(DSBuffer* self)
{
    DSFree(self->data);
    self->unit = 0;
    self->len = 0;
    self->curSz = 0;
}

DSBuffer* DSBufferNew(size_t minSize, size_t unit)
{
    DSBuffer* ret;
    ret = (DSBuffer*)DSMalloc(sizeof(DSBuffer));
    if(!ret){
        LDS_ERR("DSMalloc failed on new buffer\n");
        return NULL;
    }
    if(DSBufferInit(ret, minSize, unit)){
        DSFree(ret);
        return NULL;
    }
    return ret;
}

void* DSBufferGetPtr(DSBuffer* buf)
{
    return buf->data;
}

size_t DSBufferGetSize(DSBuffer* buf)
{
    return buf->len;
}

void DSBufferDestroy(DSBuffer* buf)
{
    DSBufferExit(buf);
    DSFree(buf);
}

int DSBufferCp(DSBuffer* to, const void* source, size_t len)
{
	size_t needed;
	
	if (len <= to->minSize) {
		needed = to->minSize;
	} else {
		needed = to->minSize + DS_ALIGN(len-(to->minSize), to->unit);
	}
	
	if (to->curSz != needed) {
		to->data = DSRealloc(to->data, needed);
		if(!to->data){
			LDS_ERR("DSRealloc() failed for buffer_cp\n");
			return -1;
		}
		to->curSz = needed;
	}
	
    if(source)
            DSMemcpy(to->data, source, len);
    to->len = len;
    return 0;
}

int DSBufferCat(DSBuffer* to, const void* source, size_t len)
{
	size_t needed;
    size_t newLen = len + to->len;
	
	if (newLen <= to->minSize) {
		needed = to->minSize;
	} else {
		needed = to->minSize + DS_ALIGN(newLen-(to->minSize), to->unit);
	}
	
	if (to->curSz != needed) {
		to->data = DSRealloc(to->data, needed);
		if(!to->data){
			LDS_ERR("DSRealloc() failed for buffer_cat\n");
			return -1;
		}
		to->curSz = needed;
	}
	
    if(source) {
        DSMemcpy(to->data + to->len, source, len);
    }
    to->len = newLen;
    return 0;
}
