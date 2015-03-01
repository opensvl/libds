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
#include "ds_list.h"

void DSListNodeInit(DSListNode* node)
{
    node->prev=node;
    node->next=node;
}

void DSListNodeInsertNext(DSListNode* node, DSListNode* toIns)
{
    node->next->prev = toIns;
    toIns->next = node->next;
    toIns->prev = node;
    node->next = toIns;
}

void DSListNodeInsertPre(DSListNode* node, DSListNode* toIns)
{
    node->prev->next = toIns;
    toIns->next = node;
    toIns->prev = node->prev;
    node->prev = toIns;
}

int DSListNodeDel(DSListNode* node)
{
    if(node->next == node)
        return -1;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node->prev = node;
    return 0;
}

