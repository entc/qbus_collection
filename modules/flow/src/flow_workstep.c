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

int flow_workstep__intern__last_prev (FlowWorkstep self, number_t* p_prev, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (params, "wfid"        , self->wfid);    
    cape_udc_add_n     (params, "sqtid"       , self->sqid);
    cape_udc_add_n     (values, "wsid"        , 0);
    
    // execute the query
    // select w1.wfid, w1.sqtid, w1.id wsid from proc_worksteps w1 left join proc_worksteps w2 on w1.id = w2.prev and w1.sqtid = w2.sqtid where w2.id is NULL;
    query_results = adbl_session_query (self->adbl_session, "flow_last_prev_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row)
  {
    *p_prev = cape_udc_get_n (first_row, "wsid", 0);
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep__intern__get (FlowWorkstep self, number_t* p_prev, number_t* p_dataid, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row;
  
  // flow_worksteps_prev_succ_view
  // select ws.id, ws.wfid, ws.sqtid, ws.prev ws_prev, w2.id ws_succ, w4.prev pr_prev, w3.id sc_succ from proc_worksteps ws left join proc_worksteps w2 on w2.prev = ws.id left join proc_worksteps w3 on w3.prev = w2.id left join proc_worksteps w4 on w4.id = ws.prev;
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
  
  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "error in fetching prev and succ of the workstep");
    goto exit_and_cleanup;
  }
  
  // get the values
  *p_prev = cape_udc_get_n (first_row, "prev", 0);
  *p_dataid = cape_udc_get_n (first_row, "p_data", 0);
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);  
  return res;  
}

//-----------------------------------------------------------------------------

int flow_workstep__intern__fetch_prev_succ (FlowWorkstep self, number_t* p_prev, number_t* p_succ, number_t* p_pr_prev, number_t* p_sc_succ, CapeErr err)
{
  int res;
  
  // return values
  number_t prev = 0;
  number_t succ = 0;
  number_t pr_prev = 0;
  number_t sc_succ = 0;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row;
  
  // flow_worksteps_prev_succ_view
  // select ws.id, ws.wfid, ws.sqtid, ws.prev ws_prev, w2.id ws_succ, w4.prev pr_prev, w3.id sc_succ from proc_worksteps ws left join proc_worksteps w2 on w2.prev = ws.id left join proc_worksteps w3 on w3.prev = w2.id left join proc_worksteps w4 on w4.id = ws.prev;
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"            , self->wsid);
    cape_udc_add_n      (params, "wfid"          , self->wfid);
    cape_udc_add_n      (params, "sqtid"         , self->sqid);
    
    cape_udc_add_n      (values, "ws_prev"       , 0);
    cape_udc_add_n      (values, "ws_succ"       , 0);
    cape_udc_add_n      (values, "pr_prev"       , 0);
    cape_udc_add_n      (values, "sc_succ"       , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_worksteps_prev_succ_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "error in fetching prev and succ of the workstep");
    goto exit_and_cleanup;
  }
  
  // get the values
  prev = cape_udc_get_n (first_row, "ws_prev", 0);
  succ = cape_udc_get_n (first_row, "ws_succ", 0);

  // get the values
  pr_prev = cape_udc_get_n (first_row, "pr_prev", 0);
  sc_succ = cape_udc_get_n (first_row, "sc_succ", 0);
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  // set the values
  *p_prev = prev;
  *p_succ = succ;
  *p_pr_prev = pr_prev;
  *p_sc_succ = sc_succ;

  cape_udc_del (&query_results);  
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep__intern__update_prev (AdblTrx trx, number_t wsid, number_t prev, CapeErr err)
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

