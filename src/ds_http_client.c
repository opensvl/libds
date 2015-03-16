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
    const uint8_t *start, *end;
    size_t seLen;
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
                        seLen = 0;
                        start = rd->buf;
                        end = NULL;
                        while ((end = memchr(start, '\n', rd->size-(start-rd->buf)))) {    /* find \r\n */
                            seLen = end - start + 1;
                            
                            LDS_DBG("seLen=%d, cli->filled=%d\n", seLen, cli->filled);
                            LDS_DBG("start: %c, end: %c\n", *start, *end);
                            if ((seLen == 1 && (cli->filled == 1)) || (seLen == 2 && (cli->filled == 0))) {   /* find empty \r\n header */
                                LDS_DBG("find empty \\r\\n header\n");
                                break;
                            } else {
                                struct DSConstBuf hdBuf;
                                
                                if (cli->filled) {
                                    memcpy(cli->buf+cli->filled, start, seLen);
                                    hdBuf.size = (cli->filled + seLen);
                                    hdBuf.buf = cli->buf;
                                    if (!(cli->cb(cli, DS_MHB_CLIENT_CB_RESP_HEADER, &hdBuf, cli->userData))) {
                                        return;
                                    } 
                                    cli->filled = 0;
                                } else {
                                    hdBuf.size = seLen;
                                    hdBuf.buf = start;
                                    if (!(cli->cb(cli, DS_MHB_CLIENT_CB_RESP_HEADER, &hdBuf, cli->userData))) {
                                        return;
                                    }
                                }
                            }
                            
                            start = end+1;
                        }
                        
                        if (end == NULL) {  /* find part header */
                            size_t partSz;
                            partSz = rd->size-(start-rd->buf);
                            
                            if ((partSz+cli->filled) >= cli->bufSz) {
                                LDS_ERR("header too large");
                                cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "header too large", cli->userData);
                            } else {
                                memcpy(cli->buf+cli->filled, start, partSz);
                                cli->filled += partSz;
                            }
                        } else {    /* find \r\n\r\n */
                            struct DSConstBuf bdBuf;
                            
                            LDS_DBG("find \\r\\n\\r\\n\n");
                            cli->st = DS_MHB_CLIENT_ST_READ_RESP_BODY;
                            bdBuf.size =  rd->size-(end-rd->buf+1);
                            bdBuf.buf = end+1;
                            /* todo: compare with \r\n */
                            cli->cb(cli, DS_MHB_CLIENT_CB_RESP_BODY, &bdBuf, cli->userData);
                            
                        }
                    } else if (rd->size ==0) {
                        LDS_ERR("DSStreamRead(header), (readed == 0)");
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(header), (readed == 0)", cli->userData);
                    } else {    /* <0: error */
                        LDS_ERR("DSStreamRead(header) failed");
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(header) failed", cli->userData);
                    }
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    if (rd->size > 0) {
                        struct DSConstBuf bdBuf;
                        
                        bdBuf.size =  rd->size;
                        bdBuf.buf = rd->buf;
                        cli->cb(cli, DS_MHB_CLIENT_CB_RESP_BODY, &bdBuf, cli->userData);
                    } else if (rd->size == 0) {
                        LDS_ERR("rd->size == 0\n");
                    } else {
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(body) failed", cli->userData);
                    }
                    break;
                case DS_MHB_CLIENT_ST_INITED:
                    LDS_ERR("Should not be here");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Should not be here", cli->userData);
                    break;
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                    LDS_ERR("Should not be here\n");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Should not be here", cli->userData);
                    break;
            }
            break;
        case DS_STREAM_CB_SENT:
            switch (cli->st) {
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                    reqHasBody = TRUE;
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                {
                    size_t sent;
                    
                    sent = DSStreamSend(strm, cli->buf+cli->filled, cli->bufSz-cli->filled);
                    if (sent >= 0) {
                        cli->filled += sent;
                    } else {    /* <0: error */
                        LDS_ERR("DSStreamWrite() failed");
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamWrite() failed", cli->userData);
                    }
                    if (cli->filled == cli->bufSz) {
                        DSFree(cli->buf);
                        cli->buf = NULL;
                        cli->bufSz = 0;
                        cli->filled = 0;
                        
                        if (reqHasBody) {
                            DSMhbClientBodyWriteFinished(cli);
                        } else {
                            cli->st = DS_MHB_CLIENT_ST_WRITE_BODY;
                            cli->cb(cli, DS_MHB_CLIENT_CB_BODY_WRITABLE, NULL, cli->userData);
                        }
                    }
                    
                }
                    break;
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                    cli->cb(cli, DS_MHB_CLIENT_CB_BODY_WRITABLE, NULL, cli->userData);
                    break;
                case DS_MHB_CLIENT_ST_INITED:
                    LDS_ERR("Should not be here\n");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Should not be here", cli->userData);
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    LDS_ERR("Should not be here\n");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Should not be here", cli->userData);
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
                    LDS_ERR("Unexpected connected\n");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Unexpected connected\n", cli->userData);
                    break;
            }
            break;
        case DS_STREAM_CB_DISCONNECTED: /* remote disconnected */
            switch(cli->st) {
                case DS_MHB_CLIENT_ST_INITED:
                    LDS_DBG("MHB connection closed gracefully\n");
                    break;
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                    LDS_ERR("Connection closed unexpectedly\n");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Connection closed unexpectedly", cli->userData);
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    cli->cb(cli, DS_MHB_CLIENT_CB_DISCONN, NULL, cli->userData);
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
                    LDS_ERR("Connection Error");
                    cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Connection Error", cli->userData);
                    break;
            }
            break;
    }
}

