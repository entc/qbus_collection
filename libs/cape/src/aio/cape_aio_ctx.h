#ifndef __CAPE_AIO__AIO__H
#define __CAPE_AIO__AIO__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"

//=============================================================================

struct CapeAioHandle_s; typedef struct CapeAioHandle_s* CapeAioHandle;

// returns the mask which should be observed
typedef int                (__STDCALL *fct_cape_aio_onEvent)   (void* ptr, int hflags, unsigned long events, unsigned long);
typedef void               (__STDCALL *fct_cape_aio_onUnref)   (void* ptr, CapeAioHandle, int close);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioHandle     cape_aio_handle_new            (int hflags, void* ptr, fct_cape_aio_onEvent, fct_cape_aio_onUnref);

__CAPE_LIBEX   void              cape_aio_handle_del            (CapeAioHandle*);

//-----------------------------------------------------------------------------

struct CapeAioContext_s; typedef struct CapeAioContext_s* CapeAioContext;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioContext    cape_aio_context_new           (void);             // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_aio_context_del           (CapeAioContext*);  // release memory

__CAPE_LIBEX   int               cape_aio_context_open          (CapeAioContext, CapeErr);   // open the context, now wait or next can be used to gather events

__CAPE_LIBEX   int               cape_aio_context_close         (CapeAioContext, CapeErr);   // close the context and all handles will be triggered for destruction

//-----------------------------------------------------------------------------

               // blocking the thread, until the context was closed
__CAPE_LIBEX   int               cape_aio_context_wait          (CapeAioContext, CapeErr);

               // waits until next event or timeout occours
__CAPE_LIBEX   int               cape_aio_context_next          (CapeAioContext, long timeout_in_ms, CapeErr);

//-----------------------------------------------------------------------------

#define CAPE_AIO_NONE     0x0000
#define CAPE_AIO_DONE     0x0001
#define CAPE_AIO_ABORT    0x0002
#define CAPE_AIO_WRITE    0x0004
#define CAPE_AIO_READ     0x0008
#define CAPE_AIO_ALIVE    0x0010
#define CAPE_AIO_TIMER    0x0020
#define CAPE_AIO_ERROR    0x0040

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               cape_aio_context_add           (CapeAioContext, CapeAioHandle aioh, void* handle, number_t option);         // add handle to event queue

               // modify handle
__CAPE_LIBEX   void              cape_aio_context_mod           (CapeAioContext, CapeAioHandle aioh, void* handle, int hflags, number_t option);

//-----------------------------------------------------------------------------

               // add handle for a signals to return a specific status
__CAPE_LIBEX   int               cape_aio_context_set_interupts (CapeAioContext, int sigint, int term, CapeErr);

//=============================================================================

#endif
