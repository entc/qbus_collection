#ifndef __QBUS__CHAIN__H
#define __QBUS__CHAIN__H 1

#include "qbus_message.h"
#include "qbus_methods.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusMethodItem_s; typedef struct QBusMethodItem_s* QBusMethodItem;

//-----------------------------------------------------------------------------

struct QBusChain_s; typedef struct QBusChain_s* QBusChain;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusChain          qbus_chain_new               ();

__CAPE_LOCAL   void               qbus_chain_del               (QBusChain*);

__CAPE_LOCAL   void               qbus_chain_add__cp           (QBusChain, const CapeString chainkey, QBusMethodItem* p_mitem);

__CAPE_LOCAL   void               qbus_chain_add__mv           (QBusChain, CapeString* p_chainkey, QBusMethodItem* p_mitem);

__CAPE_LOCAL   QBusMethodItem     qbus_chain_ext               (QBusChain, const CapeString chainkey);

//-----------------------------------------------------------------------------

typedef void (__STDCALL *fct_qbus_chain__on_item) (void* user_ptr, QBusMethodItem* p_mitem);

__CAPE_LIBEX   void               qbus_chain_all               (QBusChain, void* user_ptr, fct_qbus_chain__on_item);

//-----------------------------------------------------------------------------

#endif
