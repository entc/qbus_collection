#ifndef __FLOW__CHAIN__H
#define __FLOW__CHAIN__H 1

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct FlowChain_s; typedef struct FlowChain_s* FlowChain;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   FlowChain      flow_chain_new           (QBus, AdblSession);

__CAPE_LIBEX   void           flow_chain_del           (FlowChain*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            flow_chain_get           (FlowChain*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_chain_data          (FlowChain*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LOCAL   int            flow_chain_get__fetch    (AdblTrx trx, number_t psid, CapeUdc logs, CapeErr err);

//-----------------------------------------------------------------------------

#endif
