#ifndef __CAPE_SYS__AIO__H
#define __CAPE_SYS__AIO__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//=============================================================================

struct CapeAio_s; typedef struct CapeAio_s* CapeAio;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAio           cape_aio_new          (void);              // allocate memory

__CAPE_LIBEX   void              cape_aio_del          (CapeAio*);          // stop and release memory

__CAPE_LIBEX   int               cape_aio_init         (CapeAio, CapeErr);

__CAPE_LIBEX   int               cape_aio_wait         (CapeAio, CapeErr);

__CAPE_LIBEX   int               cape_aio_next         (CapeAio, number_t timeout, CapeErr);

__CAPE_LIBEX   void              cape_aio_kill         (CapeAio);

//-----------------------------------------------------------------------------

struct CapeAioItem_s; typedef struct CapeAioItem_s* CapeAioItem;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioItem       cape_aio_add          (CapeAio, void* handle, CapeErr);

__CAPE_LIBEX   void              cape_aio_rm           (CapeAio, CapeAioItem*);

//-----------------------------------------------------------------------------

typedef void     (__STDCALL *fct_cape_aio_item__on_event)      (void* user_ptr, void* handle);

                                 /* sets the callback method and user pointer for upcoming events */
__CAPE_LIBEX     void            cape_aio_item_set     (CapeAioItem, void* user_ptr, fct_cape_aio_item__on_event on_event, fct_cape_aio_item__on_event on_done);

                                 /* returns the original handle */
__CAPE_LIBEX     void*           cape_aio_item_get     (CapeAioItem);

//-----------------------------------------------------------------------------

#endif
