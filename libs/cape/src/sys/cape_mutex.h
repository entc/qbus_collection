#ifndef __CAPE_SYS__MUTEX__H
#define __CAPE_SYS__MUTEX__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//=============================================================================

typedef void* CapeMutex;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeMutex         cape_mutex_new         (void);                // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_mutex_del         (CapeMutex*);          // release memory

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              cape_mutex_lock        (CapeMutex);

__CAPE_LIBEX   void              cape_mutex_unlock      (CapeMutex);

//-----------------------------------------------------------------------------

#endif



