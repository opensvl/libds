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
#include "ds_buffer.h"
#include "ds_stream.h"
#include "ds_http_client.h"
#include "lds_log.h"

static void DSMhbClientCleanupBuf(DSMhbClient* cli)
{
    if (cli->buf) {
        DSFree(cli->buf);
        cli->buf = NULL;
        cli->bufSz = 0;
        cli->filled = 0;
    }
}

static void StrmCb(DSStream* strm, DSStreamCbReason reas, void* data, void* userData)
{
    int pos, parsed, sent;
    const char *start;
    BOOL reqHasBody = FALSE;
    DSMhbClient *cli;
    struct DSConstBuf *rd;
    
    cli = userData;
    rd = data;
    
    switch (reas) {
        case DS_STREAM_CB_RECVED:
            switch (cli->st) {
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                    /* read response and parse with DSStreamFinder */
                    if (rd->size > 0) {
                        parsed = 0;
                        start = rd->buf;
                        while (DSStreamFinderParse(&cli->sf, start, rd->size-parsed, &pos)) {    /* find \r\n */
                            parsed += (pos+1);
                            
                            if ((pos == 1 && (cli->filled == 0)) || pos == 0 && (cli->filled == 1)) {   /* find \r\n\r\n  end */
                                start = NULL;
                                break;
                            } else {
                                struct DSConstBuf hdBuf;
                                int sz;
                                
                                if (pos > 1) {
                                    sz = pos - 1;
                                } else {
                                    sz = 0;
                                }
                                
                                if (cli->filled) {
                                    strncpy(cli->buf+cli->filled, start, sz);
                                    hdBuf.size = cli->filled+pos-1;
                                    hdBuf.buf = cli->buf;
                                    cli->cb(cli, DS_MHB_CLIENT_CB_RESP_HEADER, &hdBuf, cli->userData);
                                    cli->filled = 0;
                                } else {
                                    hdBuf.size = sz;
                                    hdBuf.buf = start;
                                    cli->cb(cli, DS_MHB_CLIENT_CB_RESP_HEADER, &hdBuf, cli->userData);
                                }
                            }
                            
                            start = &rd->buf[parsed];
                        }
                        
                        if (rd->size > parsed) {
                            memcpy(cli->buf, rd->buf+parsed, (cli->filled = rd->size-parsed));
                        }
                        if (start == NULL) {
                            DSStreamFinderExit(&cli->sf);
                            cli->st = DS_MHB_CLIENT_ST_READ_RESP_BODY;
                            cli->cb(cli, DS_MHB_CLIENT_CB_RESP_BODY, cli->buf, cli->userData);
                        }
                    } else if (rd->size ==0) {
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(header), (readed == 0)", cli->userData);
                    } else {    /* <0: error */
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(header) failed", cli->userData);
                    }
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    if (rd->size > 0) {
                        cli->cb(cli, DS_MHB_CLIENT_CB_RESP_BODY, rd->buf, cli->userData);
                    } else if (rd->size == 0) {
                        DSStreamClose(cli->strm);
                        cli->cb(cli, DS_MHB_CLIENT_CB_RESP_END, NULL, cli->userData);
                        cli->st = DS_MHB_CLIENT_ST_INITED;
                    } else {
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(body) failed", cli->userData);
                    }
                    break;
            }
            break;
        case DS_STREAM_CB_SENT:
            switch (cli->st) {
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                    reqHasBody = TRUE;
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                    sent = DSStreamSend(strm, cli->buf+cli->filled, cli->bufSz-cli->filled);
                    if (sent >= 0) {
                        cli->filled += sent;
                    } else {    /* <0: error */
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamWrite() failed", cli->userData);
                    }
                    if (cli->filled == cli->bufSz) {
                        DSFree(cli->buf);
                        cli->buf = NULL;
                        cli->bufSz = 0;
                        cli->filled = 0;
                    }
                    if (reqHasBody) {
                        DSMhbClientBodyWriteFinished(cli);
                    } else {
                        cli->st = DS_MHB_CLIENT_ST_WRITE_BODY;
                        cli->cb(cli, DS_MHB_CLIENT_CB_BODY_WRITABLE, NULL, cli->userData);
                    }
                    break;
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                    cli->cb(cli, DS_MHB_CLIENT_CB_BODY_WRITABLE, NULL, cli->userData);
                    break;
            }
            break;
        case DS_STREAM_CB_CONNECTED:
            switch(cli->st) {
                case DS_MHB_CLIENT_ST_INITED:
                    break;
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                    StrmCb(strm, DS_STREAM_CB_SENT, NULL, userData);    /* work around */
                    break;
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Unexpected connected\n", cli->userData);
                    break;
            }
            break;
        case DS_STREAM_CB_DISCONNECTED:
            switch(cli->st) {
                case DS_MHB_CLIENT_ST_INITED:
                    LDS_DBG("MHB connection closed gracefully\n");
                    break;
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Connection closed unexpectedly", cli->userData);
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Connection closed unexpectedly", cli->userData);
                    break;
            }
            break;
        case DS_STREAM_CB_ERROR:
            switch(cli->st) {
                case DS_MHB_CLIENT_ST_INITED:
                    LDS_DBG("MHB connection closed painfully\n");
                    break;
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Connection Error", cli->userData);
                    break;
            }
            break;
    }
}