DSMhbClient* DSMhbClientNew(DSStream* strm, DSMhbClientCb cb, void* userData)
{
    DSMhbClient* cli;
    
    LDS_DBG_IN_FUNC();
    if (!(cli = (DSMhbClient*)DSZalloc(sizeof(DSMhbClient)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (DSObjectInit((DSObject*)cli)) {
        LDS_ERR_OUT(ERR_FREE_CLI, "\n");
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
    
    LDS_DBG_IN_FUNC();
    cli->bufSz = 2;    // last \r\n
    for (i=0; i<req->headersCount; i++) {
        cli->bufSz += (DSStrlen(req->headers[i])+2);
    }
    if (req->body) {
        cli->bufSz += req->bodySz;
    }
    
    if (!(cli->buf = DSMalloc(cli->bufSz))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
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

    if (DSStreamConnect(cli->strm)) {
        LDS_ERR_OUT(ERR_FREE_BUF, "\n");
    }
    return 0;
    
ERR_FREE_BUF:
    DSFree(cli->buf);
    cli->buf =  NULL;
ERR_OUT:
    return -1;
}

void DSMhbClientBodyWriteFinished(DSMhbClient* cli)
{
    LDS_DBG_IN_FUNC();
    if (!(cli->buf = DSMalloc(DS_MHB_HEADER_MAX_LEN))) {
        LDS_ERR("Malloc recieve buffer failed");
        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Malloc recieve buffer failed", cli->userData);
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    cli->bufSz = DS_MHB_HEADER_MAX_LEN;
    cli->filled = 0;
    cli->st = DS_MHB_CLIENT_ST_READ_RESP_HEADER;
    return;
ERR_DESTOY_BUF:
    DSFree(cli->buf);
    cli->buf = NULL;
ERR_OUT:
    return;
}

static void _DSMhbClientStopRequest(DSMhbClient* cli)
{
    switch (cli->st) {
        case DS_MHB_CLIENT_ST_INITED:
            break;
        case DS_MHB_CLIENT_ST_WRITE_REQ:
        case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
            DSMhbClientCleanupBuf(cli);
            break;
        case DS_MHB_CLIENT_ST_WRITE_BODY:
            DSMhbClientCleanupBuf(cli);
            break;
        case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
            DSMhbClientCleanupBuf(cli);
            break;
        case DS_MHB_CLIENT_ST_READ_RESP_BODY:
            DSMhbClientCleanupBuf(cli);
            break;
    }
    cli->st = DS_MHB_CLIENT_ST_INITED;
}

void DSMhbClientDestroy(DSMhbClient* cli)
{
    _DSMhbClientStopRequest(cli);
ERR_EXIT_OBJ:
    DSObjectExit((DSObject*)cli);
ERR_FREE_CLI:
    DSFree(cli);
}

typedef struct _DSSimpleHttpClient {
    DSStream* strm;
    DSMhbClient* mCli;
    
    unsigned int maxRespSize;
    
    int16_t statusCode;
    DSBuffer *respBuffer;
    
    size_t contentLength;
    
    void* userData;
    DSSimpleHttpClientCb cb;
}DSSimpleHttpClient;

static void DSSimpleHttpClientDestroy(DSSimpleHttpClient* shc)
{
ERR_DESTOY_RESP_BUFFER:
    DSBufferDestroy(shc->respBuffer);
ERR_DESTOY_MC:
    DSMhbClientDestroy(shc->mCli);
ERR_DESTOY_TCP_STRM:
    LDS_DBG_N();
    DSTcpClientDestroy((DSTcpClient*)shc->strm);
ERR_FREE_SHC:
    LDS_DBG_N();
    DSFree(shc);
    LDS_DBG_N();
}

static void* _MhbClientCb(DSMhbClient* mc, DSMhbClientCbReason reas, const void* data, void* userData)
{
    DSSimpleHttpClient *shc;
    
    shc = userData;
    switch (reas) {
        case DS_MHB_CLIENT_CB_ERROR:
            shc->cb(data, NULL, shc->userData);
            DSSimpleHttpClientDestroy(shc);
            return NULL;
            break;
        case DS_MHB_CLIENT_CB_BODY_WRITABLE:
            DSMhbClientBodyWriteFinished(mc);
            return mc;
            break;
        case DS_MHB_CLIENT_CB_RESP_HEADER:   /* data: DSConstBuf* */
        {
            const struct DSConstBuf *hdBuf;
            
            hdBuf = data;
            if (shc->statusCode == -1) {
                /*  HTTP/1.1 404 Not Found */
                const char* ch;
                
                if (!(ch = memchr(hdBuf->buf, ' ', hdBuf->size))) {
                    shc->cb("Wrong HTTP header\n", NULL, shc->userData);
                    shc->statusCode = 0;
                    DSSimpleHttpClientDestroy(shc);
                    return NULL;
                }
                
                ch++;
                
                if (!memcmp(ch, DS_CONST_STR_LEN("200"))) {
                    shc->statusCode = 200;
                    return mc;
                } else {
                    shc->statusCode = atoi(ch);
                    LDS_DBG("STATUS CODE: %d\n", shc->statusCode);
                    shc->cb("status code not 404", NULL, shc->userData);
                    DSSimpleHttpClientDestroy(shc);
                    return NULL;
                }
            } else {
                if (memcmp(hdBuf->buf, DS_CONST_STR_LEN("Content-Length:"))) {
                    return mc;
                }
                
                {const char *blank, *end;
                
                if (!(blank = memchr(hdBuf->buf, ' ', hdBuf->size))
                        || !(end = memchr(blank, '\r', hdBuf->size))) {
                    shc->cb("Content-Length header format wrong", NULL, shc->userData);
                    DSSimpleHttpClientDestroy(shc);
                    return NULL;
                    break;
                }
                
                if ((shc->contentLength = atoi((blank+1))) < 0) {
                    shc->cb("Content-Length < 0", NULL, shc->userData);
                    DSSimpleHttpClientDestroy(shc);
                    return NULL;
                }
                
                LDS_DBG("contentLength=%d\n", shc->contentLength);}
                return mc;
            }
            break;
        }
        case DS_MHB_CLIENT_CB_RESP_BODY:
        {
            const struct DSConstBuf *bBuf;
            
            bBuf = data;
            if ((DSBufferGetSize(shc->respBuffer)+bBuf->size) > shc->maxRespSize) {
                shc->cb("Resp body too large", NULL, shc->userData);
                DSSimpleHttpClientDestroy(shc);
                return NULL;
            } else {
                DSBufferCat(shc->respBuffer, bBuf->buf, bBuf->size);
                if (DSBufferGetSize(shc->respBuffer) == shc->contentLength) {
                    struct DSConstBuf respBuf;
                    
                    respBuf.buf = DSBufferGetPtr(shc->respBuffer);
                    respBuf.size = DSBufferGetSize(shc->respBuffer);
                    shc->cb(NULL, &respBuf, shc->userData);
                    DSSimpleHttpClientDestroy(shc);
                    return NULL;
                }
            }
            break;
        }
        case DS_MHB_CLIENT_CB_DISCONN:
        {
            if (DSBufferGetSize(shc->respBuffer) == shc->contentLength) {
                struct DSConstBuf respBuf;
                respBuf.buf = DSBufferGetPtr(shc->respBuffer);
                respBuf.size = DSBufferGetSize(shc->respBuffer);
                shc->cb(NULL, &respBuf, shc->userData);
                DSSimpleHttpClientDestroy(shc);
                return NULL;
            } else {
                shc->cb("Remote disconnected unexpectedly", NULL, shc->userData);
                DSSimpleHttpClientDestroy(shc);
                return NULL;
            }
            break;
        }
    }
    return NULL;
}

int DSSimpleHttpClientRequest(const uint8_t ip[4], const uint16_t port, struct DSMhbRequest* req, size_t maxRespSize, DSSimpleHttpClientCb cb, void* userData, void* evtBase)
{
    DSSimpleHttpClient *shc;

    if (!(shc = (DSSimpleHttpClient*)DSZalloc(sizeof(DSSimpleHttpClient)))) {
        LDS_ERR_OUT(ERR_OUT, "DSMalloc() failed\n");
    }
    
    if (!(shc->strm = (DSStream*)DSTcpClientNew(NULL, 0, ip, port))) {
        LDS_ERR_OUT(ERR_FREE_SHC, "strm == NULL\n");
    }
    
    if (evtBase) {
        DSStreamSetEventBase((DSStream*)shc->strm, evtBase);
    }

    if (!(shc->mCli = DSMhbClientNew(shc->strm, _MhbClientCb, shc))) {
        LDS_ERR_OUT(ERR_DESTOY_TCP_STRM, "DSMhbClientNew() failed");
    }

    if (!(shc->respBuffer = DSBufferNew(maxRespSize>>1, 16))) {
        LDS_ERR_OUT(ERR_DESTOY_MC, "DSBufferNew(%d, 16) failed", maxRespSize>>1);
    }

    if (DSMhbClientRequest(shc->mCli, req, NULL, NULL)) {
        LDS_ERR_OUT(ERR_DESTOY_RESP_BUFFER, "DSMhbClientRequest(%s) failed\n", req->headers[0]);
    }

    shc->statusCode = -1;
    shc->contentLength = -1;
    
    shc->cb = cb;
    shc->maxRespSize = maxRespSize;
    shc->userData = userData;
    return 0;
    
ERR_DESTOY_RESP_BUFFER:
    DSBufferDestroy(shc->respBuffer);
ERR_DESTOY_MC:
    DSMhbClientDestroy(shc->mCli);
ERR_DESTOY_TCP_STRM:
    DSTcpClientDestroy((DSTcpClient*)shc->strm);
ERR_FREE_SHC:
    DSFree(shc);
ERR_OUT:
    return -1;
}
