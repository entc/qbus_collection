#ifndef __FLOW__RUN_STEP__H
#define __FLOW__RUN_STEP__H 1

#include "flow_run_dbw.h"

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

//-----------------------------------------------------------------------------

#define FLOW_STATE__NONE              0
#define FLOW_STATE__ERROR             1
#define FLOW_STATE__HALT              2
#define FLOW_STATE__DONE              3
#define FLOW_STATE__ABORTED           4
#define FLOW_STATE__INIT              5
#define FLOW_STATE__SET               6
#define FLOW_STATE__GET               7

//-----------------------------------------------------------------------------

struct FlowRunStep_s; typedef struct FlowRunStep_s* FlowRunStep;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   FlowRunStep    flow_run_step_new         (QBus);

__CAPE_LIBEX   int            flow_run_step_set         (FlowRunStep*, FlowRunDbw*, number_t action, CapeUdc params, CapeErr err);

//-----------------------------------------------------------------------------

#endif
