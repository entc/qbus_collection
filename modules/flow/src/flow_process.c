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
  QJobs jobs;                  // reference

  number_t wpid;               // workspace id
  number_t gpid;               // global person id
  number_t wfid;               // workflow id
  number_t sqid;               // sequence id
  number_t psid;               // process id
  number_t fiid;               // flow instance ID

  CapeString modp;             // the name of the module
  number_t refid;              // reference id
  number_t syncid;             // process sync id

  number_t tdata_id;           // container to store all variables of the flow process

  CapeUdc tmp_data;
};

//-----------------------------------------------------------------------------

FlowProcess flow_process_new (QBus qbus, AdblSession adbl_session, CapeQueue queue, QJobs jobs)
{
  FlowProcess self = CAPE_NEW(struct FlowProcess_s);

  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->queue = queue;
  self->jobs = jobs;

  self->wpid = 0;
  self->gpid = 0;
  self->wfid = 0;
  self->psid = 0;
  self->sqid = 0;
  self->fiid = 0;

  self->modp = NULL;
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

    cape_str_del (&(self->modp));
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
    return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
  }

  self->gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (self->gpid == 0)
  {
    return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
  }

  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_process_add__instance (FlowProcess self, AdblTrx trx, CapeString* p_cb_module, CapeString* p_cb_method, CapeUdc* p_cb_params, CapeErr err)
{
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n      (values, "wpid"         , self->wpid);
  cape_udc_add_n      (values, "gpid"         , self->gpid);
  cape_udc_add_n      (values, "wfid"         , self->wfid);
  cape_udc_add_s_cp   (values, "modp"         , self->modp);
  cape_udc_add_n      (values, "refid"        , self->refid);
  cape_udc_add_n      (values, "psid"         , self->psid);

  {
    CapeDatetime dt; cape_datetime_utc (&dt);
    cape_udc_add_d (values, "toc", &dt);
  }

  if (*p_cb_module && *p_cb_method)
  {
    cape_udc_add_s_mv   (values, "module"         , p_cb_module);
    cape_udc_add_s_mv   (values, "method"         , p_cb_method);

    if (*p_cb_params)
    {
      cape_udc_add_name (values, p_cb_params, "params");
    }
  }

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

int flow_process_add__init (FlowProcess self, FlowRunDbw flow_run_dbw, CapeString* p_cb_module, CapeString* p_cb_method, CapeUdc* p_cb_params, CapeErr err)
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

  res = flow_process_add__instance (self, trx, p_cb_module, p_cb_method, p_cb_params, err);
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

/*
update database

alter table flow_instance add column modp varchar(20) not null default 'MISC';
alter table flow_instance drop index uc_flow_instance, add unique key uc_flow_instance (wfid, modp, refid);

alter table flow_instance add column module varchar(30);
alter table flow_instance add column method varchar(50);
alter table flow_instance add column params varchar(1000);

*/

int flow_process_add (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // local objects
  FlowRunDbw flow_run_dbw = NULL;
  CapeString cb_module = NULL;
  CapeString cb_method = NULL;
  CapeUdc cb_params = NULL;

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

  // still optional
  self->modp = cape_str_cp (cape_udc_get_s (qin->cdata, "modp", "MISC"));

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
  flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->jobs, self->wpid, 0, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, self->refid);

  {
    CapeUdc params = cape_udc_ext (qin->cdata, "params");
    if (params)
    {
      flow_run_dbw_tdata__merge_to (flow_run_dbw, &params);
    }
  }

  {
    CapeUdc cb = cape_udc_get (qin->cdata, "_cb");
    if (cb)
    {
      // callback (optional)
      cb_module = cape_udc_ext_s (cb, "module");
      cb_method = cape_udc_ext_s (cb, "method");
      cb_params = cape_udc_ext (cb, "params");
    }
  }

  // initialize the new flow process
  // -> checks for uniqueness
  res = flow_process_add__init (self, flow_run_dbw, &cb_module, &cb_method, &cb_params, err);
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

  cape_str_del (&cb_module);
  cape_str_del (&cb_method);
  cape_udc_del (&cb_params);

  flow_run_dbw_del (&flow_run_dbw);

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process__process_check (FlowProcess self, number_t* p_wpid, number_t* p_gpid, CapeErr err)
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

    cape_udc_add_n      (params, "id"             , self->psid);
    cape_udc_add_n      (values, "wpid"           , 0);
    //cape_udc_add_n      (values, "gpid"           , 0);

    query_results = adbl_session_query (self->adbl_session, "proc_tasks", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
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

  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_process_set (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  CapeUdc first_row;
  number_t wpid;
  number_t gpid;

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

  // check if the instance exists and fetch the original wpid and gpid
  res = flow_process__process_check (self, &wpid, &gpid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (self->wpid)
  {
    if (wpid != self->wpid)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "{flow_process_set} workspace missmatch");
      goto exit_and_cleanup;
    }
  }
  else
  {
    // switch to another wpid
    // TODO: dirty workaround
    cape_udc_put_n (qin->rinfo, "wpid", wpid);
  }

  {
    // create a new run database wrapper
    FlowRunDbw flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->jobs, self->wpid, self->psid, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, 0);

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
    if (res)
    {
      goto exit_and_cleanup;
    }

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

int flow_process_instance_rm__instance (FlowProcess self, AdblTrx trx, CapeErr err)
{
  int res;

  CapeUdc first_row;
  number_t psid;

  // local objects
  CapeUdc query_results = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "refid"         , self->refid);
    cape_udc_add_n      (params, "wpid"          , self->wpid);
    cape_udc_add_n      (values, "gpid"          , 0);
    cape_udc_add_n      (values, "psid"          , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_instance", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }

  psid = cape_udc_get_n (first_row, "psid", 0);
  if (psid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "psid not found");
    goto exit_and_cleanup;
  }

  // optional
  self->gpid = cape_udc_get_n (first_row, "gpid", 0);

  res = flow_process_rm__item (self, trx, psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_instance_rm__tasks (FlowProcess self, AdblTrx trx, CapeErr err)
{
  int res;

  CapeUdc first_row;
  number_t psid;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "refid"         , self->refid);
    cape_udc_add_n      (params, "wpid"          , self->wpid);
    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "sync"          , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_tasks", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    number_t psid = cape_udc_get_n (cursor->item, "id", 0);
    number_t sync = cape_udc_get_n (cursor->item, "sync", 0);

    if (sync == 0)
    {
      res = flow_process_rm__item (self, trx, psid, err);
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

int flow_process_instance_rm (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // local objects
  AdblTrx trx = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "missing cdata");
    goto exit_and_cleanup;
  }

  self->refid = cape_udc_get_n (qin->cdata, "refid", 0);
  if (self->refid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing refid");
    goto exit_and_cleanup;
  }

  // allow only admin role from workspace
  if (qbus_message_role_has (qin, "flow_admin"))
  {
    self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    if (self->wpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing wpid");
      goto exit_and_cleanup;
    }
  }
  else
  {
    self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    if (self->wpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing wpid");
      goto exit_and_cleanup;
    }
  }

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = flow_process_instance_rm__instance (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = flow_process_instance_rm__tasks (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "instance rm", cape_err_text (err));
  }

  adbl_trx_rollback (&trx, err);

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_wait_get (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // local objects
  CapeString uuid = NULL;
  number_t code = 0;
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_ROLE");
    goto exit_and_cleanup;
  }

  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_PDATA");
    goto exit_and_cleanup;
  }

  self->psid = cape_udc_get_n (qin->pdata, "psid", 0);
  if (0 == self->psid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_PSID");
    goto exit_and_cleanup;
  }

  code = cape_udc_get_n (qin->pdata, "code", 0);
  if (0 == code)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CODE");
    goto exit_and_cleanup;
  }

  uuid = cape_udc_ext_s (qin->pdata, "uuid");
  if (NULL == uuid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_UUID");
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "wait get", "check for wait process = %i and uuid = %s", self->psid, uuid);

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "psid"          , self->psid);
    cape_udc_add_s_mv   (params, "uuid"          , &uuid);

    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "status"        , 0);
    cape_udc_add_n      (values, "code"          , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_wait_items", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_ext_first (query_results);

  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NOT_FOUND");
    goto exit_and_cleanup;
  }

  // check code
  {
    CapeUdc code_node = cape_udc_get (first_row, "code");
    if (code_node)
    {
      if (code != cape_udc_n (code_node, 0))
      {
        res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.CODE_MISSMATCH");
        goto exit_and_cleanup;
      }
    }
  }

  cape_udc_replace_mv (&(qout->pdata), &first_row);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&first_row);
  cape_udc_del (&query_results);
  cape_str_del (&uuid);

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
    cape_udc_add_n      (values, "refid"         , 0);
    cape_udc_add_d      (values, "toc"           , NULL);
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

      /*
       flow_process_get_view

       select ps.id, ps.wpid, ps.wfid, fi.refid, ps.active, ws.name step_name, ws.fctid, wf.name wf_name, ps.t_data, ws.p_data, fi.toc from proc_tasks ps left join proc_worksteps ws on ws.id = ps.current_step join proc_workflows wf on wf.id = ps.wfid join flow_instance fi on fi.wfid = wf.id and fi.psid = ps.id;
       */

      // execute the query
      // select ps.id, ps.wpid, ps.wfid, fi.refid, fi.toc, ps.active, ws.name step_name, ws.fctid, wf.name wf_name, ps.t_data, ws.p_data from proc_tasks ps left join proc_worksteps ws on ws.id = ps.current_step join proc_workflows wf on wf.id = ps.wfid join flow_instance fi on fi.wfid = wf.id and fi.psid = ps.id;
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

