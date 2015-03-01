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
#ifndef _DS_LIST_H_
#define _DS_LIST_H_

#include "ds_plat_basic.h"
#include "ds_utils.h"

typedef struct _DSListNode {
    struct _DSListNode *prev;
    struct _DSListNode *next;
} DSListNode;

void DSListNodeInit(DSListNode* node);

#define DSListNodeExit(...)

#define DSListNodeGetNext(__node) ((__node)->next)

#define DSListNodeGetPrev(__node) ((__node)->prev)

#define DSListGetHead(__node) (__node)

#define DSListGetTail(__node) DSListGetPrev(__node)

#define DSListNodeGetNextContainer(__ptr, __type, __member) DS_CONTAINER_OF((__ptr)->next, __type, __member)

#define DSListNodeGetPrevContainer(__ptr, __type, __member) DS_CONTAINER_OF((__ptr)->prev, __type, __member)

void DSListNodeInsertNext(DSListNode* node, DSListNode* toIns);

void DSListNodeInsertPre(DSListNode* node, DSListNode* toIns);

#define DSListAppend  DSListInsertPre

#define DSListPrepend(__node, __to_ins) ({DSListInsertPre(__node, __to_ins);__to_ins;})

int DSListNodeDel(DSListNode* node);

#define DSListNodeContainerGetNext(__CNode, type, member) \
    DS_CONTAINER_OF((__CNode)->member.next, type, member)
    
#define DSListNodeContainerGetPrev(__CNode, type, member) \
	 DS_CONTAINER_OF((__CNode)->member.prev, type, member)

#define DSListForeach(__head, __node)   \
    DSListNode* __node;\
    for(__node=(__head)->next; __node!=__head; __node=(__node)->next)

#define DSListForeachSafe(__head, __node)\
    DSListNode *__node, *__nextNode;\
    for(__node=(__head)->next, __nextNode=(__node)->next; __node!=__head; __node=__nextNode, __nextNode=(__node)->next)


#define DSListForeachContainer(__Head, __CNode, __type, __member)\
    __type *__CNode;\
    DSListNode* __next;\
    for(__next=(__Head)->next, __CNode=DS_CONTAINER_OF(__next, __type, __member);\
            __next != __Head; \
            __next=(__next)->next, __CNode=DS_CONTAINER_OF(__next, __type, __member)\
            )

#define DSListForeachContainerSafe(__Head, __CNode, __type, __member)\
    __type *__CNode;\
    DSListNode* __next, *__tmpNext;\
    for(__next=(__Head)->next, __CNode=DS_CONTAINER_OF(__next, __type, __member),  __tmpNext=(__next)->next;\
            __next != __Head;\
            __next=__tmpNext,  __CNode=DS_CONTAINER_OF(__next, __type, __member), __tmpNext=(__next)->next\
            )

#define DSListContainerForeach(__CHead, __CNode, __type, __member)\
    __type *__CNode;\
    for(__CNode=DS_CONTAINER_OF((__CHead)->__member.next, __type, __member); __CNode != __CHead; __CNode=DS_CONTAINER_OF((__CNode)->__member.next, __type, __member))

#define DSListContainerForeachSafe(__CHead, __CNode, __type, __member)\
    __type *__CNode, *__nextNode;\
    for(__CNode=DS_CONTAINER_OF((__CHead)->__member.next, __type, __member),__nextNode=DS_CONTAINER_OF((__CNode)->__member.next, __type, __member);\
            __CNode != __CHead;\
            __CNode=__nextNode, __nextNode=DS_CONTAINER_OF((__CNode)->__member.next, __type, __member)\
            )

#endif
