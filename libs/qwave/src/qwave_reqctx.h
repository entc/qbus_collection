#ifndef __QWAVE_REQCTX__H
#define __QWAVE_REQCTX__H 1

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_map.h>

#include "qwave_conctx.h"

//-----------------------------------------------------------------------------

struct QWaveReqctx_s; typedef struct QWaveReqctx_s* QWaveReqctx;

                                    /* constructor: create a new instance of the qwave class */
__CAPE_LIBEX     QWaveReqctx        qwave_reqctx_new            (QWaveConctx, QWaveConfig);

                                    /* decrease reference counter */
__CAPE_LIBEX     void               qwave_reqctx_dec            (QWaveReqctx*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     void               qwave_reqctx_clr            (QWaveReqctx);

__CAPE_LIBEX     void               qwave_reqctx_set_url        (QWaveReqctx, CapeString* p_url);

__CAPE_LIBEX     void               qwave_reqctx_set_ohf        (QWaveReqctx, CapeString* p_field);

__CAPE_LIBEX     void               qwave_reqctx_set_ohv        (QWaveReqctx, CapeString* p_value);

__CAPE_LIBEX     void               qwave_reqctx_set_complete   (QWaveReqctx);

__CAPE_LIBEX     void               qwave_reqctx_set            (QWaveReqctx, int upgrade, int keep_alive, const char* method);

__CAPE_LIBEX     int                qwave_reqctx_is_complete    (QWaveReqctx);

__CAPE_LIBEX     void               qwave_reqctx_exec           (QWaveReqctx);

//-----------------------------------------------------------------------------

#endif