CapeUdc flow_process__internal__get (FlowProcess self, number_t wpid, number_t psid, number_t psid_root, CapeErr err)
{
  CapeUdc ret = NULL;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , psid);

    if (wpid)
    {
      cape_udc_add_n    (params, "wpid"          , wpid);
    }

    cape_udc_add_n      (values, "active"        , 0);
    cape_udc_add_n      (values, "wfid"          , 0);
    cape_udc_add_n      (values, "tdata"         , 0);
    cape_udc_add_n      (values, "wsid"          , 0);
    cape_udc_add_n      (values, "log"           , 0);
    cape_udc_add_s_cp   (values, "tag"           , NULL);

    /*
     flow_process_view

     select ps.id, ps.wpid, ps.wfid, ps.active, ps.refid, ps.sync, ws.id wsid, ws.tag, ws.log, ps.t_data tdata from proc_tasks ps left join proc_worksteps ws on ws.id = ps.current_step;
     */

    query_results = adbl_session_query (self->adbl_session, "flow_process_view", &params, &values, err);
    if (NULL == query_results)
    {
      cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NO_PROCESS");
      goto exit_and_cleanup;
    }
  }

  // extract the first row of the result-set
  first_row = cape_udc_ext_first (query_results);
  if (NULL == first_row)
  {
    cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NO_PROCESS");
    goto exit_and_cleanup;
  }

  number_t active = cape_udc_get_n (first_row, "active", 0);
  if (0 == active)
  {
    if (0 == psid_root)
    {
      cape_err_set (err, CAPE_ERR_NOT_FOUND, "task is not active anymore");
      goto exit_and_cleanup;
    }

    cape_log_fmt (CAPE_LL_TRACE, "FLOW", "process get", "process is not active anymore, try root process psid = %i", psid_root);

    // if the process is not active anymore, check psid root
    ret = flow_process__internal__get (self, wpid, psid_root, 0, err);
  }
  else
  {
    // use the first row as return value
    ret = cape_udc_mv (&first_row);
  }

