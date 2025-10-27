#ifndef __QWAVE_CONFIG__H
#define __QWAVE_CONFIG__H 1

// cape includes
#include "sys/cape_types.h"
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QWaveConfig_s; typedef struct QWaveConfig_s* QWaveConfig; // use a simple version

                                    /* constructor: create a new instance of the global qwave class */
__CAPE_LIBEX     QWaveConfig        qwave_config_new    ();

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qwave_config_del    (QWaveConfig*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     const CapeString   qwave_config_site   (QWaveConfig, const char *bufdat, size_t buflen, CapeString* url);

__CAPE_LIBEX     int                qwave_config_route  (QWaveConfig, const CapeString name);

//-----------------------------------------------------------------------------

#endif
