#include "qbus.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <fmt/cape_json.h>

#include "flow_workflow.h"
#include "flow_workstep.h"
#include "flow_process.h"
#include "flow_chain.h"
#include "flow_run.h"
#include "flow_run_dbw.h"

#include <qjobs.h>
#include <qcrypt.h>

//-------------------------------------------------------------------------------------

struct FlowContext_s
{
  AdblCtx adbl_ctx;
  AdblSession adbl_session;

  CapeQueue queue;
  QJobs jobs;

  QBus qbus;     // reference

}; typedef struct FlowContext_s* FlowContext;

//-------------------------------------------------------------------------------------

FlowContext qbus_flow__ctx__new (QBus qbus)
{
  FlowContext self = CAPE_NEW (struct FlowContext_s);

  self->queue = cape_queue_new (300000);   // maximum of 5min

  self->adbl_ctx = NULL;
  self->adbl_session = NULL;

  self->qbus = qbus;

  return self;
}

//-------------------------------------------------------------------------------------

void qbus_flow__ctx__del (FlowContext* p_self)
{
  if (*p_self)
  {
    FlowContext self = *p_self;

    qjobs_del (&(self->jobs));

    adbl_session_close (&(self->adbl_session));
    adbl_ctx_del (&(self->adbl_ctx));

    cape_queue_del (&(self->queue));

    CAPE_DEL (p_self, struct FlowContext_s);
  }
}

//-------------------------------------------------------------------------------------

int __STDCALL qbus_flow__ctx__on_event (QJobs jobs, QJobsEvent event, void* user_ptr, CapeErr err)
{
  FlowContext self = user_ptr;

  // local objects
  CapeString h1 = NULL;
  CapeUdc rinfo = NULL;

  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "event", "received event trigger");

  h1 = qcrypt__decrypt ("flowsecret", event->rinfo_encrypted, err);
  if (h1 == NULL)
  {
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "on event", "can't run set: %s", cape_err_text (err));
    goto exit_and_cleanup;
  }

  rinfo = cape_json_from_s (h1);
  if (rinfo == NULL)
  {
    goto exit_and_cleanup;
  }

  // create a new run database wrapper
  FlowRunDbw flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->jobs, event->wpid, event->r1id, NULL, rinfo, 0);

  // forward business logic to this class
  if (flow_run_dbw_set (&flow_run_dbw, FLOW_ACTION__SET, &(event->params), err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "on event", "can't run set: %s", cape_err_text (err));
    goto exit_and_cleanup;
  }

exit_and_cleanup:

  cape_str_del (&h1);

  return CAPE_ERR_EOF;
}

//-------------------------------------------------------------------------------------

