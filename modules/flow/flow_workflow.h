#ifndef __FLOW__WORKFLOW__H
#define __FLOW__WORKFLOW__H 1

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct FlowWorkflow_s; typedef struct FlowWorkflow_s* FlowWorkflow;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   FlowWorkflow   flow_workflow_new         (QBus, AdblSession);

__CAPE_LIBEX   void           flow_workflow_del         (FlowWorkflow*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            flow_workflow_add         (FlowWorkflow*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_workflow_set         (FlowWorkflow*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_workflow_rm          (FlowWorkflow*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_workflow_get         (FlowWorkflow*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
