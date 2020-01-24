#ifndef __FLOW__WORKSTEP__H
#define __FLOW__WORKSTEP__H 1

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct FlowWorkstep_s; typedef struct FlowWorkstep_s* FlowWorkstep;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   FlowWorkstep   flow_workstep_new         (QBus, AdblSession);

__CAPE_LIBEX   void           flow_workstep_del         (FlowWorkstep*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            flow_workstep_add         (FlowWorkstep*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_workstep_set         (FlowWorkstep*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_workstep_rm          (FlowWorkstep*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_workstep_get         (FlowWorkstep*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
