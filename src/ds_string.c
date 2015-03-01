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

#include "ds_plat_basic.h"
#include "ds_string.h"

#ifdef LARGE_MEMORY
#define DS_STRING_BUFFER_NEW_MIN_SIZE 512
#else
#define DS_STRING_BUFFER_NEW_MIN_SIZE 16
#endif

int DSStringInitLen(DSString* this, const char* start, unsigned int len)
{
    this->aSize = len + 1 + DS_STRING_BUFFER_NEW_MIN_SIZE;    
    this->str = (char*)DSMalloc(this->aSize);
    strncpy(this->str, start, len);
    this->str[len] = '\0';
    this->len = len;
    return 0;
}

int DSStringInit(DSString* this, const char* str)
{
    return DSStringInitLen(this, str, strlen(str));
}

DSString* DSStringNewLen(const char* start, unsigned int len)
{
    DSString *this;
    this = (DSString*)DSMalloc(sizeof(DSString));
    DSStringInitLen(this, start, len);
    return this;
}

DSString* DSStringNew(const char* str)
{
    unsigned int len = strlen(str);
    return DSStringNewLen(str, len);
}

DSString* DSStringNewStartEnd(const char* start, const char* end)
{
    unsigned int len = end - start + 1;
    return DSStringNewLen(start, len);
}

int DSStringExit(DSString* this)
{
    DSFree(this->str);
    return 0;
}

void DSStringDestroy(DSString* this)
{
    DSStringExit(this);
    DSFree(this);
}

DSString* DSStringSafeCpN(DSString* this, const char* source, unsigned int sourceLen)
{
    if (this->aSize < sourceLen+1) {
        this->str = DSRealloc(this->str, sourceLen+1);
        this->aSize = sourceLen + 1;
    }
    strncpy(this->str, source, sourceLen);
    this->str[sourceLen] = '\0';
    this->len = strlen(this->str);
    return this;
}

DSString* DSStringSafeCp(DSString* this, const char* source)
{
    unsigned sourceLen = strlen(source);
    if(this->aSize < sourceLen+1){
        this->str = DSRealloc(this->str, sourceLen+1);
        this->aSize = sourceLen + 1;
    }
    strncpy(this->str, source, sourceLen);
    this->str[sourceLen] = '\0';
    this->len = sourceLen;
    return this;
}

DSString* DSStringSafeCatN(DSString* this, const char* source, unsigned int sourceLen)
{
    unsigned int neededSize = this->len + sourceLen+1;
    if(this->aSize < neededSize){
        this->str = DSRealloc(this->str, neededSize);
        this->aSize = neededSize;
    }
    strncat(this->str + this->len, source, sourceLen);
    this->len = neededSize - 1;
    return this;
}

DSString* DSStringSafeCat(DSString* this, const char* source)
{
    unsigned int sourceLen = strlen(source);
    return DSStringSafeCatN(this, source, sourceLen);
}

