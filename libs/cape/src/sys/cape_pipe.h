#ifndef __CAPE_SYS__PIPE__H
#define __CAPE_SYS__PIPE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//=============================================================================

__CAPE_LIBEX   void*         cape_pipe_create               (const char* path, const char* name, CapeErr err);

__CAPE_LIBEX   void*         cape_pipe_connect              (const char* path, const char* name, CapeErr err);

__CAPE_LIBEX   void*         cape_pipe_create_or_connect    (const char* path, const char* name, CapeErr err);

//-----------------------------------------------------------------------------

#endif

