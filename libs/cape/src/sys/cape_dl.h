#ifndef __CAPE_SYS__DL__H
#define __CAPE_SYS__DL__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//=============================================================================

struct CapeDl_s; typedef struct CapeDl_s* CapeDl;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeDl            cape_dl_new            (void);             // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_dl_del            (CapeDl*);          // release memory

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               cape_dl_load           (CapeDl, const char* path, const char* name, CapeErr);

__CAPE_LIBEX   void*             cape_dl_funct          (CapeDl, const char* name, CapeErr);   // returns the function pointer or NULL if not found

//-----------------------------------------------------------------------------

#endif


