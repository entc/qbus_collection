#include "flow_process.h"
#include "flow_run_dbw.h"
#include "flow_run_step.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct FlowProcess_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  CapeQueue queue;             // reference
  
  number_t wpid;               // workspace id
  number_t gpid;               // global person id
  number_t wfid;               // workflow id
  number_t sqid;               // sequence id
  number_t psid;               // process id
  number_t fiid;               // flow instance ID

  number_t refid;              // reference id
  number_t syncid;             // process sync id
  
  number_t tdata_id;           // container to store all variables of the flow process
  
  CapeUdc tmp_data;
};

//-----------------------------------------------------------------------------

FlowProcess flow_process_new (QBus qbus, AdblSession adbl_session, CapeQueue queue)
{
  FlowProcess self = CAPE_NEW(struct FlowProcess_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->queue = queue;
  
  self->wpid = 0;
  self->gpid = 0;
  self->wfid = 0;
  self->psid = 0;
  self->sqid = 0;
  self->fiid = 0;
  
  self->refid = 0;
  self->syncid = 0;
  
  self->tdata_id = 0;
  self->tmp_data = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void flow_process_del (FlowProcess* p_self)
{
  if (*p_self)
  {
    FlowProcess self = *p_self;
    
    cape_udc_del (&(self->tmp_data));
    
    CAPE_DEL (p_self, struct FlowProcess_s);
  }
}

//-----------------------------------------------------------------------------

int flow_process__intern__qin_check (FlowProcess self, QBusM qin, CapeErr err)
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
  
  self->gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (self->gpid == 0)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
  }
  
  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_process_add__instance (FlowProcess self, AdblTrx trx, CapeErr err)
{
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_n    (values, "wpid"         , self->wpid);
  cape_udc_add_n    (values, "gpid"         , self->gpid);
  cape_udc_add_n    (values, "wfid"         , self->wfid);
  cape_udc_add_n    (values, "refid"        , self->refid);
  cape_udc_add_n    (values, "psid"         , self->psid);

  // execute the query
  self->fiid = adbl_trx_insert (trx, "flow_instance", &values, err);
  
  if (self->fiid)
  {
    return CAPE_ERR_NONE;
  }
  
  cape_err_clr (err);
  return cape_err_set (err, CAPE_ERR_RUNTIME, "instance was already created");
}

//-----------------------------------------------------------------------------

int flow_process_add__init (FlowProcess self, FlowRunDbw flow_run_dbw, CapeErr err)
{
  int res;
  AdblTrx trx = NULL;

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  self->psid = flow_run_dbw_init (flow_run_dbw, trx, self->wfid, self->syncid, TRUE, err);
  if (0 == self->psid)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = flow_process_add__instance (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);

exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_add (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;
  
  // local objects
  FlowRunDbw flow_run_dbw = NULL;

  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (0 == self->wfid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing wfid");
    goto exit_and_cleanup;
  }

  self->sqid = cape_udc_get_n (qin->cdata, "sqtid", FLOW_SQTID__DEFAULT);
  if (0 == self->sqid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'sqtid' is empty or missing");
    goto exit_and_cleanup;
  }

  self->refid = cape_udc_get_n (qin->cdata, "refid", 0);
  if (self->refid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'refid' is empty or missing");
    goto exit_and_cleanup;
  }
  
  // extract the reference id to a sync object
  {
    CapeUdc syncid_node = cape_udc_ext (qin->cdata, FLOW_CDATA__SYNCID);
    if (syncid_node)
    {
      self->syncid = cape_udc_n (syncid_node, 0);

      // cleanup
      cape_udc_del (&syncid_node);
    }
  }

  // create a new run database wrapper
  flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->wpid, 0, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, self->refid);

  {
    CapeUdc params = cape_udc_ext (qin->cdata, "params");
    if (params)
    {
      flow_run_dbw_tdata__merge_to (flow_run_dbw, &params);
    }
  }
  
  // initialize the new flow process
  // -> checks for uniqueness
  res = flow_process_add__init (self, flow_run_dbw, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // forward business logic to this class
  res = flow_run_dbw_start (&flow_run_dbw, FLOW_ACTION__PRIM, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qout->cdata, FLOW_CDATA__PSID, self->psid);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  if (cape_err_code(err))
  {
    cape_log_msg (CAPE_LL_ERROR, "FLOW", "on add", cape_err_text (err));
  }
  
  flow_run_dbw_del (&flow_run_dbw);
  
  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_set (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;
  
  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // check role
  {
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    if (roles == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
      goto exit_and_cleanup;
    }

    CapeUdc fa_role = cape_udc_get (roles, "flow_wp_fa_w");
    if (fa_role)
    {
      self->wpid = 0;
    }
  }
  
  // support both version
  self->psid = cape_udc_get_n (qin->cdata, "psid", cape_udc_get_n (qin->cdata, "taid", 0));
  if (self->psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_set} missing parameter 'psid'");
    goto exit_and_cleanup;
  }  
  
  {
    // create a new run database wrapper
    FlowRunDbw flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->wpid, self->psid, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, 0);

    CapeUdc params = cape_udc_ext (qin->cdata, "params");
    
    // forward business logic to this class
    res = flow_run_dbw_set (&flow_run_dbw, cape_udc_get_n (qin->cdata, "action", FLOW_ACTION__SET), &params, err);
  }
    
exit_and_cleanup:
  
  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_rm__logs (FlowProcess self, AdblTrx trx, number_t psid, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "taid"          , psid);

    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "vdata"         , 0);
    
    // execute the query
    query_results = adbl_trx_query (trx, "proc_task_logs", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    // check for vdata
    {
      number_t dataid = cape_udc_get_n (cursor->item, "vdata", 0);
      if (dataid)
      {
        res = flow_data_rm (trx, dataid, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
      }
    }

    // remove the log entry
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

      cape_udc_add_n      (params, "id"           , cape_udc_get_n (cursor->item, "id", 0));

      res = adbl_trx_delete (trx, "proc_task_logs", &params, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_process_rm__item (FlowProcess self, AdblTrx trx, number_t psid, CapeErr err);

//-----------------------------------------------------------------------------

int proc_taskflow_rm__syncs_process (FlowProcess self, AdblTrx trx, number_t syncid, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "sync"          , syncid);
    cape_udc_add_n      (values, "id"            , 0);
    
    // execute the query
    query_results = adbl_trx_query (trx, "proc_tasks", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    res = flow_process_rm__item (self, trx, cape_udc_get_n (cursor->item, "id", 0), err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
    
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int proc_taskflow_rm__syncs (FlowProcess self, AdblTrx trx, number_t psid, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "taid"          , psid);
    cape_udc_add_n      (values, "id"            , 0);
    
    // execute the query
    query_results = adbl_trx_query (trx, "proc_task_sync", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    number_t syncid = cape_udc_get_n (cursor->item, "id", 0);
    
    // remove all processes which are related to this sync object
    res = proc_taskflow_rm__syncs_process (self, trx, syncid, err);
    
    // remove the sync entry
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

      cape_udc_add_n      (params, "id"           , syncid);

      res = adbl_trx_delete (trx, "proc_task_sync", &params, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int proc_taskflow_rm__entry (FlowProcess self, AdblTrx trx, number_t psid, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"            , psid);
    cape_udc_add_n      (values, "t_data"        , 0);
    
    // execute the query
    query_results = adbl_trx_query (trx, "proc_tasks", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  // remove the logs
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "psid"         , psid);
    
    res = adbl_trx_delete (trx, "flow_log", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  // remove the instance
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "psid"         , psid);
    
    res = adbl_trx_delete (trx, "flow_instance", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  // remove the process
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"           , psid);

    res = adbl_trx_delete (trx, "proc_tasks", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    // check for vdata
    {
      number_t dataid = cape_udc_get_n (cursor->item, "t_data", 0);
      if (dataid)
      {
        res = flow_data_rm (trx, dataid, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
      }
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_process_rm__item (FlowProcess self, AdblTrx trx, number_t psid, CapeErr err)
{
  int res;
  
  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "process rm", "---------------+----------------------------------+");
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "process rm", " REMOVE %4i   |                                  |", psid);
  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "process rm", "---------------+----------------------------------+");
  
  res = flow_process_rm__logs (self, trx, psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = proc_taskflow_rm__syncs (self, trx, psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = proc_taskflow_rm__entry (self, trx, psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_rm (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;
  
  // local objects
  AdblTrx trx = NULL;
  
  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->psid = cape_udc_get_n (qin->cdata, FLOW_CDATA__PSID, 0);
  if (self->psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_rm} missing parameter 'psid'");
    goto exit_and_cleanup;
  }
  
  // TODO: before starting the delete sequence, the delete workflow shall be started
  
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = flow_process_rm__item (self, trx, self->psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  adbl_trx_rollback (&trx, err);

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_all (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;
  
  number_t tdataid;

  // local objects
  CapeUdc query_results = NULL;

  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // check role
  {
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    if (roles == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
      goto exit_and_cleanup;
    }
    
    CapeUdc fa_role = cape_udc_get (roles, "flow_wp_fa_r");
    if (fa_role)
    {
      self->wpid = 0;
    }
  }

  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "wfid"          , 0);
    cape_udc_add_n      (values, "active"        , 0);
    cape_udc_add_s_cp   (values, "step_name"     , NULL);
    cape_udc_add_n      (values, "fctid"         , 0);
    cape_udc_add_s_cp   (values, "wf_name"       , NULL);
    cape_udc_add_n      (values, "t_data"        , 0);
    cape_udc_add_n      (values, "p_data"        , 0);

    if (self->wpid)
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

      cape_udc_add_n    (params, "wpid"          , self->wpid);

      // execute the query
      // select ps.id, ps.wpid, ps.wfid, ps.active, ws.name step_name, ws.fctid, wf.name wf_name, ps.t_data, ws.p_data from proc_tasks ps left join proc_worksteps ws on ws.id = ps.current_step join proc_workflows wf on wf.id = ps.wfid where sync is null;
      query_results = adbl_session_query (self->adbl_session, "flow_process_get_view", &params, &values, err);
    }
    else
    {
      // execute the query
      query_results = adbl_session_query (self->adbl_session, "flow_process_get_view", NULL, &values, err);
    }

    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }

  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_get (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;
  CapeString remote = NULL;
  CapeUdc data = NULL;
  CapeUdc client = NULL;
  
  number_t logtype = 0;
  number_t wsid = 0;
  
  CapeUdc tdata_node = NULL;
  CapeUdc tdata = NULL;
  
  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // check role
  {
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    if (roles == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
      goto exit_and_cleanup;
    }
    
    CapeUdc fa_role = cape_udc_get (roles, "flow_wp_fa_r");
    if (fa_role)
    {
      self->wpid = 0;
    }
  }

  // support both versions
  self->psid = cape_udc_get_n (qin->cdata, "psid", cape_udc_get_n (qin->cdata, "taid", 0));
  if (self->psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_get} missing parameter 'psid'");
    goto exit_and_cleanup;
  }
  
  // get info about the used device
  client = cape_udc_ext (qin->cdata, "client");
  
  // get the remote address
  remote = cape_str_cp (cape_udc_get_s (qin->rinfo, "remote", NULL));

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "process get", "fetch info from process psid = %i, remote = %s", self->psid, remote);
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"            , self->psid);

    if (self->wpid)
    {
      cape_udc_add_n    (params, "wpid"          , self->wpid);
    }

    cape_udc_add_n      (values, "active"        , 0);
    cape_udc_add_n      (values, "wfid"          , 0);
    cape_udc_add_n      (values, "tdata"         , 0);
    cape_udc_add_n      (values, "wsid"          , 0);
    cape_udc_add_n      (values, "log"           , 0);

    /*
     flow_process_view
     
     create view flow_process_view as select ps.id, ps.wpid, ps.wfid, ps.active, ps.refid, ps.sync, ws.id wsid, ws.tag, ws.log, ps.t_data tdata from proc_tasks ps left join proc_worksteps ws on ws.id = ps.current_step;
     */
    
    query_results = adbl_session_query (self->adbl_session, "flow_process_view", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_ext_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find the process");
    goto exit_and_cleanup;
  }

  logtype = cape_udc_get_n (first_row, "log", 0);

  wsid = cape_udc_get_n (first_row, "wsid", 0);
  if (wsid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find the process");
    goto exit_and_cleanup;
  }

  {
    number_t active = cape_udc_get_n (first_row, "active", 0);
    if (active == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "task is not active anymore");
      goto exit_and_cleanup;
    }
  }
  
  {
    tdata_node = cape_udc_ext (first_row, "tdata");
    if (tdata_node)
    {
      number_t tdataid = cape_udc_n (tdata_node, 0);
      if (tdataid)
      {
        tdata = flow_data_get (self->adbl_session, tdataid, err);
        if (NULL == tdata)
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
        
        cape_udc_add_name (first_row, &tdata, "tdata");
      }
    }
  }
  
  data = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  if (remote)
  {
    cape_udc_add_s_cp (data, "remote", remote);
  }
  
  if (client)
  {
    cape_udc_add (data, &client);
  }

  res = flow_log_add (self->adbl_session, self->psid, wsid, logtype, FLOW_STATE__GET, data, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // add psid
  cape_udc_add_n (first_row, "psid", self->psid);
  
  cape_udc_replace_mv (&(qout->cdata), &first_row);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  cape_udc_del (&first_row);
  
  cape_udc_del (&tdata_node);
  cape_udc_del (&tdata);
  
  cape_str_del (&remote);
  cape_udc_del (&data);
  cape_udc_del (&client);

  // cleanup
  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_details (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  number_t wpid;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // support both versions
  self->psid = cape_udc_get_n (qin->cdata, "psid", cape_udc_get_n (qin->cdata, "taid", 0));
  if (self->psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_get} missing parameter 'psid'");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"             , self->psid);
    
    cape_udc_add_n      (values, "wpid"           , 0);
    cape_udc_add_n      (values, "wfid"           , 0);
    cape_udc_add_n      (values, "current_step"   , 0);
    cape_udc_add_n      (values, "current_state"  , 0);
    
    cape_udc_add_n      (values, "sync_pa_id"     , 0);
    cape_udc_add_n      (values, "sync_pa_cnt"    , 0);
    cape_udc_add_n      (values, "sync_pa_wsid"   , 0);

    cape_udc_add_n      (values, "sync_ci_id"     , 0);
    cape_udc_add_n      (values, "sync_ci_cnt"    , 0);
    cape_udc_add_n      (values, "sync_ci_wsid"   , 0);

    cape_udc_add_node   (values, "tdata"          );
    cape_udc_add_node   (values, "pdata"          );
    
    /*
     flow_process_details_view

     select ps.id, ps.wpid, ps.wfid, ps.current_step, ps.current_state, ps.sync sync_pa_id, pa.cnt sync_pa_cnt, pa.wsid sync_pa_wsid, ci.id sync_ci_id, ci.cnt sync_ci_cnt, ci.wsid sync_ci_wsid, td.content tdata, pd.content pdata from proc_tasks ps left join proc_task_sync pa on ps.sync = pa.id left join proc_task_sync ci on ci.taid = ps.id join proc_data td on td.id = ps.t_data left join proc_worksteps ws on ws.id = ps.current_step left join proc_data pd on pd.id = ws.p_data;
     */
    
    query_results = adbl_session_query (self->adbl_session, "flow_process_details_view", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_ext_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find the process");
    goto exit_and_cleanup;
  }
  
  /*
  wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "{flow_process_get} data set has no 'wpid'");
    goto exit_and_cleanup;
  }
  */
  
  cape_udc_replace_mv (&(qout->cdata), &first_row);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&query_results);
  cape_udc_del (&first_row);

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process__instance_check (FlowProcess self, number_t* p_wpid, number_t* p_gpid, CapeErr err)
{
  int res;

  CapeUdc first_row;

  // local objects
  CapeUdc query_results = NULL;
  number_t wpid = 0;
  number_t gpid = 0;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "psid"           , self->psid);
    cape_udc_add_n      (values, "wpid"           , 0);
    cape_udc_add_n      (values, "gpid"           , 0);
    
    query_results = adbl_session_query (self->adbl_session, "flow_instance", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find the process");
    goto exit_and_cleanup;
  }
  
  wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "wpid is missing");
    goto exit_and_cleanup;
  }

  gpid = cape_udc_get_n (first_row, "gpid", 0);

  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  *p_wpid = wpid;
  *p_gpid = gpid;

  return res;
}

//-----------------------------------------------------------------------------

int flow_process_once__dbw (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // create a new run database wrapper
  FlowRunDbw flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->wpid, self->psid, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, 0);
  
  // forward business logic to this class
  res = flow_run_dbw_once (&flow_run_dbw, err);
  
exit_and_cleanup:
  
  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL flow_process_once__on_switch (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }

  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  {
    CapeString h = cape_json_to_s (qin->rinfo);
    
    printf ("RINFO: %s\n", h);
  }
  
  res = flow_process_once__dbw (&self, qin, qout, err);
  
exit_and_cleanup:
  
  flow_process_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_once (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;
  
  number_t wpid;
  number_t gpid;
  
  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // support both versions
  self->psid = cape_udc_get_n (qin->cdata, "psid", cape_udc_get_n (qin->cdata, "taid", 0));
  if (self->psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_once} missing parameter 'psid'");
    goto exit_and_cleanup;
  }

  // check if the instance exists and fetch the original wpid and gpid
  res = flow_process__instance_check (self, &wpid, &gpid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  if (self->wpid != wpid)
  {
    // check role
    {
      CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
      if (roles == NULL)
      {
        res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
        goto exit_and_cleanup;
      }
      
      CapeUdc fa_role = cape_udc_get (roles, "flow_wp_fa_w");
      if (fa_role == NULL)
      {
        res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
        goto exit_and_cleanup;
      }
    }

    // clean up
    qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
    
    qin->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n (qin->pdata, "wpid", wpid);
    cape_udc_add_n (qin->pdata, "gpid", gpid);

    res = qbus_continue (self->qbus, "AUTH", "ui_switch", qin, (void**)p_self, flow_process_once__on_switch, err);
  }
  else
  {
    res = flow_process_once__dbw (p_self, qin, qout, err);
  }
  
exit_and_cleanup:
  
  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_prev (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  CapeUdc first_row;
  number_t prev;

  // local objects
  CapeUdc query_results = NULL;
  AdblTrx trx = NULL;

  res = flow_process__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // support both versions
  self->psid = cape_udc_get_n (qin->cdata, "psid", cape_udc_get_n (qin->cdata, "taid", 0));
  if (self->psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_prev} missing parameter 'psid'");
    goto exit_and_cleanup;
  }

  // fetch info about this process
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"             , self->psid);
    
    cape_udc_add_n      (values, "wpid"           , 0);
    cape_udc_add_n      (values, "wfid"           , 0);
    cape_udc_add_n      (values, "active"         , 0);
    cape_udc_add_n      (values, "sync"           , 0);
    cape_udc_add_n      (values, "current_step"   , 0);
    cape_udc_add_n      (values, "prev"           , 0);

    /*
    flow_process_prev_view
     
    select ps.id, ps.wpid, ps.wfid, ps.active, ps.sync, ps.current_step, ws.prev from proc_tasks ps left join proc_worksteps ws on ws.id = ps.current_step;
    */
    
    query_results = adbl_session_query (self->adbl_session, "flow_process_prev_view", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find the process");
    goto exit_and_cleanup;
  }

  // check active
  {
    number_t active = cape_udc_get_n (first_row, "active", 0);
    if (active == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "process is not active");
      goto exit_and_cleanup;
    }
  }

  // check sync
  {
    number_t sync = cape_udc_get_n (first_row, "sync", 0);
    if (sync)
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "can't set previous step if sync is present");
      goto exit_and_cleanup;
    }
  }

  // get the previous step id
  prev = cape_udc_get_n (first_row, "prev", 0);
  if (prev == 0)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "there is no previous step");
    goto exit_and_cleanup;
  }

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"              , self->psid);
    cape_udc_add_n      (values, "current_step"    , prev);
    cape_udc_add_n      (values, "current_state"   , FLOW_STATE__NONE);

    // update status
    res = adbl_trx_update (trx, "proc_tasks", &params, &values, err);
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
  
  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
