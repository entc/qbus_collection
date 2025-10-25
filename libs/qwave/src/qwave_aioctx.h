#ifndef __QWAVE_AIOCTX__H
#define __QWAVE_AIOCTX__H 1

// cape includes
#include "sys/cape_types.h"
#include <sys/cape_err.h>
//-----------------------------------------------------------------------------

struct QWaveAioctx_s; typedef struct QWaveAioctx_s* QWaveAioctx; // use a simple version

                                    /* constructor: create a new instance of the asyncronous IO context */
__CAPE_LIBEX     QWaveAioctx        qwave_aioctx_new    ();

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qwave_aioctx_del    (QWaveAioctx*);

//-----------------------------------------------------------------------------

                                    /* initialize the AIO */
__CAPE_LIBEX     int                qwave_aioctx_open   (QWaveAioctx, CapeErr err);

__CAPE_LIBEX     int                qwave_aioctx_add    (QWaveAioctx, void** p_handle, CapeErr err);

//-----------------------------------------------------------------------------

                                    /* waits until next event or the timeout has been reaced */
__CAPE_LIBEX     int                qwave_aioctx_next   (QWaveAioctx, number_t timeout_in_ms, CapeErr err);

                                    /* waits until terminatation */
__CAPE_LIBEX     int                qwave_aioctx_wait   (QWaveAioctx, CapeErr err);

//-----------------------------------------------------------------------------

#endif
