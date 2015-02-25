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
#ifndef _DS_PLAT_BASIC_H_
#define _DS_PLAT_BASIC_H_

#include <ets_sys.h>
#include <os_type.h>
#include <mem.h>
#include <osapi.h>

#define fprintf(__FP, ...)	printf(__VA_ARGS__);

#define DSMalloc os_malloc
#define DSFree os_free
#define DSZalloc os_zalloc
void* DSRealloc(void *ptr, size_t size);
#define DSMemcpy os_memcpy
#define DSMemcmp os_memcmp

#define DS_FUNC_ATTR

#endif
