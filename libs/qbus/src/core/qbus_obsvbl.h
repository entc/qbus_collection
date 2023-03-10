#ifndef __QBUS_OBSVBL__H
#define __QBUS_OBSVBL__H 1

#include "qbus_route.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusSubscriber_s; typedef struct QBusSubscriber_s* QBusSubscriber;
struct QBusObsvbl_s; typedef struct QBusObsvbl_s* QBusObsvbl;
struct QBusEmitter_s; typedef struct QBusEmitter_s* QBusEmitter;

typedef int      (__STDCALL     *fct_qbus_on_emit) (QBusSubscriber, void* user_ptr, CapeUdc data, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusObsvbl         qbus_obsvbl_new              (QBusEngines, QBusRoute);

__CAPE_LOCAL   void               qbus_obsvbl_del              (QBusObsvbl*);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_obsvbl_add_nodes        (QBusObsvbl, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection, CapeUdc*);

__CAPE_LOCAL   void               qbus_obsvbl_send_update      (QBusObsvbl, QBusPvdConnection not_in_list, QBusPvdConnection single_trans);

__CAPE_LOCAL   QBusSubscriber     qbus_obsvbl_subscribe        (QBusObsvbl, const CapeString module_name, const CapeString value_name, void* user_ptr, fct_qbus_on_emit);

__CAPE_LOCAL   void               qbus_obsvbl_emit             (QBusObsvbl, const CapeString value_name, CapeUdc* p_value);

__CAPE_LOCAL   void               qbus_obsvbl_value            (QBusObsvbl, const CapeString value_name, CapeUdc* p_value);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_obsvbl_subloads         (QBusObsvbl);

//-----------------------------------------------------------------------------

#endif
