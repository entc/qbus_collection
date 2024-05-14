#ifndef __QBUS_CHAINS__H
#define __QBUS_CHAINS__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

// qbus includes
#include "qbus_method.h"

//-----------------------------------------------------------------------------

struct QBusChains_s; typedef struct QBusChains_s* QBusChains;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusChains         qbus_chains_new               ();

__CAPE_LIBEX   void               qbus_chains_del               (QBusChains*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_chains_push              (QBusChains, const CapeString method_ident, QBusMethod* p_method);

__CAPE_LIBEX   QBusMethod         qbus_chains_pop               (QBusChains, const CapeString method_ident);

//-----------------------------------------------------------------------------

#endif

