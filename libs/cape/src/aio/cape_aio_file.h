#ifndef __CAPE_AIO__FILE__H
#define __CAPE_AIO__FILE__H 1

#include "sys/cape_export.h"
#include "aio/cape_aio_ctx.h"

//=============================================================================

struct CapeAioFileReader_s; typedef struct CapeAioFileReader_s* CapeAioFileReader;

//-----------------------------------------------------------------------------

typedef void       (__STDCALL *fct_cape_aio_file__on_read)       (void* ptr, CapeAioFileReader freader, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioFileReader  cape_aio_freader_new           (void* handle, void* ptr, fct_cape_aio_file__on_read);

__CAPE_LIBEX   int                cape_aio_freader_add           (CapeAioFileReader*, CapeAioContext);

//=============================================================================

#endif