exit_and_cleanup:

  cape_udc_del (&first_row);
  cape_udc_del (&query_results);

  return ret;
}

//-----------------------------------------------------------------------------

int flow_process_get (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // local objects
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

  // try to get the process using psid
  first_row = flow_process__internal__get (self, self->wpid, self->psid, cape_udc_get_n (qin->cdata, "psid_root", 0), err);
  if (NULL == first_row)
  {
    res = cape_err_code (err);
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

    cape_udc_add_s_cp   (values, "tdata"          , "{\"size\": 20000}");
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

  {
    CapeUdc tdata_node = cape_udc_ext (first_row, "tdata");
    if (tdata_node)
    {
      const CapeString h1 = cape_udc_s (tdata_node, NULL);
      if (h1)
      {
        CapeUdc h2 = cape_json_from_s (h1);
        if (h2)
        {
          cape_udc_add_name (first_row, &h2, "tdata");
        }
      }

      cape_udc_del (&tdata_node);
    }
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
      res = cape_err_code (err);
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
  FlowRunDbw flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->jobs, self->wpid, self->psid, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, 0);

  // forward business logic to this class
  res = flow_run_dbw_once (&flow_run_dbw, err);

exit_and_cleanup:

  flow_process_del (p_self);
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

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "process once", "using psid = %i", self->psid);

  // check if the instance exists and fetch the original wpid and gpid
  res = flow_process__process_check (self, &wpid, &gpid, err);
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

    // switch to another wpid
    // TODO: dirty workaround
    cape_udc_put_n (qin->rinfo, "wpid", wpid);

    self->wpid = wpid;
  }

  res = flow_process_once__dbw (p_self, qin, qout, err);

exit_and_cleanup:

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_next__dbw (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // create a new run database wrapper
  FlowRunDbw flow_run_dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->jobs, self->wpid, self->psid, cape_udc_get_s (qin->rinfo, "remote", NULL), qin->rinfo, 0);

  // forward business logic to this class
  res = flow_run_dbw_next (&flow_run_dbw, err);

