#ifndef __FLOW__RUN__H
#define __FLOW__RUN__H 1

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>
#include <sys/cape_queue.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct FlowRun_s; typedef struct FlowRun_s* FlowRun;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   FlowRun        flow_run_new             (QBus, AdblSession, CapeQueue);

__CAPE_LIBEX   void           flow_run_del             (FlowRun*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            flow_run_add             (FlowRun*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LOCAL   int            flow_run_get             (FlowRun*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
