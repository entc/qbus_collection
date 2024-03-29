#ifndef __CAPE_SYS__THREAD__H
#define __CAPE_SYS__THREAD__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "sys/cape_types.h"

//=============================================================================

typedef int (__STDCALL *cape_thread_worker_fct)(void* ptr);
typedef void (__STDCALL *cape_thread_on_done)(void* ptr);

struct CapeThread_s; typedef struct CapeThread_s* CapeThread;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeThread      cape_thread_new         (void);                // allocate memory and initialize the object

__CAPE_LIBEX   void            cape_thread_del         (CapeThread*);

__CAPE_LIBEX   void            cape_thread_start       (CapeThread, cape_thread_worker_fct, void* ptr);

__CAPE_LIBEX   void            cape_thread_join        (CapeThread);

__CAPE_LIBEX   void            cape_thread_cancel      (CapeThread);

__CAPE_LIBEX   void            cape_thread_cb          (CapeThread, cape_thread_on_done);

__CAPE_LIBEX   void            cape_thread_signal      (CapeThread);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void            cape_thread_nosignals   ();

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void            cape_thread_sleep       (unsigned long milliseconds);

                               /* returns the available physical cores of the system */
__CAPE_LIBEX   number_t        cape_thread_concurrency ();

//-----------------------------------------------------------------------------

#endif



