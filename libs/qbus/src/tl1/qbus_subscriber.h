#ifndef __QBUS_MANIFOLD_SUBSCRIBER__H
#define __QBUS_MANIFOLD_SUBSCRIBER__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

// qbus includes
#include "qbus_tl1.h"
#include "qbus_member.h"

//-----------------------------------------------------------------------------

struct QBusManifoldSubscriber_s; typedef struct QBusManifoldSubscriber_s* QBusManifoldSubscriber;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusManifoldSubscriber  qbus_manifold_subscriber_new    (QBusManifoldMember member, const CapeString uuid, const CapeString name);

__CAPE_LIBEX   void                    qbus_manifold_subscriber_del    (QBusManifoldSubscriber*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                    qbus_manifold_subscriber_emit   (QBusManifoldSubscriber self, CapeUdc val, const CapeString module_ident, const CapeString module_name);

//-----------------------------------------------------------------------------

#endif

