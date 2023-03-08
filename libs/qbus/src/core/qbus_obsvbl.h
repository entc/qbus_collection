#ifndef __QBUS_OBSVBL__H
#define __QBUS_OBSVBL__H 1

#include "qbus_route.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusObsvbl_s; typedef struct QBusObsvbl_s* QBusObsvbl;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusObsvbl         qbus_obsvbl_new              (QBusEngines);

__CAPE_LOCAL   void               qbus_obsvbl_del              (QBusObsvbl*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_obsvbl_add_nodes        (QBusObsvbl, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection, CapeUdc*);

__CAPE_LOCAL   void               qbus_obsvbl_send_update      (QBusObsvbl, QBusPvdConnection not_in_list, QBusPvdConnection single_trans);

//-----------------------------------------------------------------------------

#endif