DSMhbClient* DSMhbClientNew(DSStream* strm, DSMhbClientCb cb, void* userData)
{
    DSMhbClient* cli;
    
    if (!(cli = (DSMhbClient*)DSZalloc(sizeof(DSMhbClient)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (DSObjectInit((DSObject*)cli)) {
        LDS_ERR_OUT(ERR_FREE_CLI, "");
    }
    
    cli->strm = strm;
    DSStreamSetCb(cli->strm, StrmCb, cli);
    cli->userData = userData;
    cli->cb = cb;
    cli->st = DS_MHB_CLIENT_ST_INITED;
    return cli;

ERR_EXIT_OBJ:
    DSObjectExit((DSObject*)cli);
ERR_FREE_CLI:
    DSFree(cli);
ERR_OUT:
    return NULL;
}

int DSMhbClientRequest(DSMhbClient* cli, struct DSMhbRequest* req, DSMhbClientCb cb, void* userData)
{
    int i;
    int copied = 0;
    
    if (DSStreamConnect(cli->strm)) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    cli->bufSz = 2;    // last \r\n
    for (i=0; i<req->headersCount; i++) {
        cli->bufSz += (DSStrlen(req->headers[i])+2);
    }
    if (req->body) {
        cli->bufSz += req->bodySz;
    }
    
    if (!(cli->buf = (char*)DSMalloc(cli->bufSz))) {
        LDS_ERR_OUT(ERR_CLOSE_STRM, "\n");
    }
    cli->filled = 0;
    for (i=0; i<req->headersCount; i++) {
        int headerLen;
        
        headerLen = DSStrlen(req->headers[i]);
        DSMemcpy(cli->buf+copied, req->headers[i], headerLen);
        copied += headerLen;
        memcpy(cli->buf+copied, "\r\n", 2);
        copied += 2;
    }
    memcpy(cli->buf+copied, "\r\n", 2);
    copied += 2;
    if (!req->body) {
        cli->st = DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS;
    } else {
        memcpy(cli->buf+copied, req->body, req->bodySz);
        cli->st = DS_MHB_CLIENT_ST_WRITE_REQ;
    }
    
    if (cb) {
        cli->cb = cb;
    }
    
    if (userData) {
        cli->userData = userData;
    }
    
    return 0;
ERR_FREE_BUF:
    DSFree(cli->buf);
ERR_CLOSE_STRM:
    DSStreamClose(cli->strm);
ERR_OUT:
    return -1;
}

void DSMhbClientBodyWriteFinished(DSMhbClient* cli)
{
    DSStreamFinderInit(&cli->sf);
    if (!(cli->buf = (char*)DSMalloc(DS_MHB_HEADER_MAX_LEN))) {
        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Malloc recieve buffer failed", cli->userData);
        LDS_ERR_OUT(ERR_DESTOY_SF, "\n");
    }
    cli->bufSz = DS_MHB_HEADER_MAX_LEN;
    cli->filled = 0;
    cli->st = DS_MHB_CLIENT_ST_READ_RESP_HEADER;
    return;
ERR_DESTOY_BUF:
    DSFree(cli->buf);
ERR_DESTOY_SF:
    DSStreamFinderExit(&cli->sf);
ERR_OUT:
    return;
}

void DSMhbClientStopRequest(DSMhbClient* cli)
{
    switch (cli->st) {
        case DS_MHB_CLIENT_ST_INITED:
            break;
        case DS_MHB_CLIENT_ST_WRITE_REQ:
        case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
            DSMhbClientCleanupBuf(cli);
            DSStreamClose(cli->strm);
            break;
        case DS_MHB_CLIENT_ST_WRITE_BODY:
            DSMhbClientCleanupBuf(cli);
            DSStreamClose(cli->strm);
            break;
        case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
            DSStreamFinderExit(&cli->sf);
            DSMhbClientCleanupBuf(cli);
            DSStreamClose(cli->strm);
            break;
        case DS_MHB_CLIENT_ST_READ_RESP_BODY:
            DSMhbClientCleanupBuf(cli);
            DSStreamClose(cli->strm);
            break;
    }
    cli->st = DS_MHB_CLIENT_ST_INITED;
}

void DSMhbClientDestroy(DSMhbClient* cli)
{
    DSMhbClientStopRequest(cli);
ERR_EXIT_OBJ:
    DSObjectExit((DSObject*)cli);
ERR_FREE_CLI:
    DSFree(cli);
}

typedef struct _DSSimpleHttpClient {
    DSMhbClient* mCli;
    
    unsigned int maxRespSize;
    
    int8_t statusCode;
    DSBuffer *respBuffer;
    
    void* userData;
    DSSimpleHttpClientCb cb;
}DSSimpleHttpClient;

static void DSSimpleHttpClientDestroy(DSSimpleHttpClient* shc)
{
ERR_DESTOY_RESP_BUFFER:
    DSBufferDestroy(shc->respBuffer);
ERR_DESTOY_MC:
    DSMhbClientDestroy(shc->mCli);
ERR_FREE_SHC:
    DSFree(shc);
}

void _MhbClientCb(DSMhbClient* mc, DSMhbClientCbReason reas, const void* data, void* userData)
{
    DSSimpleHttpClient *shc;
    
    shc = userData;
    switch (reas) {
        case DS_MHB_CLIENT_CB_ERROR:
            shc->cb(data, NULL, shc->userData);
            DSSimpleHttpClientDestroy(shc);
            break;
        case DS_MHB_CLIENT_CB_BODY_WRITABLE:
            DSMhbClientBodyWriteFinished(mc);
            break;
        case DS_MHB_CLIENT_CB_RESP_HEADER:   /* data: DSConstBuf* */
        {
            struct DSConstBuf *hdBuf;
            
            if (shc->statusCode == -1) {
                if (!strncmp(hdBuf->buf, DS_CONST_STR_LEN("200"))) {
                    shc->statusCode = 200;
                } else {
                    shc->statusCode = 0;
                    shc->cb("status code not 200", NULL, shc->userData);
                    DSSimpleHttpClientDestroy(shc);
                }
            }
            break;
        }
        case DS_MHB_CLIENT_CB_RESP_BODY:
        {
            struct DSConstBuf *bBuf;
            
            if ((DSBufferGetSize(shc->respBuffer)+bBuf->size) > shc->maxRespSize) {
                shc->cb("Resp body is so large", NULL, shc->userData);
                DSSimpleHttpClientDestroy(shc);
            } else {
                DSBufferCat(shc->respBuffer, bBuf->buf, bBuf->size);
            }
            break;
        }
        case DS_MHB_CLIENT_CB_RESP_END:
        {
            struct DSConstBuf respBuf;
            
            respBuf.buf = DSBufferGetPtr(shc->respBuffer);
            respBuf.size = DSBufferGetSize(shc->respBuffer);
            shc->cb(NULL, &respBuf, shc->userData);
            DSSimpleHttpClientDestroy(shc);
            break;
        }
    }
}

int DSSimpleHttpClientRequest(DSStream* strm, struct DSMhbRequest* req, size_t maxRespSize, DSSimpleHttpClientCb cb, void* userData)
{
    DSSimpleHttpClient *shc;
    
    if (!strm) {
        LDS_ERR_OUT(ERR_OUT, "strm == NULL\n");
    }
    
    if (!(shc = (DSSimpleHttpClient*)DSZalloc(sizeof(DSSimpleHttpClient)))) {
        LDS_ERR_OUT(ERR_OUT, "DSMalloc() failed\n");
    }
    
    if (!(shc->mCli = DSMhbClientNew(strm, _MhbClientCb, shc))) {
        LDS_ERR_OUT(ERR_FREE_SHC, "DSMhbClientNew() failed");
    }
    
    if (!(shc->respBuffer = DSBufferNew(maxRespSize>>1, 16))) {
        LDS_ERR_OUT(ERR_DESTOY_MC, "DSBufferNew(32, 16) failed");
    }
    
    DSMhbClientRequest(shc->mCli, req, NULL, NULL);
    
    shc->cb = cb;
    shc->maxRespSize = maxRespSize;
    shc->userData = userData;
    return 0;
    
ERR_DESTOY_RESP_BUFFER:
    DSBufferDestroy(shc->respBuffer);
ERR_DESTOY_MC:
    DSMhbClientDestroy(shc->mCli);
ERR_FREE_SHC:
    DSFree(shc);
ERR_OUT:
    return -1;
}
