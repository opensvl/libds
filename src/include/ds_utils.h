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
#ifndef _DS_UTILS_H_
#define _DS_UTILS_H_

#ifndef offsetof
#ifdef __compiler_offsetof
#define DSOffsetOf(TYPE, MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define DSOffsetOf(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
#endif
#endif

#define DS_CONTAINER_OF(__pointer, __type, __member) ((__type*)((char*)__pointer - offsetof(__type, __member)))

#define DS_MIN(__a, __b) (((__a)<(__b))?(__a):(__b))
#define DS_MAX(__a, __b) (((__a)>(__b))?(__a):(__b))
#define DS_ALIGN(__x, __a)	(((__x + __a - 1) / __a) * __a)

#define DSRewriteVFunc(pClass, obj, vFunc, func) ((pClass*)obj)->vFunc = func

/* stream utility */
typedef struct DSStreamFinder {
	char* stuf;
	int stufSz;
	int n;
} DSStreamFinder;

void DSStreamFinderInit(DSStreamFinder* sf);

int DSStreamFinderStartFind(DSStreamFinder* sf, const char* stuf, int stufSz);

BOOL DSStreamFinderParseOne(DSStreamFinder* sf, char ch);

BOOL DSStreamFinderParse(DSStreamFinder* sf, char* buf, int size, int* ndlEndPos);

void DSStreamFinderRestartFind(DSStreamFinder* sf);

int DSStreamFinderExit(DSStreamFinder* sf);

#endif
