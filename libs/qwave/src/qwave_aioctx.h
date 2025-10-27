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

// the list of event return codes
#define QWAVE_EVENT_RESULT__CONTINUE         0
#define QWAVE_EVENT_RESULT__ERROR_CLOSED     1
#define QWAVE_EVENT_RESULT__TRYAGAIN         2

typedef int     (__STDCALL *fct_qwave__on_aio_event)      (void* user_ptr, void* handle);

//-----------------------------------------------------------------------------

                                    /* initialize the AIO */
__CAPE_LIBEX     int                qwave_aioctx_open   (QWaveAioctx, CapeErr err);

__CAPE_LIBEX     int                qwave_aioctx_add    (QWaveAioctx, void** p_handle, void* user_ptr, fct_qwave__on_aio_event fct, CapeErr err);

//-----------------------------------------------------------------------------

                                    /* waits until next event or the timeout has been reaced */
__CAPE_LIBEX     int                qwave_aioctx_next   (QWaveAioctx, number_t timeout_in_ms, CapeErr err);

                                    /* waits until terminatation */
__CAPE_LIBEX     int                qwave_aioctx_wait   (QWaveAioctx, CapeErr err);

//-----------------------------------------------------------------------------

#endif
