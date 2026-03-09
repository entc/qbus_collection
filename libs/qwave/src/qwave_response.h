#ifndef __QWAVE_RESPONSE__H
#define __QWAVE_RESPONSE__H 1

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <stc/cape_stream.h>

//-----------------------------------------------------------------------------

struct QWaveResponse_s; typedef struct QWaveResponse_s* QWaveResponse; // use a simple version

//-----------------------------------------------------------------------------

                                   /* constructor: create a new instance of the qwave class */
__CAPE_LIBEX     QWaveResponse     qwave_response_new         (const CapeString server_identifier, const CapeString provider);

                                   /* destructor */
__CAPE_LIBEX     void              qwave_response_del         (QWaveResponse*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL     void              qwave_response_file        (QWaveResponse, CapeStream, const CapeString path, int keep_alive);

__CAPE_LOCAL     void              qwave_response_upgrade     (QWaveResponse, CapeStream, const CapeString accept_key);

//-----------------------------------------------------------------------------

#endif


