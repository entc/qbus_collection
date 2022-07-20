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
    self->wfid = adbl_trx_insert (adbl_trx, "proc_workflows", &values, err);
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
    res = adbl_trx_update (adbl_trx, "proc_workflows", &params, &values, err);
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
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workflow_rm} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

void flow_workflow__intern__order (FlowWorkflow self, CapeUdc cdata, CapeUdc results)
{
  number_t prev = 0;
  int run = TRUE;
  
  while (run)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (results, CAPE_DIRECTION_FORW);
    
    // don't repeat again, if we won't find a match
    run = FALSE;
    
    while (cape_udc_cursor_next (cursor))
    {
      number_t item_prev = cape_udc_get_n (cursor->item, "prev", 0);
      
      if (item_prev == prev)
      {
        CapeUdc item = cape_udc_cursor_ext (results, cursor);        

        // set new prev
        prev = cape_udc_get_n (item, "id", 0);

        cape_udc_add (cdata, &item);
        
        run = TRUE;
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
}

//-----------------------------------------------------------------------------

int flow_workflow_get (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  
  res = flow_workflow__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid)
  {
    number_t sqtid = cape_udc_get_n (qin->cdata, "sqtid", 0);
    if (sqtid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_SQTID");
      goto exit_and_cleanup;
    }

    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (params, "wfid"        , self->wfid);
    cape_udc_add_n     (params, "sqtid"       , sqtid);

    cape_udc_add_n     (values, "id"          , 0);
    cape_udc_add_n     (values, "sqtid"       , 0);
    cape_udc_add_s_cp  (values, "name"        , NULL);
    cape_udc_add_s_cp  (values, "tag"         , NULL);
    cape_udc_add_n     (values, "prev"        , 0);
    cape_udc_add_n     (values, "fctid"       , 0);
    cape_udc_add_n     (values, "usrid"       , 0);
    cape_udc_add_node  (values, "pdata"       );
    
    // execute the query
    /*
     flow_proc_worksteps_view
     
     select `ws`.`id` AS `id`,`ws`.`wfid` AS `wfid`,`ws`.`sqtid` AS `sqtid`,`ws`.`name` AS `name`, ws.tag tag,`ws`.`prev` AS `prev`,`ws`.`fctid` AS `fctid`, ws.usrid,`pd`.`content` AS `pdata` from (`proc_worksteps` `ws` left join `proc_data` `pd` on(`pd`.`id` = `ws`.`p_data`))
     */
    query_results = adbl_session_query (self->adbl_session, "flow_proc_worksteps_view", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
    
    if (cape_udc_get_b (qin->cdata, "ordered", FALSE))
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_LIST, NULL);
      
      flow_workflow__intern__order (self, h, query_results);
      
      cape_udc_replace_mv (&(qout->cdata), &h);
    }
    else
    {
      cape_udc_replace_mv (&(qout->cdata), &query_results);
    }
    
    res = CAPE_ERR_NONE;
  }
  else
  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (values, "id"          , 0);
    cape_udc_add_s_cp  (values, "name"        , NULL);
    cape_udc_add_list  (values, "steps"       );
    
    // execute the query
    
    // flow_workflows_sqt_view
    // select wf.id, wf.name, sqtid, count(ws.id) steps from proc_workflows wf left join proc_worksteps ws on ws.wfid = wf.id group by wf.id, ws.sqtid

    // flow_workflows_view
    // select id, name, concat('[', group_concat(concat('{', '"sqtid":', sqtid, ',"steps":', steps, '}')), ']') steps from flow_workflows_sqt_view group by id;
    
    query_results = adbl_session_query (self->adbl_session, "flow_workflows_view", NULL, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }

    res = CAPE_ERR_NONE;
    cape_udc_replace_mv (&(qout->cdata), &query_results);
  }
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workflow_perm_get (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;

  // local objects
  CapeUdc query_results = NULL;

  // check roles
  if (qbus_message_role_has (qin, "admin") == FALSE)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NOROLE");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NOCDATA");
    goto exit_and_cleanup;
  }

  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_WFID");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (params, "wfid"        , self->wfid);
    
    cape_udc_add_n     (values, "id"          , 0);
    cape_udc_add_n     (values, "wpid"        , 0);
    cape_udc_add_n     (values, "gpid"        , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_workspaces", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workflow_perm_set (FlowWorkflow* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkflow self = *p_self;

  int perm;
  CapeUdc perm_node;
  CapeUdc first_row;
  
  // local objects
  AdblTrx trx = NULL;
  CapeUdc query_results = NULL;

  // check roles
  if (qbus_message_role_has (qin, "admin") == FALSE)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NOROLE");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NOCDATA");
    goto exit_and_cleanup;
  }

  self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_WPID");
    goto exit_and_cleanup;
  }

  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_WFID");
    goto exit_and_cleanup;
  }

  perm_node = cape_udc_get (qin->cdata, "perm");
  if (perm_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_PERM");
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (params, "wfid"        , self->wfid);
    cape_udc_add_n     (params, "wpid"        , self->wpid);
    cape_udc_add_n     (values, "id"          , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_workspaces", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  perm = cape_udc_b (perm_node, FALSE);
  
  // start the transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  if (first_row)
  {
    if (perm)
    {
      // already exists
      res = cape_err_set (err, CAPE_ERR_EOF, "ERR.ALREADY_EXISTS");
      goto exit_and_cleanup;
    }
    else
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n (params, "id", cape_udc_get_n (first_row, "id", 0));
      
      res = adbl_trx_delete (trx, "flow_workspaces", &params, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    if (perm)
    {
      int id;
      
      CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n (values, "wfid", self->wfid);
      cape_udc_add_n (values, "wpid", self->wpid);
      
      // execute query
      id = adbl_trx_insert (trx, "flow_workspaces", &values, err);
      if (id == 0)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
    else
    {
      // already exists
      res = cape_err_set (err, CAPE_ERR_EOF, "ERR.ALREADY_REMOVED");
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);

  flow_workflow_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