int flow_workstep__intern__swap (AdblTrx trx, number_t pr_prev, number_t ws_prev, number_t ws, number_t ws_succ, CapeErr err)
{
  int res;
  
  // update ws
  res = flow_workstep__intern__update_prev (trx, ws, pr_prev, err);
  if (res)
  {
    goto exit_and_cleanup;
  } 
  
  // update prev
  res = flow_workstep__intern__update_prev (trx, ws_prev, ws, err);
  if (res)
  {
    goto exit_and_cleanup;
  } 
  
  // update succ
  if (ws_succ)
  {
    res = flow_workstep__intern__update_prev (trx, ws_succ, ws_prev, err);
    if (res)
    {
      goto exit_and_cleanup;
    } 
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
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
  const CapeString tag;

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
  tag = cape_udc_get_s (qin->cdata, "tag", NULL);

  // optional
  pdata = cape_udc_ext (qin->cdata, "pdata");
  
  // optional
  prev = cape_udc_get_n (qin->cdata, "prev", 0);
  
  if (0 == prev)
  {
    // get laststepof this workflow
    res = flow_workstep__intern__last_prev (self, &prev, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
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
    
    if (tag)
    {
      cape_udc_add_s_cp (values, "tag"    , tag);
    }
    
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
  
  const CapeString name;
  const CapeString tag;
  number_t fctid;
  
  // local objects
  AdblTrx trx = NULL;
  CapeUdc pdata = NULL;
  
  number_t prev = 0;
  number_t pdataid = 0;
  
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
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_rm} missing parameter 'wsid'");
    goto exit_and_cleanup;
  }
  
  // values name
  name = cape_udc_get_s (qin->cdata, "name", NULL);

  // values name
  tag = cape_udc_get_s (qin->cdata, "tag", NULL);

  // values name
  fctid = cape_udc_get_n (qin->cdata, "fctid", 0);
  
  // extract p_data values
  pdata = cape_udc_ext (qin->cdata, "pdata");
  
  res = flow_workstep__intern__get (self, &prev, &pdataid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  if (pdata)
  {
    if (pdataid)
    {
      res = flow_data_set (trx, pdataid, pdata, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
    else
    {
      pdataid = flow_data_add (trx, pdata, err);
      if (0 == pdataid)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    if (pdataid)
    {
      res = flow_data_rm (trx, pdataid, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      pdataid = 0;
    }
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"            , self->wsid);
    
    if (name)
    {
      cape_udc_add_s_cp (values, "name", name);
    }

    if (tag)
    {
      cape_udc_add_s_cp (values, "tag", tag);
    }

    if (fctid)
    {
      cape_udc_add_n (values, "fctid", fctid);
    }
    
    if (pdataid)
    {
      cape_udc_add_n (values, "p_data", pdataid);
    }
    else
    {
      cape_udc_add_z (values, "p_data");
    }
        
    // execute the query
    res = adbl_trx_update (trx, "proc_worksteps", &params, &values, err);
    if (res)
    {
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
    
    cape_udc_add_n      (values, "id"            , 0);
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
    res = flow_workstep__intern__update_prev (trx, cape_udc_get_n (step_successor, "id", 0), prev, err);
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

int flow_workstep_mv (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  number_t direction;
  
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
  
  direction = cape_udc_get_n (qin->cdata, "direction", 0);
  
  switch (direction)
  {
    case -1:
    {
      number_t ws_prev;
      number_t ws_succ;
      number_t pr_prev;
      number_t sc_succ;
      
      res = flow_workstep__intern__fetch_prev_succ (self, &ws_prev, &ws_succ, &pr_prev, &sc_succ, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      if (ws_prev)
      {
        trx = adbl_trx_new (self->adbl_session, err);
        if (NULL == trx)
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
        
        res = flow_workstep__intern__swap (trx, pr_prev, ws_prev, self->wsid, ws_succ, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
      }
      
      break;
    }
    case +1:
    {
      number_t ws_prev;
      number_t ws_succ;
      number_t pr_prev;
      number_t sc_succ;
      
      res = flow_workstep__intern__fetch_prev_succ (self, &ws_prev, &ws_succ, &pr_prev, &sc_succ, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      if (ws_succ)
      {
        trx = adbl_trx_new (self->adbl_session, err);
        if (NULL == trx)
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
        
        res = flow_workstep__intern__swap (trx, ws_prev, self->wsid, ws_succ, sc_succ, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
      }
      
      break;
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
