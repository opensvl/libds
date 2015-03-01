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
#include "ds_utils.h"

void DSStreamFinderInit(DSStreamFinder* sf)
{
	sf->stuf = NULL;
	sf->stufSz = 0;
	sf->n = 0;
	return;
}

int DSStreamFinderStartFind(DSStreamFinder* sf, const char* stuf, int stufSz)
{
	if (sf->stuf) {
		DSFree(sf->stuf);
		sf->stuf = NULL;
	}
	if (!(sf->stuf = (char*)DSMalloc(stufSz))) {
		return -1;
	}
	DSMemcpy(sf->stuf, stuf, stufSz);
	sf->stufSz = stufSz;
	sf->n = 0;
	return 0;
}

BOOL DSStreamFinderParseOne(DSStreamFinder* sf, char ch)
{
	if (sf->stuf[sf->n] == ch) {
		sf->n++;
	} else {
		int verified = sf->n;
		int i;
		int m;
		
		for (m=1; m<=verified-1; m++) {
			sf->n = 0;
			for (i=m; i<=verified; i++) {
				if (sf->stuf[i] == sf->stuf[sf->n]) {
					sf->n++;
				} else {
					break;
				}
			}
			if (sf->n+1 == verified) {
				if (sf->stuf[sf->n] == ch) {
					sf->n++;
				}
				break;
			}
		}
	}
	
	if (sf->n == sf->stufSz) {
        sf->n = 0;
		return TRUE;
	}
	return FALSE;
}

BOOL DSStreamFinderParse(DSStreamFinder* sf, char* buf, int size, int* ndlEndPos)
{
	int i;
	
	for (i=0; i<size; i++) {
		if (DSStreamFinderParseOne(sf, buf[i])) {
			*ndlEndPos = i;
			return TRUE;
		}
	}
	return FALSE;
}

void DSStreamFinderRestartFind(DSStreamFinder* sf)
{
	sf->n = 0;
}

int DSStreamFinderExit(DSStreamFinder* sf)
{
	if (sf->stuf) {
		DSFree(sf->stuf);
	}
	return 0;
}
