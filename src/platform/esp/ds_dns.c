
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
#include "user_interface.h"
#include "espconn.h"
#include "lds_log.h"
#include "ds_dns.h"

typedef struct EspGetAddrInfo {
    DSGetAddrInfo dGai;
    
    struct espconn ec;
    ip_addr_t ip;
    
    DSGetAddrInfoCb cb;
    void *userdData;
}EspGetAddrInfo;

static void ICACHE_FLASH_ATTR DnsFound(const char *name, ip_addr_t *ipaddr, void *arg)
{
    EspGetAddrInfo *eGai;
    struct DSAddrInfo ai;
    
    eGai = ((struct espconn*)arg)->reverse;
    
    ai.ip = (uint8_t*)ipaddr->addr;
    
    eGai->cb((DSGetAddrInfo*)eGai, NULL, &ai, eGai->userdData);
}

DSGetAddrInfo* DSGetAddrInfoNew(const char *nodename, const char *servname,
        const struct DSAddrInfo* hintsIn,
        DSGetAddrInfoCb cb, void* userData, 
        void* evtBase)
{
    DSGetAddrInfo* gai;
    EspGetAddrInfo* eGai;
    int ret;
    
    if (!(gai = DSZalloc(sizeof(EspGetAddrInfo)))) {
        LDS_ERR_OUT(ERR_OUT, "\n");
    }
    eGai = (EspGetAddrInfo*)gai;
    
    eGai->ec.reverse = eGai;
    ret = espconn_gethostbyname(&eGai->ec, "baidu.com", &eGai->ip, DnsFound);
    
    if (ret != ESPCONN_OK && ret != ESPCONN_INPROGRESS) {
        LDS_ERR_OUT(ERR_FREE_GAI, "espconn_gethostbyname() failed\n");
    }
    
    return gai;
ERR_DEL_CONN:
    espconn_delete(&eGai->ec);
ERR_FREE_GAI:
    DSFree(gai);
ERR_OUT:
    return NULL;
}

void DSGetAddrInfoDestroy(DSGetAddrInfo *gai)
{
    EspGetAddrInfo* eGai;
    
    eGai = (EspGetAddrInfo*)gai;
    
ERR_DEL_CONN:
    espconn_delete(&eGai->ec);
ERR_FREE_GAI:
    DSFree(gai);
}