exit_and_cleanup:

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_next (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  number_t wpid;
  number_t gpid;

  if (!qbus_message_role_has (qin, "flow_wp_fa_w"))
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
    goto exit_and_cleanup;
  }

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

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "process next", "using psid = %i", self->psid);

  // check if the instance exists and fetch the original wpid and gpid
  res = flow_process__process_check (self, &wpid, &gpid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // switch to another wpid
  // TODO: dirty workaround
  cape_udc_put_n (qin->rinfo, "wpid", wpid);

  self->wpid = wpid;

  res = flow_process_next__dbw (p_self, qin, qout, err);

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
  /*
  {
    number_t active = cape_udc_get_n (first_row, "active", 0);
    if (active == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "process is not active");
      goto exit_and_cleanup;
    }
  }
   */

  // check sync
  /*
  {
    number_t sync = cape_udc_get_n (first_row, "sync", 0);
    if (sync)
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "can't set previous step if sync is present");
      goto exit_and_cleanup;
    }
  }
   */

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

int flow_process_step__sync_rm (FlowProcess self, AdblTrx trx, number_t wsid, CapeErr err);

//-----------------------------------------------------------------------------

int flow_process_step__sync_next (FlowProcess self, AdblTrx trx, number_t wsid, CapeErr err)
{
  int res;
  CapeUdc first_row;

  // local objects
  CapeUdc query_results = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "prev"          , wsid);
    cape_udc_add_n      (params, "sqtid"         , self->sqid);

    cape_udc_add_n      (values, "id"            , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_worksteps", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (first_row)
  {
    number_t wsid_next = cape_udc_get_n (first_row, "id", 0);
    if (wsid_next)
    {
      res = flow_process_step__sync_rm (self, trx, wsid_next, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_process_step__sync_rm (FlowProcess self, AdblTrx trx, number_t wsid, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "sync rm", "remove sync for wsid = %i", wsid);

  // fetch info about this sync
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "taid"           , self->psid);
    cape_udc_add_n      (params, "wsid"           , wsid);

    cape_udc_add_n      (values, "id"             , 0);

    query_results = adbl_session_query (self->adbl_session, "proc_task_sync", &params, &values, err);
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
    if (res)
    {
      goto exit_and_cleanup;
    }

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

  // continue with the next workstep
  res = flow_process_step__sync_next (self, trx, wsid, err);

exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_process_step (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  number_t wsid;
  CapeUdc first_row;

  // local objects
  AdblTrx trx = NULL;
  CapeUdc query_results = NULL;

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

  wsid = cape_udc_get_n (qin->cdata, "wsid", 0);
  if (wsid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_prev} missing parameter 'wsid'");
    goto exit_and_cleanup;
  }

  // fetch info about this workstep
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "psid"           , self->psid);
    cape_udc_add_n      (params, "wsid"           , wsid);

    cape_udc_add_n      (values, "sqtid"          , 0);

    /*
     flow_worksteps_tasks_view

     select ps.id psid, ws.id wsid, sqtid from proc_worksteps ws join proc_tasks ps on ps.wfid = ws.wfid;
     */

    query_results = adbl_session_query (self->adbl_session, "flow_worksteps_tasks_view", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "{flow_process_step} workstep not found");
    goto exit_and_cleanup;
  }

  self->sqid = cape_udc_get_n (first_row, "sqtid", 0);

  // start transaction
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
    cape_udc_add_n      (values, "current_step"    , wsid);
    cape_udc_add_n      (values, "current_state"   , FLOW_STATE__NONE);

    // update status
    res = adbl_trx_update (trx, "proc_tasks", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = flow_process_step__sync_rm (self, trx, wsid, err);
  if (res)
  {
    goto exit_and_cleanup;
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

int flow_wspc_clr__process (FlowProcess self, number_t psid, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = flow_process_rm__item (self, trx, psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int flow_wspc_clr (FlowProcess* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowProcess self = *p_self;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  // allow user role or admin role from workspace
  if (qbus_message_role_has (qin, "admin") == FALSE)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing role");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_PDATA");
    goto exit_and_cleanup;
  }

  self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing wpid");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "wpid"          , self->wpid);
    cape_udc_add_n      (values, "id"            , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_tasks", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    res = flow_wspc_clr__process (self, cape_udc_get_n (cursor->item, "id", 0), err);
    if (res)
    {
      // ignore errors
      cape_err_clr (err);
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  flow_process_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
