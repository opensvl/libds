/*
 * libds
 * Copyright (c) Chunfeng Zhang(CrazyPandar@gmail.com), All rights reserved.
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
#ifndef _DS_PLAT_H_
#define _DS_PLAT_H_
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>	/* for errno, strerror */

#define BOOL int
#define TRUE 1
#define FALSE 0

#define DSMalloc malloc
#define DSFree free
void* DSZalloc(size_t size);
#define DSRealloc realloc
#define DSMemcpy memcpy
#define DSMemcmp memcmp

#define DSStrlen    strlen
#define DSStrncpy   strncpy
#define DSStrcpy   strcpy

#endif
