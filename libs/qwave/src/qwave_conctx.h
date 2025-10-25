#ifndef __QWAVE_CONCTX__H
#define __QWAVE_CONCTX__H 1

// cape includes
#include "sys/cape_types.h"
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QWaveConctx_s; typedef struct QWaveConctx_s* QWaveConctx; // use a simple version

                                    /* constructor: create a new instance of the qwave class */
__CAPE_LIBEX     QWaveConctx        qwave_conctx_new    ();

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qwave_conctx_del    (QWaveConctx*);

//-----------------------------------------------------------------------------

#endif
