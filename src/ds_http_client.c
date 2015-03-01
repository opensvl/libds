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
    int processed, pos, parsed, start, filled;
    BOOL reqHasBody = FALSE;
    DSMhbClient *cli;
    struct DSStreamRecvedData *rd;
    
    cli = userData;
    rd = data;
    
    switch (reas) {
        case DS_STREAM_CB_RECVED:
            switch (cli->st) {
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                    /* read response and parse with DSStreamFinder */
#if 0   /* todo */
                    processed = rd->size;
                    if (processed > 0) {
                        parsed = 0;
                        start = 0;
                        while (DSStreamFinderParse(&cli->sf, cli->buf+cli->filled, readed-parsed, &pos)) {    /* find \r\n */
                            parsed += (pos+1)
                            if (pos == 1 || (cli->buf[start-1]=='\n')) {   /* find \r\n\r\n  end */
                                start = -1; /* flag for header end */
                                break;
                            } else {
                                cli->buf[cli->filled+parsed+pos-1] = '\0';
                                cli->cb(cli, DS_MHB_CLIENT_CB_RESP_HEADER, cli->buf[start], cli->userData);
                            }
                            start = cli->filled+parsed;
                        }
                        
                        if (processed > parsed) {
                            memcpy(cli->buf, cli->buf+cli->filled+parsed, readed-parsed);
                            cli->filled = readed-parsed;
                            if (start == -1) {
                                DSStreamFinderDestroy(&cli->sf);
                                cli->st = DS_MHB_CLIENT_ST_READ_RESP_BODY;
                                cli->cb(cli, DS_MHB_CLIENT_CB_RESP_BODY, cli->buf, cli->userData);
                            }
                        }
                    } else if (processed ==0) {    /* header buffer is full */
                        if (cli->bufSz == cli->filled) {
                            cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Recieved header so large", cli->userData);
                        } else {
                            cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "Server closed connection unexpectedly", cli->userData);
                        }
                    } else {    /* <0: error */
                        cli->cb(cli, DS_MHB_CLIENT_CB_ERROR, "DSStreamRead(header) failed", cli->userData);
                    }
#endif
                    break;
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    processed = DSStreamRead(strm, cli->buf, cli->bufSz);
                    if (processed > 0) {
                        cli->cb(cli, DS_MHB_CLIENT_CB_RESP_BODY, cli->buf, cli->userData);
                    } else if (processed == 0) {
                        cli->cb(cli, DS_MHB_CLIENT_CB_RESP_END, NULL, cli->userData);
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
                    processed = DSStreamSend(strm, cli->buf+cli->filled, cli->bufSz-cli->filled);
                    if (processed >= 0) {
                        cli->filled += filled;
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
                case DS_MHB_CLIENT_ST_WRITE_REQ_HEADERS:
                    StrmCb(strm, DS_STREAM_CB_SENT, NULL, userData);    /* work around */
                    break;
                case DS_MHB_CLIENT_ST_WRITE_REQ:
                case DS_MHB_CLIENT_ST_WRITE_BODY:
                case DS_MHB_CLIENT_ST_READ_RESP_HEADER:
                case DS_MHB_CLIENT_ST_READ_RESP_BODY:
                    /* todo */
                    break;
            }
            break;
        case DS_STREAM_CB_DISCONNECTED:
            break;
        case DS_STREAM_CB_ERROR:
            break;
    }
}

DSMhbClient* DSMhbClientNew(DSStream* strm, DSMhbClientCb cb, void* userData)
{
    DSMhbClient* cli;
    
    if (!(cli = (DSMhbClient*)DSZalloc(sizeof(DSMhbClient)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    
    cli->strm = strm;
    DSStreamSetCb(cli->strm, StrmCb, cli);
    cli->userData = userData;
    cli->cb = cb;
    cli->st = DS_MHB_CLIENT_ST_INITED;
    return cli;
    
ERR_OBJECT_DESTY:
    DSObjectDestroy(cli);
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
ERR_OBJECT_DESTY:
    DSObjectDestroy(cli);
}

