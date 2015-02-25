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
#ifndef _DS_LOG_H_
#define _DS_LOG_H_
#include "ds_plat_basic.h"
/*
	Define these macros before include this head file
#define ERROR_LOG_FILE "/var/log/tn_error.log"
#define MESSAGE_LOG_FILE "/var/log/tn_message.log"
#define DEBUG_LOG_FILE "/var/log/tn_debug.log"
*/

enum DS_LOG_TYPE{LOG_DEBUG=0, LOG_MESSAGE, LOG_ERROR};
int DSLogToFile(enum DS_LOG_TYPE type, const char* file_path, const char* format, ...);

#ifndef ERROR_LOG_FILE
#define DS_ERR(...) fprintf(stderr, "[E][%s:%d] ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__);
#else
#define DS_ERR(...) DSLogToFile(LOG_ERROR, ERROR_LOG_FILE, "[E][%s:%d] ", __FILE__, __LINE__); DSLogToFile(LOG_ERROR, ERROR_LOG_FILE, __VA_ARGS__);
#endif

#define DS_SYS_ERR(...)   {DS_ERR(__VA_ARGS__);DS_ERR("For %s\n", strerror(errno));}
#define DS_ERR_OUT(_dest, ...)   {DS_ERR(__VA_ARGS__); goto _dest;}
#define DS_SYS_ERR_OUT(_dest, ...)   {DS_SYS_ERR(__VA_ARGS__); goto _dest;}


#ifndef MESSAGE_LOG_FILE
#define DS_MSG(...) fprintf(stdout, "[M][%s:%d] ", __FILE__, __LINE__);	fprintf(stdout, __VA_ARGS__);
#else
#define DS_MSG(...) DSLogToFile(LOG_MESSAGE, MESSAGE_LOG_FILE, "[M][%s:%d] ", __FILE__, __LINE__); DSLogToFile(LOG_MESSAGE, MESSAGE_LOG_FILE, __VA_ARGS__);
#endif


#ifndef DEBUG_LOG_FILE
#define DS_DBG(...) fprintf(stdout, "[D][%s:%d] ", __FILE__, __LINE__); fprintf(stdout, __VA_ARGS__);
#else
#define DS_DBG(...) DSLogToFile(LOG_DEBUG, DEBUG_LOG_FILE, "[D][%s:%d]", __FILE__, __LINE__); DSLogToFile(LOG_DEBUG, DEBUG_LOG_FILE, __VA_ARGS__);
#endif

#endif
