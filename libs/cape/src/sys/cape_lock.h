#ifndef __CAPE_SYS__LOCK__H
#define __CAPE_SYS__LOCK__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_map.h"

//=============================================================================

struct CapeLock_s; typedef struct CapeLock_s* CapeLock;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeLock          cape_lock_new          (void);                // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_lock_del          (CapeLock*);          // release memory

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeMapNode       cape_lock_get_s        (CapeLock, const CapeString);

__CAPE_LIBEX   void              cape_lock_get_fmt      (CapeLock, const CapeString format, ...);

__CAPE_LIBEX   void              cape_lock_release      (CapeLock, CapeMapNode);

//-----------------------------------------------------------------------------

#endif



