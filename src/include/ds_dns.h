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

typedef struct _DSGetAddrInfo DSGetAddrInfo;

struct DSAddrInfo {
    uint8_t* ip;
};

typedef void (*DSGetAddrInfoCb)(DSGetAddrInfo* gai, void* err, struct DSAddrInfo *res, void *userData);



struct _DSGetAddrInfo{
};


DSGetAddrInfo* DSGetAddrInfoNew(
    const char *nodename, const char *servname,
    const struct DSAddrInfo* hintsIn,
    DSGetAddrInfoCb cb, void* userData, void* evtBase);

void DSGetAddrInfoDestroy(DSGetAddrInfo* gai);


