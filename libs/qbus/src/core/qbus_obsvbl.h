#ifndef __QBUS_OBSVBL__H
#define __QBUS_OBSVBL__H 1

#include "qbus_route.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusEmitter_s; typedef struct QBusEmitter_s* QBusEmitter;

typedef void     (__STDCALL     *fct_qbus_on_emit) (void* user_ptr, const CapeString subscriber_name, CapeUdc* p_data);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusObsvbl         qbus_obsvbl_new              (QBusEngines, QBusRoute);

__CAPE_LOCAL   void               qbus_obsvbl_del              (QBusObsvbl*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   CapeUdc            qbus_obsvbl_get              (QBusObsvbl, const CapeString module_name, const CapeString module_uuid);

__CAPE_LOCAL   void               qbus_obsvbl_set              (QBusObsvbl, const CapeString module_name, const CapeString module_uuid, CapeUdc observables);

__CAPE_LOCAL   void               qbus_obsvbl_add_nodes        (QBusObsvbl, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection, CapeUdc*);

__CAPE_LOCAL   QBusSubscriber     qbus_obsvbl_subscribe        (QBusObsvbl, const CapeString module_name, const CapeString value_name, void* user_ptr, fct_qbus_on_emit);

__CAPE_LOCAL   QBusSubscriber     qbus_obsvbl_subscribe_uuid   (QBusObsvbl, const CapeString uuid, const CapeString value_name, void* user_ptr, fct_qbus_on_emit);

__CAPE_LOCAL   void               qbus_obsvbl_emit             (QBusObsvbl, const CapeString value_name, CapeUdc* p_value);

__CAPE_LOCAL   void               qbus_obsvbl_value            (QBusObsvbl, const CapeString module_name, const CapeString value_name, CapeUdc* p_value);

__CAPE_LOCAL   void               qbus_obsvbl_dump             (QBusObsvbl);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_obsvbl_subloads         (QBusObsvbl, QBusPvdConnection);

//-----------------------------------------------------------------------------

#endif
