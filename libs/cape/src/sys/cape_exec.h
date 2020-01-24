#ifndef __CAPE_SYS__EXEC__H
#define __CAPE_SYS__EXCE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//=============================================================================

struct CapeExec_s; typedef struct CapeExec_s* CapeExec;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeExec          cape_exec_new          (void);             // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_exec_del          (CapeExec*);        // release memory

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               cape_exec_run          (CapeExec, const char* executable, CapeErr);

__CAPE_LIBEX   void              cape_exec_append_s     (CapeExec, const char* parameter);

__CAPE_LIBEX   void              cape_exec_append_fmt   (CapeExec, const char* format, ...);

__CAPE_LIBEX   const CapeString  cape_exec_get_stdout   (CapeExec);

__CAPE_LIBEX   const CapeString  cape_exec_get_stderr   (CapeExec);

//-----------------------------------------------------------------------------

#endif



