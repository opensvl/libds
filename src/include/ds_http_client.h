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
#ifndef _DS_HTTP_CLIENT_H_
#define _DS_HTTP_CLIENT_H_

#include "ds_utils.h"
#include "ds_object.h"

#define DS_MHB_HEADER_MAX_LEN   256

typedef struct _DSMhbClient DSMhbClient;

typedef enum {
    DS_MHB_CLIENT_CB_ERROR = -1,
    DS_MHB_CLIENT_CB_BODY_WRITABLE,
    DS_MHB_CLIENT_CB_RESP_HEADER,   /* data: DSConstBuf* */
    DS_MHB_CLIENT_CB_RESP_BODY,
    DS_MHB_CLIENT_CB_RESP_END
}DSMhbClientCbReason;

typedef void(*DSMhbClientCb)(DSMhbClient* cli, DSMhbClientCbReason reas, const void* data, void* userData);

struct _DSMhbClient {
    DSObject obj;
    DSStream* strm;
    DSMhbClientCb cb;
    void* userData;
    
    enum {
        DS_MHB_CLIENT_ST_INITED = 0,
        DS_MHB_CLIENT_ST_WRITE_REQ,
        DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS,
        DS_MHB_CLIENT_ST_WRITE_BODY,
        DS_MHB_CLIENT_ST_READ_RESP_HEADER,
        DS_MHB_CLIENT_ST_READ_RESP_BODY
    }st;
    char* buf;
    int bufSz;
    int filled;
    DSStreamFinder sf;
};

struct DSMhbRequest {
    const char* *headers;
    uint8_t headersCount;
    const char* body;   /* if null, CB_BODY_WRITEABLE will be called */
    int bodySz;
};

DSMhbClient* DSMhbClientNew(DSStream* strm, DSMhbClientCb cb, void* userData);

int DSMhbClientRequest(DSMhbClient* cli, struct DSMhbRequest*, DSMhbClientCb cb, void* userData);

void DSMhbClientBodyWriteFinished(DSMhbClient* cli);

void DSMhbClientStopRequest(DSMhbClient* cli);

void DSMhbClientDestroy(DSMhbClient* cli);


typedef void(*DSSimpleHttpClientCb)(const char* err, struct DSConstBuf* respBuf, void* userData);

int DSSimpleHttpClientRequest(DSStream* strm, struct DSMhbRequest* req, size_t maxRespSize, DSSimpleHttpClientCb cb, void* userData);

#endif
