
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
#include "lds_log.h"
#include "ds_dns.h"

#include "lwip/ip.h"


static void found(const char *name, struct ip_addr *ipaddr, void *userData)
{
    uint8_t ip[4];
    
    ip[0] = ip4_addr1(ipaddr);
    ip[1] = ip4_addr2(ipaddr);
    ip[2] = ip4_addr3(ipaddr);
    ip[3] = ip4_addr4(ipaddr);
    
    if (ipaddr) {
        cb(name, ip, userData);
    } else {
        cb(name, NULL, userData);
    }
}

int DSSimpleGetAddrInfo(const char *hostname, int timeout, DSSimpleGetAddrInfoCb cb, void *userData)
{
    struct ip_addr addr;
    err_t ret;
    
    ret = dns_gethostbyname(hostname, &addr, found, userData);
    if (ret == ERR_OK) {
        uint8_t ip[4];
    
        ip[0] = ip4_addr1(&addr);
        ip[1] = ip4_addr2(&addr);
        ip[2] = ip4_addr3(&addr);
        ip[3] = ip4_addr4(&addr);
        cb(hostname, ip, userData);
        return 0;
    } else if (ret == ERR_INPROGRESS) {
        return 0;
    } else {
        return -1;
    }
}
