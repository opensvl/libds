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

void* DS_FUNC_ATTR DSRealloc(void *ptr, size_t size)
{
	void * ret;
	
	if (!(ret = (void*)DSMalloc(size))) {
		return NULL;
	}
	
	DSMemcpy(ret, ptr, size);
	
	DSFree(ptr);
	return ret;
}
