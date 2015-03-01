/*
 * libds
 * Copyright (c) Chunfeng Zhang <CrazyPandar@gmail.com>, All rights reserved.
 * 
 * This library is DSFree software; you can redistribute it and/or
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
#ifndef _DS_STRING_H_
#define _DS_STRING_H_

typedef struct {
	char *str;
	unsigned int len;
	unsigned int aSize;
}DSString;

int DSStringInitLen(DSString* self, const char* start, unsigned int len);

int DSStringInit(DSString* self, const char* str);

DSString* DSStringNew(const char* str);

DSString* DSStringNewLen(const char* start, unsigned int len);

DSString* DSStringNewStartEnd(const char* start, const char* end);

int DSStringExit(DSString* string);

void DSStringDestroy(DSString* string);

DSString* DSStringSafeCpN(DSString* string, const char* source, unsigned int sourceLen);

DSString* DSStringSafeCp(DSString* string, const char* source);

DSString* DSStringSafeCatN(DSString* string, const char* source, unsigned int sourceLen);

DSString* DSStringSafeCat(DSString* string, const char* source);

#define DSStringGetStr(_string) ((_string)->str)

#define DSStringGetLen(_string) ((_string)->len)

#define DSStringGetSize(_string) ((_string)->aSize)

#define DS_CONST_STR_LEN(__str) (__str),sizeof(__str)-1

#endif
