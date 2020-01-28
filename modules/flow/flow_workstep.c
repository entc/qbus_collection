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
  
  // local objects
  CapeUdc pdata = NULL;
  AdblTrx trx = NULL;
  number_t pdataid = 0;
  
  number_t fctid;
  const CapeString name;
  
  number_t prev = 0;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_add} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  fctid = cape_udc_get_n (qin->cdata, "fctid", 0);
  if (fctid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_add} missing parameter 'fctid'");
    goto exit_and_cleanup;
  }
  
  name = cape_udc_get_s (qin->cdata, "name", NULL);
  if (name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_add} missing parameter 'name'");
    goto exit_and_cleanup;
  }
  
  // optional
  pdata = cape_udc_ext (qin->cdata, "pdata");
  
  // optional
  prev = cape_udc_get_n (qin->cdata, "prev", 0);
  
  if (0 == prev)
  {
    // get laststepof this workflow
    // select * from proc_worksteps w1 left join proc_worksteps w2 on w1.id = w2.prev and w1.sqtid = w2.sqtid where w2.id is NULL;
  }
  
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  if (pdata)
  {
    pdataid = flow_data_add (trx, pdata, err); 
    if (0 == pdataid)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  // insert
  {
    // create a node for all values for insert
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n    (values, "wfid"     , self->wfid);
    cape_udc_add_n    (values, "sqtid"    , self->sqid);
    cape_udc_add_n    (values, "fctid"    , fctid);
    cape_udc_add_s_cp (values, "name"     , name);
    
    if (prev)
    {
      cape_udc_add_n    (values, "prev"   , prev);
    }
    
    if (pdataid)
    {
      cape_udc_add_n    (values, "p_data" , pdataid);
    }
    
    // insert into database, return value is the primary key
    self->wsid = adbl_trx_insert (trx, "proc_worksteps", &values, err);
    if (self->wsid == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  cape_udc_del (&pdata);

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
