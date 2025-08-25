
#ifndef __HEALTHCHECK__H
#define __HEALTHCHECK__H 1

#include "sys/cape_export.h"

//-----------------------------------------------------------------------------

struct Healthcheck_s; typedef struct Healthcheck_s* Healthcheck;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   Healthcheck          healthcheck_new          ();

__CAPE_LIBEX   void                 healthcheck_del          (Healthcheck*);

//-----------------------------------------------------------------------------

#endif
