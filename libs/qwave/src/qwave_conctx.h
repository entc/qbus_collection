#ifndef __QWAVE_CONCTX__H
#define __QWAVE_CONCTX__H 1

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <sys/cape_queue.h>
#include <stc/cape_str.h>
#include <stc/cape_udc.h>

#include "qwave_config.h"
#include "qwave_response.h"
#include "qwave_aioctx.h"

//-----------------------------------------------------------------------------

struct QWaveConctx_s; typedef struct QWaveConctx_s* QWaveConctx; // use a simple version

                                    /* constructor: create a new instance of the qwave class */
__CAPE_LOCAL     QWaveConctx        qwave_conctx_new         (QWaveConfig config, QWaveResponse response, CapeQueue queue, QWaveAioctxEvent event, const CapeString remote_address);

//-----------------------------------------------------------------------------

__CAPE_LOCAL     void               qwave_conctx_close       (QWaveConctx);

__CAPE_LOCAL     void               qwave_conctx_reqdec      (QWaveConctx);

__CAPE_LOCAL     int                qwave_conctx_read        (QWaveConctx);

__CAPE_LOCAL     void               qwave_conctx_send        (QWaveConctx, CapeStream* p_output, int close_connection);

__CAPE_LOCAL     void               qwave_conctx_send_file   (QWaveConctx, const CapeString site, const CapeString path, int keep_alive);

//-----------------------------------------------------------------------------

#endif
