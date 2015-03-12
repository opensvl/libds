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
#include "ds_event.h"

#define user_procTaskPrio     0
#define user_procTaskQueueLen    1

static os_event_t user_procTaskQueue[user_procTaskQueueLen];

static void Loop(os_event_t *events)
{
    os_delay_us(10000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

DSEventBase* DSEventBaseNew()
{
    system_os_task(Loop, 0 , user_procTaskQueue, DS_ARRAY_SIZE(user_procTaskQueue));
    system_os_post(user_procTaskPrio, 0, 0 );
    return (DSEventBase*)user_procTaskQueueLen;
}