int qbus_flow__ctx__init  (FlowContext self, CapeAioContext aio_ctx, CapeErr err)
{
  self->adbl_ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
  if (self->adbl_ctx == NULL)
  {
    return cape_err_code (err);
  }

  self->adbl_session = adbl_session_open_file (self->adbl_ctx, "adbl_default.json", err);
  if (self->adbl_session == NULL)
  {
    return cape_err_code (err);
  }

  self->jobs = qjobs_new (self->adbl_session, "flow_jobs");

  if (qjobs_init (self->jobs, aio_ctx, 1000, self, qbus_flow__ctx__on_event, err))
  {
    return cape_err_code (err);
  }

  return cape_queue_start (self->queue, 1, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workflow__add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkflow flow_workflow = flow_workflow_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workflow_add (&flow_workflow, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workflow__set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkflow flow_workflow = flow_workflow_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workflow_set (&flow_workflow, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workflow__rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkflow flow_workflow = flow_workflow_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workflow_rm (&flow_workflow, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workflow__perm_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkflow flow_workflow = flow_workflow_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workflow_perm_get (&flow_workflow, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workflow__perm_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkflow flow_workflow = flow_workflow_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workflow_perm_set (&flow_workflow, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workflow__get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkflow flow_workflow = flow_workflow_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workflow_get (&flow_workflow, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workstep__add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkstep flow_workstep = flow_workstep_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workstep_add (&flow_workstep, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workstep__set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkstep flow_workstep = flow_workstep_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workstep_set (&flow_workstep, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workstep__rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkstep flow_workstep = flow_workstep_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workstep_rm (&flow_workstep, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workstep__mv (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkstep flow_workstep = flow_workstep_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workstep_mv (&flow_workstep, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__workstep__get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowWorkstep flow_workstep = flow_workstep_new (qbus, ctx->adbl_session);

  // run the command
  return flow_workstep_get (&flow_workstep, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_add (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_set (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_rm (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_get (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__details (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_details (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__once (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_once (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__next (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_next (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__prev (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_prev (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__step (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_step (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__instance_rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_instance_rm (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__wait_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_wait_get (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__stats (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_stats (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__process__all (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_process_all (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__run__add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowRun flow_run = flow_run_new (qbus, ctx->adbl_session, ctx->queue);

  // run the command
  return flow_run_add (&flow_run, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__run__get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowRun flow_run = flow_run_new (qbus, ctx->adbl_session, ctx->queue);

  // run the command
  return flow_run_get (&flow_run, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__chain__get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowChain flow_chain = flow_chain_new (qbus, ctx->adbl_session);

  // run the command
  return flow_chain_get (&flow_chain, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__chain__data (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowChain flow_chain = flow_chain_new (qbus, ctx->adbl_session);

  // run the command
  return flow_chain_data (&flow_chain, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__log__get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowChain flow_chain = flow_chain_new (qbus, ctx->adbl_session);

  // run the command
  return flow_chain_log (&flow_chain, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow__wspc__clr (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  FlowContext ctx = ptr;

  // create a temporary object
  FlowProcess flow_process = flow_process_new (qbus, ctx->adbl_session, ctx->queue, ctx->jobs);

  // run the command
  return flow_wspc_clr (&flow_process, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  FlowContext ctx = qbus_flow__ctx__new (qbus);

  // initialize the global context
  res = qbus_flow__ctx__init  (ctx, qbus_aio (qbus), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  *p_ptr = ctx;

  // -------- callback methods --------------------------------------------

  // add a new workflow
  //   args:
  qbus_register (qbus, "workflow_add"        , ctx, qbus_flow__workflow__add, NULL, err);

  // change a workflow
  //   args: wfid
  qbus_register (qbus, "workflow_set"        , ctx, qbus_flow__workflow__set, NULL, err);

  // get a workflow
  //   args: wfid
  qbus_register (qbus, "workflow_get"        , ctx, qbus_flow__workflow__get, NULL, err);

  // delete a workflow
  //   args: wfid
  qbus_register (qbus, "workflow_rm"         , ctx, qbus_flow__workflow__rm, NULL, err);

  // get the permissions for a workflow
  //   args: wfid
  qbus_register (qbus, "workflow_perm_get"   , ctx, qbus_flow__workflow__perm_get, NULL, err);

  // set the permissions for a workflow
  //   args: wfid, peid
  qbus_register (qbus, "workflow_perm_set"   , ctx, qbus_flow__workflow__perm_set, NULL, err);

  // -------- callback methods --------------------------------------------

  // add a new workflow
  //   args:
  qbus_register (qbus, "workstep_add"        , ctx, qbus_flow__workstep__add, NULL, err);

  // change a workflow
  //   args: wfid
  qbus_register (qbus, "workstep_set"        , ctx, qbus_flow__workstep__set, NULL, err);

  // get a workflow
  //   args: wfid
  qbus_register (qbus, "workstep_get"        , ctx, qbus_flow__workstep__get, NULL, err);

  // delete a workflow
  //   args: wfid
  qbus_register (qbus, "workstep_rm"         , ctx, qbus_flow__workstep__rm, NULL, err);

  // move a workstep
  //   args: wsid, direction
  qbus_register (qbus, "workstep_mv"         , ctx, qbus_flow__workstep__mv, NULL, err);

  // -------- callback methods --------------------------------------------

  // add a new process
  //   args: wfid, sqtid
  qbus_register (qbus, "process_add"         , ctx, qbus_flow__process__add, NULL, err);

  // change the current process step
  //   args: psid
  qbus_register (qbus, "process_set"         , ctx, qbus_flow__process__set, NULL, err);

  // stops the current process step and triggers the failover processing
  //   args: psid
  qbus_register (qbus, "process_rm"          , ctx, qbus_flow__process__rm, NULL, err);

  // get all active processes
  //   args:
  qbus_register (qbus, "process_all"         , ctx, qbus_flow__process__all, NULL, err);

  // get details of the current process
  //   args: psid
  qbus_register (qbus, "process_get"         , ctx, qbus_flow__process__get, NULL, err);

  // get analytic details of a process
  //   args: psid
  qbus_register (qbus, "process_details"     , ctx, qbus_flow__process__details, NULL, err);

  // run the current process step once
  //   args: psid
  qbus_register (qbus, "process_once"        , ctx, qbus_flow__process__once, NULL, err);

  // ignores the current state of a process
  // runs the next step
  //   args: psid
  qbus_register (qbus, "process_next"        , ctx, qbus_flow__process__next, NULL, err);

  // switch to the previous step
  //   args: psid
  qbus_register (qbus, "process_prev"        , ctx, qbus_flow__process__prev, NULL, err);

  // go backwards to a certain step
  //   args: psid, wsid
  qbus_register (qbus, "process_step"        , ctx, qbus_flow__process__step, NULL, err);

  // remove all entries for a process
  //   args: wpid, refid
  qbus_register (qbus, "process_instance_rm" , ctx, qbus_flow__process__instance_rm, NULL, err);

  // get the status of a waiting process
  //   args: psid, uuid, code
  qbus_register (qbus, "process_wait_get"    , ctx, qbus_flow__process__wait_get, NULL, err);

  // get stats about all processes by workspace
  //   args: wpid
  qbus_register (qbus, "process_stats"       , ctx, qbus_flow__process__stats, NULL, err);

  // -------- callback methods --------------------------------------------

  // starts a new progress run instead of calling a module directly
  //   args: module, method, params
  qbus_register (qbus, "run_add"             , ctx, qbus_flow__run__add, NULL, err);

  // get the current state of a run
  //   args: raid
  qbus_register (qbus, "run_get"             , ctx, qbus_flow__run__get, NULL, err);

  // -------- callback methods --------------------------------------------

  // get logs of the process chain
  //   args:
  qbus_register (qbus, "chain_get"           , ctx, qbus_flow__chain__get, NULL, err);

  // get data of the process chain
  //   args: psid
  qbus_register (qbus, "chain_data"          , ctx, qbus_flow__chain__data, NULL, err);

  // -------- callback methods --------------------------------------------

  // get the logs related to a refid
  // -> this is using the internal instance table
  //   args: refid
  qbus_register (qbus, "log_get"             , ctx, qbus_flow__log__get, NULL, err);

  // -------- callback methods --------------------------------------------

  // delete everything for a workspace
  //   args: refid
  qbus_register (qbus, "wspc_clr"            , ctx, qbus_flow__wspc__clr, NULL, err);

  // -------- callback methods --------------------------------------------

  return CAPE_ERR_NONE;

exit_and_cleanup:

  cape_log_msg (CAPE_LL_ERROR, "FLOW", "init", cape_err_text(err));

  qbus_flow__ctx__del (&ctx);
  *p_ptr = NULL;

  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_flow_done (QBus qbus, void* ptr, CapeErr err)
{
  FlowContext ctx = ptr;

  qbus_flow__ctx__del (&ctx);

  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("FLOW", NULL, qbus_flow_init, qbus_flow_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
