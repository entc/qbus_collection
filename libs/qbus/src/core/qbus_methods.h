#ifndef __QBUS__METHODS__H
#define __QBUS__METHODS__H 1

#include "qbus_route.h"
#include "qbus_message.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

#define QBUS_METHOD_TYPE__REQUEST      1
#define QBUS_METHOD_TYPE__RESPONSE     2
#define QBUS_METHOD_TYPE__FORWARD      3
#define QBUS_METHOD_TYPE__METHODS      4

//-----------------------------------------------------------------------------

struct QBusMethodItem_s; typedef struct QBusMethodItem_s* QBusMethodItem;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusMethodItem     qbus_methods_item_new               (number_t type, const CapeString method, void* user_ptr, fct_qbus_onMessage user_fct);

__CAPE_LOCAL   void               qbus_methods_item_del               (QBusMethodItem*);

__CAPE_LOCAL   number_t           qbus_methods_item_type              (QBusMethodItem);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int                qbus_methods_item__call_response    (QBusMethodItem, QBus qbus, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LOCAL   int                qbus_methods_item__call_request     (QBusMethodItem, QBus qbus, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_methods_item_continue          (QBusMethodItem, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo);

//-----------------------------------------------------------------------------

struct QBusMethods_s; typedef struct QBusMethods_s* QBusMethods;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusMethods        qbus_methods_new                    ();

__CAPE_LOCAL   void               qbus_methods_del                    (QBusMethods*);

__CAPE_LOCAL   int                qbus_methods_add                    (QBusMethods, const CapeString method, void* user_ptr, fct_qbus_onMessage on_event, CapeErr);

__CAPE_LOCAL   QBusMethodItem     qbus_methods_get                    (QBusMethods, const CapeString method);

//-----------------------------------------------------------------------------

#endif
