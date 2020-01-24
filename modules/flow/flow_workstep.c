#include "flow_workstep.h"
#include "flow_run_dbw.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct FlowWorkstep_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  
  number_t wfid;               // workflow id
  number_t wsid;               // workstep id
  number_t sqid;               // sequence id
};

//-----------------------------------------------------------------------------

FlowWorkstep flow_workstep_new (QBus qbus, AdblSession adbl_session)
{
  FlowWorkstep self = CAPE_NEW(struct FlowWorkstep_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  
  self->wfid = 0;
  self->wsid = 0;
  self->sqid = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void flow_workstep_del (FlowWorkstep* p_self)
{
  if (*p_self)
  {
    FlowWorkstep self = *p_self;
    
    
    CAPE_DEL (p_self, struct FlowWorkstep_s);
  }
}

//-----------------------------------------------------------------------------

int flow_workstep__intern__qin_check (FlowWorkstep self, QBusM qin, CapeErr err)
{
  // do some security checks
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
  }
  
  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_check} missing cdata");
  }
  
  self->sqid = cape_udc_get_n (qin->cdata, "sqid", 0);
  if (self->sqid == 0)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing sqid");
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_workstep_add (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
  exit_and_cleanup:
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep_set (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep__intern__update (FlowWorkstep self, AdblTrx trx, number_t wsid, number_t prev, CapeErr err)
{
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_n      (params, "id"            , wsid);

  if (prev)
  {
    cape_udc_add_n      (values, "prev"          , prev);
  }
  else
  {
    cape_udc_add_z      (values, "prev");
  }
  
  // execute the query
  return adbl_trx_update (trx, "proc_worksteps", &params, &values, err);
}

//-----------------------------------------------------------------------------

int flow_workstep_rm (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  CapeUdc step_to_delete;
  CapeUdc step_successor;
  
  // local objects
  CapeUdc query_results = NULL;
  AdblTrx trx = NULL;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_rm} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  self->wsid = cape_udc_get_n (qin->cdata, "wsid", 0);
  if (self->wsid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_rm} missing parameter 'wsid'");
    goto exit_and_cleanup;
  }
  
  // fetch step to delete
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"            , self->wsid);
    cape_udc_add_n      (params, "wfid"          , self->wfid);
    cape_udc_add_n      (params, "sqtid"         , self->sqid);
    
    cape_udc_add_n      (values, "prev"          , 0);
    cape_udc_add_n      (values, "p_data"        , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_worksteps", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  step_to_delete = cape_udc_get_first (query_results);
  if (NULL == step_to_delete)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "{flow_workstep_rm} workstep not found");
    goto exit_and_cleanup;
  }
  
  number_t prev = cape_udc_get_n (step_to_delete, "prev", 0);
  number_t pdata_id = cape_udc_get_n (step_to_delete, "p_data", 0);
  
  // cleanup result
  cape_udc_del (&query_results);
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "prev"          , self->wsid);
    cape_udc_add_n      (params, "wfid"          , self->wfid);
    cape_udc_add_n      (params, "sqtid"         , self->sqid);
    
    cape_udc_add_n      (params, "id"            , 0);
    cape_udc_add_n      (values, "p_data"        , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_worksteps", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // this might be empty
  step_successor = cape_udc_get_first (query_results);
  
  if (step_successor)
  {
    res = flow_workstep__intern__update (self, trx, cape_udc_get_n (step_successor, "id", 0), prev, err);
    if (res)
    {
      goto exit_and_cleanup;
    }    
  }
  
  // delete pdata
  if (pdata_id)
  {
    res = flow_data_rm (trx, pdata_id, err);
    if (res)
    {
      goto exit_and_cleanup;
    }    
  }
  
  // delete 
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"           , self->wsid);
    
    res = adbl_trx_delete (trx, "proc_worksteps", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }    
  }
  
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  cape_udc_del (&query_results);
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep_get (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  self->wsid = cape_udc_get_n (qin->cdata, "wsid", 0);
  if (self->wsid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_set} missing parameter 'wsid'");
    goto exit_and_cleanup;
  }
  
  
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
