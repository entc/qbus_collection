#ifndef __QWAVE__H
#define __QWAVE__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include <aio/cape_aio_ctx.h>
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_list.h>
#include <stc/cape_map.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

                                    /* constructor: create a new instance of the qwave class */
__CAPE_LIBEX     QWave              qwave_new           (const CapeString host, number_t port, CapeUdc parameters);

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qwave_del           (QWave*);

                                    /* runs the webserver in the same thread in blocking mode */
__CAPE_LIBEX     int                qwave_run           (QWave, CapeErr err);

                                    /* runs the webserver in an extra thread */
__CAPE_LIBEX     int                qwave_run__d        (QWave, CapeErr err);

                                    /* waits until the extra thread terminates */
__CAPE_LIBEX     int                qwave_wait          (QWave, CapeErr err);

//-----------------------------------------------------------------------------

typedef int     (__STDCALL *fct_qwave__on_http_request)      (void* user_ptr, QWaveReqCtx reqctx, CapeErr err);

                                    /* register a path on which a callback will be used */
__CAPE_LIBEX     void               qwave_reg__path     (QWave, const CapeString path, void* user_ptr, fct_qwave__on_http_request);

//-----------------------------------------------------------------------------

typedef int     (__STDCALL *fct_qwebs__on_error_raise)        (void* user_ptr, number_t type, QWebsRequest);

__CAPE_LIBEX     void               qwave_reg__raise    (QWave, void* user_ptr, fct_qwebs__on_error_raise);

//-----------------------------------------------------------------------------

#endif
