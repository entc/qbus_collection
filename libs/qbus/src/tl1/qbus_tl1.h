#ifndef __QBUS_TL1__H
#define __QBUS_TL1__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusManifold_s; typedef struct QBusManifold_s* QBusManifold; // use a simple version

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusManifold       qbus_manifold_new               ();

__CAPE_LIBEX   void               qbus_manifold_del               (QBusManifold*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_manifold_emit              (QBusManifold);

__CAPE_LIBEX   void               qbus_manifold_subscribe         (QBusManifold);

__CAPE_LIBEX   void               qbus_manifold_response          (QBusManifold);

__CAPE_LIBEX   void               qbus_manifold_send              (QBusManifold);

//-----------------------------------------------------------------------------

#endif
