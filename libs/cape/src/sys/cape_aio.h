#ifndef __CAPE_SYS__AIO__H
#define __CAPE_SYS__AIO__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//=============================================================================

struct CapeAio_s; typedef struct CapeAio_s* CapeAio;

//-----------------------------------------------------------------------------

                                 /* constructor to allocate memory */
__CAPE_LIBEX   CapeAio           cape_aio_new          (void);

                                 /* closes all file descriptors, release memory  */
__CAPE_LIBEX   void              cape_aio_del          (CapeAio*);

                                 /* open all file descriptors, this may fail */
__CAPE_LIBEX   int               cape_aio_init         (CapeAio, CapeErr);

                                 /* wait infinite until the blocking state has been stopped / killed */
__CAPE_LIBEX   int               cape_aio_wait         (CapeAio, CapeErr);

                                 /* wait for next event or timeout */
__CAPE_LIBEX   int               cape_aio_next         (CapeAio, number_t timeout, CapeErr);

                                 /* using an internal mechanism to leave blocking state */
__CAPE_LIBEX   void              cape_aio_stop         (CapeAio);

                                 /* trigger a signal to terminate the blocking state */
__CAPE_LIBEX   void              cape_aio_kill         (CapeAio);

//-----------------------------------------------------------------------------

struct CapeAioItem_s; typedef struct CapeAioItem_s* CapeAioItem;

//-----------------------------------------------------------------------------

                                 /* adds a file descriptor to the event handler, returns ref to it */
__CAPE_LIBEX   CapeAioItem       cape_aio_add          (CapeAio, void* handle, CapeErr);

                                 /* removes the file descriptor from the event handler */
__CAPE_LIBEX   void              cape_aio_rm           (CapeAio, CapeAioItem*);

                                 /* returns the original handle */
__CAPE_LIBEX   void*             cape_aio_item_get     (CapeAioItem);

//-----------------------------------------------------------------------------

typedef void     (__STDCALL *fct_cape_aio_item__on_event)      (void* user_ptr, void* handle);

                                 /* sets the callback method and user pointer for upcoming events */
__CAPE_LIBEX   void              cape_aio_item_set     (CapeAioItem, void* user_ptr, fct_cape_aio_item__on_event on_event, fct_cape_aio_item__on_event on_done);

//-----------------------------------------------------------------------------

                                 /* adds timer event, returns ref to it */
__CAPE_LIBEX   CapeAioItem       cape_aio_add__timer   (CapeAio, number_t interval_in_ms, CapeErr);

//-----------------------------------------------------------------------------

#endif
