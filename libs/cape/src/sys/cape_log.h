#ifndef __CAPE_SYS__LOG__H
#define __CAPE_SYS__LOG__H 1

#include "sys/cape_export.h"

//-----------------------------------------------------------------------------

typedef enum
{
  CAPE_LL_FATAL   = 1,
  CAPE_LL_ERROR   = 2,
  CAPE_LL_WARN    = 3,
  CAPE_LL_INFO    = 4,
  CAPE_LL_DEBUG   = 5,
  CAPE_LL_TRACE   = 6
  
} CapeLogLevel;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              cape_log_msg           (CapeLogLevel, const char* unit, const char* method, const char* msg);

__CAPE_LIBEX   void              cape_log_fmt           (CapeLogLevel, const char* unit, const char* method, const char* format, ...);

//-----------------------------------------------------------------------------

struct CapeFileLog_s; typedef struct CapeFileLog_s* CapeFileLog;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeFileLog       cape_log_new           (const char* filename);

__CAPE_LIBEX   void              cape_log_del           (CapeFileLog*);

//-----------------------------------------------------------------------------

#endif
