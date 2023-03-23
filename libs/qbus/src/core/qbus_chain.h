#ifndef __QBUS__CHAIN__H
#define __QBUS__CHAIN__H 1

#include "qbus_message.h"
#include "qbus_methods.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusChain_s; typedef struct QBusChain_s* QBusChain;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusChain          qbus_chain_new               ();

__CAPE_LOCAL   void               qbus_chain_del               (QBusChain*);

__CAPE_LOCAL   void               qbus_chain_add               (QBusChain, void* ptr, fct_qbus_onMessage onMsg, CapeString* p_last_chainkey, CapeString* p_next_chainkey, CapeString* p_last_sender, CapeUdc* p_rinfo);

__CAPE_LOCAL   QBusMethodItem     qbus_chain_ext               (QBusChain, const CapeString chainkey);

//-----------------------------------------------------------------------------

#endif
