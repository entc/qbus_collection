#ifndef __CAPE_AIO__TIMER__H
#define __CAPE_AIO__TIMER__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "aio/cape_aio_ctx.h"

//=============================================================================

struct CapeAioTimer_s; typedef struct CapeAioTimer_s* CapeAioTimer;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioTimer       cape_aio_timer_new            ();

__CAPE_LIBEX   int                cape_aio_timer_add            (CapeAioTimer*, CapeAioContext);

//-----------------------------------------------------------------------------

typedef int        (__STDCALL *fct_cape_aio_timer_onEvent)      (void* ptr);   // should return TRUE or FALSE

__CAPE_LIBEX   int                cape_aio_timer_set            (CapeAioTimer, long inMs, void*, fct_cape_aio_timer_onEvent, CapeErr);

//=============================================================================

#endif
