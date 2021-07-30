#ifndef __QBUS__ROUTE_METHOD__H
#define __QBUS__ROUTE_METHOD__H 1

#include "qbus.h"
#include "qbus_frame.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//-----------------------------------------------------------------------------

#define QBUS_METHOD_TYPE__REQUEST      1
#define QBUS_METHOD_TYPE__RESPONSE     2
#define QBUS_METHOD_TYPE__FORWARD      3
#define QBUS_METHOD_TYPE__METHODS      4

//=============================================================================

struct QBusMethod_s; typedef struct QBusMethod_s* QBusMethod;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusMethod        qbus_method_new              (int type, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm);

__CAPE_LIBEX   void              qbus_method_del              (QBusMethod*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   number_t          qbus_method_type             (QBusMethod);

__CAPE_LIBEX   void*             qbus_method_ptr              (QBusMethod);

__CAPE_LIBEX   void              qbus_method_continue         (QBusMethod, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo);

__CAPE_LIBEX   void              qbus_method_handle_request   (QBusMethod, QBus qbus, QBusRoute route, QBusConnection conn, const CapeString ident, QBusFrame* p_frame);

__CAPE_LIBEX   void              qbus_method_handle_response  (QBusMethod, QBus qbus, QBusRoute route, QBusFrame* p_frame);

__CAPE_LIBEX   int               qbus_method_local            (QBusMethod, QBus qbus, QBusRoute route, QBusM msg, QBusM qout, const CapeString method, CapeErr err);

__CAPE_LIBEX   int               qbus_method_call_response    (QBusMethod, QBus qbus, QBusRoute route, QBusM qin, CapeErr err);

//-----------------------------------------------------------------------------

#endif
