#ifndef __QWAVE_REQCTX__H
#define __QWAVE_REQCTX__H 1

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QWaveReqctx_s; typedef struct QWaveReqctx_s* QWaveReqctx;

                                    /* constructor: create a new instance of the qwave class */
__CAPE_LIBEX     QWaveReqctx        qwave_reqctx_new    ();

                                    /* decrease reference counter */
__CAPE_LIBEX     void               qwave_reqctx_dec    (QWaveReqctx*);

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

#endif

