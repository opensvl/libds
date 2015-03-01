#ifndef _LDS_LOG_H_
#define _LDS_LOG_H_

#ifdef _USE_LDS_LOG_

#include	<ds_log.h>
#define LDS_MSG	DS_MSG
#define LDS_DBG	DS_DBG
#define	LDS_ERR	DS_ERR
#define LDS_ERR_OUT DS_ERR_OUT
#define LDS_SYS_ERR    DS_SYS_ERR
#define LDS_SYS_ERR_OUT    DS_SYS_ERR_OUT

#define LDS_DBG_N DS_DBG_N
#define LDS_DBG_IN_FUNC DS_DBG_IN_FUNC
#define LDS_DBG_OUT_FUNC DS_DBG_OUT_FUNC
#define LDS_DBG_TS  DS_DBG_TS

#elif defined _USE_GLOBAL_LOG_

#include <glb_log.h>
#define LDS_DBG	GLB_DBG
#define LDS_ERR	GLB_ERR
#define LDS_ERR_OUT GLB_ERR_OUT
#define LDS_SYS_ERR    GLB_SYS_ERR
#define LDS_SYS_ERR_OUT GLB_SYS_ERR_OUT
#define LDS_DBG_N GLB_DBG_N
#define LDS_DBG_IN_FUNC GLB_DBG_IN_FUNC
#define LDS_DBG_OUT_FUNC GLB_DBG_OUT_FUNC
#define LDS_DBG_TS  GLB_DBG_TS

#else
#include <ds_log.h>

#define LDS_MSG	DS_MSG
#define LDS_DBG(...)
#define LDS_ERR(...)
#define LDS_ERR_OUT(__dest, ...)    goto __dest
#define LDS_SYS_ERR(...)
#define LDS_SYS_ERR_OUT(__dest, ...) goto __dest
#define LDS_DBG_N()
#define LDS_DBG_IN_FUNC()
#define LDS_DBG_OUT_FUNC()
#define LDS_DBG_TS()

#endif	/* _LDS_LOG_ */

#endif	/* _LDS_LOG_H_ */
