#ifndef __FLOW__TASK__H
#define __FLOW__TASK__H 1

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>
#include <sys/cape_queue.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

// QJobs includes
#include <qjobs.h>

//-----------------------------------------------------------------------------

#define FLOW_CDATA__PSID              "psid"
#define FLOW_CDATA__SYNCID            "_syncid"
#define FLOW_CDATA__REFID             "_refid"
#define FLOW_CDATA__PARENT            "_parent"

#define FLOW_SQTID__DEFAULT           1    // default workflow sequence
#define FLOW_SQTID__ABORT             2    // abort sequence
#define FLOW_SQTID__FAILOVER          3    // in case of failure
#define FLOW_SQTID__DELETE            4    // delete the sequence

//-----------------------------------------------------------------------------

struct FlowProcess_s; typedef struct FlowProcess_s* FlowProcess;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   FlowProcess    flow_process_new         (QBus, AdblSession, CapeQueue, QJobs);

__CAPE_LIBEX   void           flow_process_del         (FlowProcess*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            flow_process_add         (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_set         (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_rm          (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_get         (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_all         (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_details     (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_once        (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_next        (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_prev        (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_step        (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_instance_rm (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int            flow_process_wait_get    (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            flow_wspc_clr            (FlowProcess*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
