#ifndef __QWAVE__H
#define __QWAVE__H 1

// cape includes
#include "sys/cape_types.h"
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_udc.h>

#include "qwave_conctx.h"
//-----------------------------------------------------------------------------

struct QWave_s; typedef struct QWave_s* QWave;

                                    /* constructor: create a new instance of the qwave class */
__CAPE_LIBEX     QWave              qwave_new           (CapeUdc parameters);

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qwave_del           (QWave*);

                                    /* runs the webserver in the same thread in blocking mode */
__CAPE_LIBEX     int                qwave_run           (QWave, CapeErr err);

                                    /* runs the webserver in an extra thread */
__CAPE_LIBEX     int                qwave_run__d        (QWave, CapeErr err);

                                    /* terminate the extra thread */
__CAPE_LIBEX     void               qwave_stop          (QWave);

//-----------------------------------------------------------------------------

typedef int     (__STDCALL *fct_qwave__on_http_request)      (void* user_ptr, QWaveConctx reqctx, CapeErr err);

                                    /* register a path on which a callback will be used */
__CAPE_LIBEX     void               qwave_reg__path     (QWave, const CapeString path, void* user_ptr, fct_qwave__on_http_request);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     void               qwave_reg__ws       (QWave, void* user_ptr, fct_qwave__on_ws_upgrade, fct_qwave__on_ws_message, fct_qwave__on_ws_destroy);

//-----------------------------------------------------------------------------

#endif
