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
#ifndef _DS_OBJECT_H_
#define _DS_OBJECT_H_
#include "ds_plat_basic.h"

typedef struct _DSObject DSObject;
struct _DSObject {
};

int DSObjectInit(DSObject* obj);

int DSObjectExit(DSObject* obj);

typedef struct _DSEventObject DSEventObject;

struct _DSEventObject {
    DSObject _obj;
    void* userData;
    void* cb;
};

int DSEventObjectInit(DSEventObject* obj);

int DSEventObjectExit(DSEventObject* obj);

void DSEventObjectSetCb(DSEventObject* obj, void* cb, void* userData);

void* DSEventObjectGetCb(DSEventObject* obj);

void* DSEventObjectGetUserData(DSEventObject* obj);

#endif
