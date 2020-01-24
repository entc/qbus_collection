#include "flow_workflow.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct FlowWorkflow_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  
  number_t wpid;               // workspace id
  number_t wfid;               // workflow id
};

//-----------------------------------------------------------------------------

FlowWorkflow flow_workflow_new (QBus qbus, AdblSession adbl_session)
{
  FlowWorkflow self = CAPE_NEW(struct FlowWorkflow_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  
  self->wpid = 0;
  self->wfid = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void flow_workflow_del (FlowWorkflow* p_self)
{
  if (*p_self)
  {
    FlowWorkflow self = *p_self;
    
    
    CAPE_DEL (p_self, struct FlowWorkflow_s);
  }
}

//-----------------------------------------------------------------------------

int flow_workflow__intern__qin_check (FlowWorkflow self, QBusM qin, CapeErr err)
{
  // do some security checks
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
  }
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "missing wpid");
  }
  
  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_check} missing cdata");
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_workflow_add (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;
  
  // const local objects
  const CapeString workflow_name = NULL;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  
  res = flow_workflow__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // get the workflow name from the input parameters
  workflow_name = cape_udc_get_s (qin->cdata, "name", NULL);
  if (workflow_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_add} missing name");
    goto exit_and_cleanup;
  }

  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    // create a node for all values for insert
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp (values, "name", workflow_name);
    
    // insert into database, return value is the primary key
    self->wfid = adbl_trx_insert (adbl_trx, "flow_workflows", &values, err);
    if (self->wfid == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  // generate the output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qout->cdata, "wfid", self->wfid);
  
  // finally commit
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workflow_set (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;
  
  // const local objects
  const CapeString workflow_name = NULL;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  
  res = flow_workflow__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  // get the workflow name from the input parameters
  workflow_name = cape_udc_get_s (qin->cdata, "name", NULL);
  if (workflow_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_add} missing name");
    goto exit_and_cleanup;
  }
  
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n    (params, "id", self->wfid);
    cape_udc_add_s_cp (values, "name", workflow_name);
    
    // insert into database, return value is the primary key
    res = adbl_trx_update (adbl_trx, "flow_workflows", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  // finally commit
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workflow_rm (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;
  
  res = flow_workflow__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workflow_get (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;
  
  res = flow_workflow__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
