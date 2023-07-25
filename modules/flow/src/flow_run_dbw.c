#include "flow_run_dbw.h"
#include "flow_run_step.h"
#include "flow_chain.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>
#include <fmt/cape_tokenizer.h>
#include <fmt/cape_template.h>

//-----------------------------------------------------------------------------

number_t flow_data_add (AdblTrx trx, CapeUdc content, CapeErr err)
{
  number_t ret = 0;

  if (content == NULL)
  {
    return 0;
  }

  if (cape_udc_size (content) == 0)
  {
    return 0;
  }

  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    {
      CapeString h = cape_json_to_s (content);
      cape_udc_add_s_mv    (values, "content"         , &h);
    }

    // execute the query
    ret = adbl_trx_insert (trx, "proc_data", &values, err);
  }

  return ret;
}

//-----------------------------------------------------------------------------

int flow_data_set (AdblTrx trx, number_t dataid, CapeUdc content, CapeErr err)
{
  int res;

  if (content == NULL)
  {
    return CAPE_ERR_NONE;
  }

  if (cape_udc_size (content) == 0)
  {
    return CAPE_ERR_NONE;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , dataid);

    {
      CapeString h = cape_json_to_s (content);
      cape_udc_add_s_mv    (values, "content"         , &h);
    }

    // execute the query
    res = adbl_trx_update (trx, "proc_data", &params, &values, err);
  }

  return res;
}

//-----------------------------------------------------------------------------

int flow_data_rm (AdblTrx trx, number_t dataid, CapeErr err)
{
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n      (params, "id"           , dataid);

  return adbl_trx_delete (trx, "proc_data", &params, err);
}

//-----------------------------------------------------------------------------

CapeUdc flow_data_get (AdblSession adbl_session, number_t dataid, CapeErr err)
{
  CapeUdc ret = NULL;

  const CapeString content_json;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc data = NULL;
  CapeUdc content = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , dataid);

    // default size is 4000 -> increase to 20000 bytes
    cape_udc_add_s_cp   (values, "content"       , "{\"size\": 20000}");

    // execute the query
    query_results = adbl_session_query (adbl_session, "proc_data", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }

  data = cape_udc_ext_first (query_results);
  if (NULL == data)
  {
    cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find proc_data");
    goto exit_and_cleanup;
  }

  content_json = cape_udc_get_s (data, "content", NULL);
  if (NULL == content_json)
  {
    cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find content");
    goto exit_and_cleanup;
  }

  content = cape_json_from_s (content_json);
  if (NULL == content)
  {
    cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't parse content");
    goto exit_and_cleanup;
  }

  cape_udc_replace_mv (&ret, &content);

exit_and_cleanup:

  cape_udc_del (&query_results);
  cape_udc_del (&data);
  cape_udc_del (&content);

  return ret;
}

//-----------------------------------------------------------------------------

struct FlowRunDbw_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  CapeQueue queue;             // reference
  QJobs jobs;                  // reference

  CapeString remote;
  CapeUdc rinfo;

  number_t wpid;               // workspace id
  number_t psid;               // process id

  // current variables

  number_t wsid;               // workstep id
  number_t sqid;               // sequence id

  number_t fctid;              // function id
  number_t refid;              // reference id

  number_t syncid;             // syncronization id
  number_t syncnt;             // syncronization counter
  number_t syncmy;             // referenced syncid on itself

  number_t pdata_id;           // parameters for this step
  CapeUdc pdata;

  number_t tdata_id;           // runtime variables
  CapeUdc tdata;

  number_t state;
  number_t log_type;

  CapeUdc params;
};

//-----------------------------------------------------------------------------

FlowRunDbw flow_run_dbw_new (QBus qbus, AdblSession adbl_session, CapeQueue queue, QJobs jobs, number_t wpid, number_t psid, const CapeString remote, CapeUdc rinfo, number_t refid)
{
  FlowRunDbw self = CAPE_NEW(struct FlowRunDbw_s);

  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->queue = queue;
  self->jobs = jobs;

  self->remote = cape_str_cp (remote);
  self->rinfo = cape_udc_cp (rinfo);
  self->refid = refid;

  self->wpid = wpid;
  self->psid = psid;

  self->wsid = 0;
  self->sqid = FLOW_SEQUENCE__DEFAULT;

  self->fctid = 0;

  self->syncid = 0;
  self->syncnt = 0;
  self->syncmy = 0;

  self->pdata_id = 0;
  self->pdata = NULL;

  self->tdata_id = 0;
  self->tdata = NULL;

  self->state = 0;
  self->log_type = 0;

  self->params = NULL;

  return self;
}

//-----------------------------------------------------------------------------

void flow_run_dbw_del (FlowRunDbw* p_self)
{
  if (*p_self)
  {
    FlowRunDbw self = *p_self;

    cape_str_del (&(self->remote));
    cape_udc_del (&(self->rinfo));

    cape_udc_del (&(self->tdata));
    cape_udc_del (&(self->pdata));

    CAPE_DEL (p_self, struct FlowRunDbw_s);
  }
}

//-----------------------------------------------------------------------------

FlowRunDbw flow_run_dbw_clone (FlowRunDbw rhs, number_t psid, number_t sqid, number_t refid)
{
  FlowRunDbw self = CAPE_NEW(struct FlowRunDbw_s);

  self->qbus = rhs->qbus;
  self->adbl_session = rhs->adbl_session;
  self->queue = rhs->queue;

  self->remote = cape_str_cp (rhs->remote);
  self->rinfo = cape_udc_cp (rhs->rinfo);
  self->refid = refid;

  self->wpid = rhs->wpid;
  self->psid = psid;

  self->wsid = 0;
  self->sqid = sqid;

  self->fctid = 0;

  self->syncid = 0;
  self->syncnt = 0;
  self->syncmy = 0;

  self->pdata_id = 0;
  self->pdata = NULL;

  self->tdata_id = 0;

  // copy the tdata
  self->tdata = cape_udc_cp (rhs->tdata);

  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "clone", "clone a process with refid = %i", refid);

  if (self->tdata)
  {
    CapeUdc refid_node = cape_udc_ext (self->tdata, "refid");

    cape_udc_del (&refid_node);

    cape_udc_add_n (self->tdata, "refid", self->refid);
  }

  /*
  self->vdata_id = 0;
  self->vdata = NULL;
  */

  //self->logid = 0;
  self->state = 0;

  return self;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__tdata_update (FlowRunDbw self, AdblTrx trx, CapeErr err)
{
  int res;

  if (self->tdata_id)
  {
    if (self->tdata)
    {
      res = flow_data_set (trx, self->tdata_id, self->tdata, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
    else
    {
      res = flow_data_rm (trx, self->tdata_id, err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      self->tdata_id = 0;
    }
  }
  else
  {
    if (self->tdata)
    {
      self->tdata_id = flow_data_add (trx, self->tdata, err);
      if (0 == self->tdata_id)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_init__save (FlowRunDbw self, AdblTrx trx, number_t wfid, CapeErr err)
{
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n    (values, "wpid"         , self->wpid);
  cape_udc_add_n    (values, "wfid"         , wfid);
  cape_udc_add_n    (values, "active"       , 1);
  cape_udc_add_n    (values, "refid"        , self->refid);

  if (self->syncid)
  {
    cape_udc_add_n    (values, "sync"        , self->syncid);
  }

  if (self->tdata_id)
  {
    cape_udc_add_n    (values, "t_data"      , self->tdata_id);
  }

  // execute the query
  self->psid = adbl_trx_insert (trx, "proc_tasks", &values, err);

  return (0 == self->psid) ? cape_err_code (err) : CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

number_t flow_run_dbw_init (FlowRunDbw self, AdblTrx trx, number_t wfid, number_t syncid, int add_psid, CapeErr err)
{
  int res;

  self->syncid = syncid;

  // create a new data entry if there is content
  self->tdata_id = flow_data_add (trx, self->tdata, err);
  if (cape_err_code (err))
  {
    goto exit_and_cleanup;
  }

  // create a new taskflow: set self->psid
  res = flow_run_dbw_init__save (self, trx, wfid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (NULL == self->tdata)
  {
    // if there is no TDATA create a new node
    self->tdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  }

  // set the current psid
  cape_udc_put_n (self->tdata, "psid", self->psid);

  if (add_psid)
  {
    // add the PSID for the tdata, which will be available in all sub-processes aswell
    cape_udc_put_n (self->tdata, "psid_root", self->psid);
  }

  res = flow_run_dbw__tdata_update (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run init", "------------------------------------------------------------");
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run init", " P R O C E S S   C R E A T E D                      [%4i]", wfid);
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run init", "------------------------------------------------------------");

exit_and_cleanup:

  return self->psid;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__data_load (FlowRunDbw self, CapeErr err)
{
  int res;

  if (self->pdata_id)
  {
    self->pdata = flow_data_get (self->adbl_session, self->pdata_id, err);

    res = cape_err_code (err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    /*
    // debug
    {
      CapeString h = cape_json_to_s (self->pdata);

      printf ("..............................................................................\n");
      printf ("PDATA: %s\n", h);
    }
     */
  }

  if (self->tdata_id)
  {
    self->tdata = flow_data_get (self->adbl_session, self->tdata_id, err);

    res = cape_err_code (err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    /*
    // debug
    {
      CapeString h = cape_json_to_s (self->tdata);

      printf ("..............................................................................\n");
      printf ("TDATA: %s\n", h);
    }
     */
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__next_load (FlowRunDbw self, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    if (self->wpid)
    {
      cape_udc_add_n      (params, "wpid"          , self->wpid);
    }

    cape_udc_add_n      (params, "taid"          , self->psid);
    cape_udc_add_n      (params, "sqtid"         , self->sqid);

    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "sqtid"         , 0);

    cape_udc_add_n      (values, "fctid"         , 0);
    cape_udc_add_n      (values, "log"           , 0);
    cape_udc_add_n      (values, "refid"         , 0);

    cape_udc_add_n      (values, "p_data"        , 0);
    cape_udc_add_n      (values, "t_data"        , 0);

    /*

    proc_next_workstep_view

    select ws.id, ws.sqtid, ta.id taid, ta.wpid, ta.refid, ta.current_step, ws.name, ws.fctid, ws.log, ws.p_data, ta.t_data from proc_tasks ta join proc_worksteps ws on ws.wfid = ta.wfid and (ws.prev = ta.current_step or (ws.prev IS NULL and ta.current_step IS NULL));

    */
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_next_workstep_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_ext_first (query_results);

  if (first_row)
  {
    self->wsid = cape_udc_get_n (first_row, "id", 0);

    self->fctid = cape_udc_get_n (first_row, "fctid", 0);
    self->refid = cape_udc_get_n (first_row, "refid", 0);

    self->pdata_id = cape_udc_get_n (first_row, "p_data", 0);
    self->tdata_id = cape_udc_get_n (first_row, "t_data", 0);

    self->state = FLOW_STATE__NONE;
    self->log_type = cape_udc_get_n (first_row, "log", 0);

    res = flow_run_dbw__data_load (self, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  else
  {
    self->wsid = 0;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&first_row);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__current_task_load (FlowRunDbw self, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    // there is the option do run this with an admin role (self->wpid == 0)
    if (self->wpid)
    {
      cape_udc_add_n      (params, "wpid"          , self->wpid);
    }

    cape_udc_add_n      (params, "taid"          , self->psid);

    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "sqtid"         , 0);

    cape_udc_add_n      (values, "fctid"         , 0);
    cape_udc_add_n      (values, "log"           , 0);
    cape_udc_add_n      (values, "refid"         , 0);
    cape_udc_add_n      (values, "sync"          , 0);
    cape_udc_add_n      (values, "sync_cnt"      , 0);

    cape_udc_add_n      (values, "p_data"        , 0);
    cape_udc_add_n      (values, "t_data"        , 0);

    cape_udc_add_n      (values, "current_state" , 0);

    /*
    proc_current_workstep_view

    select ws.id, ws.sqtid, ta.id taid, ta.wpid, ws.wfid, ta.active, ta.refid, ta.sync, ts.cnt sync_cnt, ta.current_step, ta.current_state, ws.name, ws.fctid, ws.log, ws.p_data, ta.t_data from proc_tasks ta join proc_worksteps ws on ws.id = ta.current_step left join proc_task_sync ts on ts.taid = ta.id order by taid desc;
    */
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_current_workstep_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_ext_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "no current process step found");
    goto exit_and_cleanup;
  }

  self->wsid = cape_udc_get_n (first_row, "id", 0);
  self->sqid = cape_udc_get_n (first_row, "sqtid", 0);

  self->fctid = cape_udc_get_n (first_row, "fctid", 0);
  self->refid = cape_udc_get_n (first_row, "refid", 0);
  self->syncid = cape_udc_get_n (first_row, "sync", 0);
  self->syncnt = cape_udc_get_n (first_row, "sync_cnt", -1);
  self->syncmy = 0;

  self->pdata_id = cape_udc_get_n (first_row, "p_data", 0);
  self->tdata_id = cape_udc_get_n (first_row, "t_data", 0);

  self->state = cape_udc_get_n (first_row, "current_state", 0);
  self->log_type = cape_udc_get_n (first_row, "log", 0);

  res = flow_run_dbw__data_load (self, err);

exit_and_cleanup:

  cape_udc_del (&first_row);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__current_task_save (FlowRunDbw self, AdblTrx trx, CapeErr err)
{
  int res;

  // update tdata
  res = flow_run_dbw__tdata_update (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , self->psid);

    // important to update the current state
    cape_udc_add_n      (values, "current_state", self->state);

    if (self->tdata_id)
    {
      cape_udc_add_n      (values, "t_data"            , self->tdata_id);
    }

    // execute the query
    res = adbl_trx_update (trx, "proc_tasks", &params, &values, err);
  }

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

const CapeString flow_run_dbw_state_str (number_t state)
{
  switch (state)
  {
    case FLOW_STATE__NONE: return "FLOW_STATE__NONE";
    case FLOW_STATE__DONE: return "FLOW_STATE__DONE";
    case FLOW_STATE__HALT: return "FLOW_STATE__HALT";
    case FLOW_STATE__ERROR: return "FLOW_STATE__ERROR";
    case FLOW_STATE__ABORTED: return "FLOW_STATE__ABORTED";
    default: return "[UNKNOWN]";
  }
}

//-----------------------------------------------------------------------------

void flow_run_dbw_state_set (FlowRunDbw self, number_t state, CapeErr result_err)
{
  int res;

  // local objects
  CapeErr err = cape_err_new ();
  AdblTrx trx = NULL;

  // set the new state
  self->state = state;

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "run dbw", "set new state = %s", flow_run_dbw_state_str (self->state));

  if (result_err)
  {
    // override the state with error state
    self->state = FLOW_STATE__ERROR;

    cape_log_fmt (CAPE_LL_TRACE, "FLOW", "run dbw", "set state to error = %s", cape_err_text (result_err));

    CapeUdc error_node = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (error_node, "err_code", cape_err_code (result_err));
    cape_udc_add_s_cp (error_node, "err_text", cape_err_text (result_err));

    // add a special log entry, that this step was successfully set from outside
    res = flow_log_add (self->adbl_session, self->psid, self->wsid, 1, FLOW_STATE__ERROR, error_node, NULL, err);

    cape_udc_del (&error_node);

    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    goto exit_and_cleanup;
  }

  // save all variables to the database
  res = flow_run_dbw__current_task_save (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  adbl_trx_commit (&trx, err);

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

int flow_run_dbw__step__start (FlowRunDbw self, CapeErr err)
{
  int res;

  AdblTrx trx = NULL;

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , self->psid);
    cape_udc_add_n      (values, "current_step"  , self->wsid);

    // execute the query
    res = adbl_trx_update (trx, "proc_tasks", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = flow_log_add_trx (trx, self->psid, self->wsid, self->log_type, FLOW_STATE__INIT, self->tdata, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  adbl_trx_commit (&trx, err);

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__task_deactivate (FlowRunDbw self, AdblTrx trx, CapeErr err)
{
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n      (params, "id"            , self->psid);
  cape_udc_add_n      (values, "active"        , 0);

  // execute the query
  return adbl_trx_update (trx, "proc_tasks", &params, &values, err);
}

//-----------------------------------------------------------------------------

int flow_run_dbw__get_parent_process (FlowRunDbw self, number_t* p_psid, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , self->syncid);
    cape_udc_add_n      (values, "taid"          , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "proc_task_sync", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't find sync");
    goto exit_and_cleanup;
  }

  number_t psid = cape_udc_get_n (first_row, "taid", 0);
  if (0 == psid)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "taid is zero");
    goto exit_and_cleanup;
  }

  *p_psid = psid;
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  if (cape_err_code(err))
  {
    cape_log_msg (CAPE_LL_ERROR, "FLOW", "continue", cape_err_text (err));
  }

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__continue_parent_process (FlowRunDbw self, CapeErr err)
{
  int res;

  number_t psid = 0;

  res = flow_run_dbw__get_parent_process (self, &psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  {
    // create a new process chain
    FlowRunDbw dbw = flow_run_dbw_new (self->qbus, self->adbl_session, self->queue, self->jobs, self->wpid, psid, self->remote, self->rinfo, self->refid);

    res = flow_run_dbw_continue (&dbw, FLOW_STATE__HALT, NULL, err);
  }

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_sync__merge_tdata (AdblTrx trx, number_t syncid, CapeUdc tdata, CapeErr err)
{
  int res;

  CapeUdc first_row;
  CapeUdc pdata_db;
  CapeUdc tdata_db;
  const CapeString merge_node;
  CapeUdc merge_tdata_db;
  number_t tdid;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc merge_node_tdata = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , syncid);
    cape_udc_add_n      (values, "tdid"          , 0);
    cape_udc_add_node   (values, "tdata"         );
    cape_udc_add_node   (values, "pdata"         );

    // execute the query
    query_results = adbl_trx_query (trx, "flow_process_sync_tdata_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    // there is nothing to update
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }

  tdid = cape_udc_get_n (first_row, "tdid", 0);
  if (0 == tdid)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "pdid is zero");
    goto exit_and_cleanup;
  }

  pdata_db = cape_udc_get (first_row, "pdata");
  if (NULL == pdata_db)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "pdata is zero");
    goto exit_and_cleanup;
  }

  tdata_db = cape_udc_get (first_row, "tdata");
  if (NULL == tdata_db)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "tdata is zero");
    goto exit_and_cleanup;
  }

  // check for merge object
  merge_node = cape_udc_get_s (pdata_db, "merge_node", NULL);
  if (NULL == merge_node)
  {
    cape_log_msg (CAPE_LL_TRACE, "FLOW", "merge tdata", "there is no merge defined");

    // there is nothing to update
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "merge tdata", "found merge node = %s", merge_node);

  // try to find this node in tdata
  merge_node_tdata = cape_udc_ext (tdata, merge_node);
  if (NULL == merge_node_tdata)
  {
    cape_log_msg (CAPE_LL_TRACE, "FLOW", "merge tdata", "can't find merge node in TDATA");

    // there is nothing to update
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }

  merge_tdata_db = cape_udc_get (tdata_db, merge_node);
  if (merge_tdata_db)
  {
    cape_log_msg (CAPE_LL_TRACE, "FLOW", "merge tdata", "merge TDATA");

    // merge both TDATA's
    cape_udc_merge_mv (merge_tdata_db, &merge_node_tdata);
  }
  else
  {
    cape_log_msg (CAPE_LL_TRACE, "FLOW", "merge tdata", "add TDATA");

    // add as new entry to TDATA
    cape_udc_add_name (tdata_db, &merge_node_tdata, merge_node);
  }

  /*
  // debug
  {
    CapeString h = cape_json_to_s (tdata_db);

    printf ("MERGED TDATA: %s\n", h);

    cape_str_del (&h);
  }
   */

  res = flow_data_set (trx, tdid, tdata_db, err);

exit_and_cleanup:

  cape_udc_del (&query_results);
  cape_udc_del (&merge_node_tdata);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_done__save_to_database (FlowRunDbw self, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;

  // start transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // set the task as done
  res = flow_run_dbw__task_deactivate (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (self->syncid)
  {
    res = flow_run_dbw_sync__merge_tdata (trx, self->syncid, self->tdata, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&trx, err);

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_done__callback (FlowRunDbw self, CapeErr err)
{
  int res;

  CapeUdc first_row;

  // local objects
  CapeUdc query_results = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "psid"          , self->psid);
    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_s_cp   (values, "module"        , NULL);
    cape_udc_add_s_cp   (values, "method"        , NULL);
    cape_udc_add_node   (values, "params"        );

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_instance", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_get_first (query_results);
  if (first_row)
  {
    const CapeString module = cape_udc_get_s (first_row, "module", NULL);
    const CapeString method = cape_udc_get_s (first_row, "method", NULL);

    if (module && method)
    {
      QBusM msg = qbus_message_new (NULL, NULL);

      msg->rinfo = cape_udc_cp (self->rinfo);
      msg->cdata = cape_udc_ext (first_row, "params");

      if (msg->cdata == NULL)
      {
        msg->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
      }

      cape_udc_add_n (msg->cdata, "refid", self->refid);

      cape_log_fmt (CAPE_LL_TRACE, "FLOW", "callback", "found callback to module = %s, method = %s, refid = %lu", module, method, self->refid);

      qbus_send (self->qbus, module, method, msg, NULL, NULL, err);

      qbus_message_del (&msg);
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_done (FlowRunDbw self, CapeErr err)
{
  int res;

  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "run init", "------------------------------------------------------------");
  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "run init", " P R O C E S S   E N D E D                     ");
  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "run init", "------------------------------------------------------------");

  // save the current state and tdata to database
  res = flow_run_dbw_done__save_to_database (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (self->syncid)
  {
    number_t cnt;

    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

      cape_udc_add_n      (params, "id"            , self->syncid);

      // running an atomar query
      cnt = adbl_session_atomic_dec (self->adbl_session, "proc_task_sync", &params, "cnt", err);
    }

    // TODO: verify the correct synced ids were reduced !!!!
    if ((cape_err_code (err) == CAPE_ERR_NONE) && (cnt == 0))
    {
      res = flow_run_dbw__continue_parent_process (self, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    res = flow_run_dbw_done__callback (self, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = cape_err_set (err, CAPE_ERR_EOF, "end of processing chain");

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__state__init (FlowRunDbw self, CapeErr err)
{
  int res;

  // load initial data sets
  res = flow_run_dbw__next_load (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run set", "got initial processing step, wsid = %i", self->wsid);

  if (0 == self->wsid)
  {
    res = flow_run_dbw_done (self, err);

    // special case
    if (res == CAPE_ERR_EOF)
    {
      res = CAPE_ERR_NONE;
    }

    goto exit_and_cleanup;
  }

  res = flow_run_dbw__step__start (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__next (FlowRunDbw self, CapeErr err)
{
  int res;

  // reset some values
  //self->logid = 0;

  res = flow_run_dbw__next_load (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run next", "got next processing step, wsid = %i", self->wsid);

  if (self->wsid)
  {
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run next", "------------------------------------------------------------");
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run next", " N E X T   S T E P");
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run next", "------------------------------------------------------------");

    // update
    res = flow_run_dbw__step__start (self, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  else
  {
    res = flow_run_dbw_done (self, err);
  }

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__continue (FlowRunDbw self, CapeErr err)
{
  int res;

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "run set", "processing step [%i], wpid = %i, psid = %i", self->state, self->wpid, self->psid);

  switch (self->state)
  {
    case FLOW_STATE__INIT:
    {
      cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run set", "processing step [INIT], wpid = %i, psid = %i", self->wpid, self->psid);

      res = flow_run_dbw__state__init (self, err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      break;
    }
    case FLOW_STATE__NONE:
    {
      cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run set", "processing step [NONE], wpid = %i, psid = %i", self->wpid, self->psid);

      // load all variables from the current state of the database
      res = flow_run_dbw__current_task_load (self, err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run set", "got current processing step, wsid = %i", self->wsid);

      break;
    }
    case FLOW_STATE__DONE:
    {
      cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run set", "processing step [DONE], wpid = %i, psid = %i", self->wpid, self->psid);

      res = flow_run_dbw__next (self, err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      break;
    }
    case FLOW_STATE__HALT:
    {
      cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run set", "processing step [HALT], wpid = %i, psid = %i", self->wpid, self->psid);

      break;
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__run_step (FlowRunDbw* p_self, number_t action, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  FlowRunStep run_step = flow_run_step_new (self->qbus);

  // transfer ownership and business logic to step class
  // -> in return self might be NULL
  res = flow_run_step_set (&run_step, p_self, action, &(self->params), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self = *p_self;
  if (self)
  {
    // clear the input params
    cape_udc_del (&(self->params));

    //cape_log_msg (CAPE_LL_TRACE, "FLOW", "run next", "continue with next step");

    flow_run_dbw_state_set (self, FLOW_STATE__DONE, NULL);
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  self = *p_self;

  if (res && self)
  {
    if (res == CAPE_ERR_EOF)
    {
      // do nothing here
    }
    else if (res == CAPE_ERR_CONTINUE)
    {
      flow_run_dbw_state_set (self, FLOW_STATE__HALT, NULL);
    }
    else
    {
      flow_run_dbw_state_set (self, FLOW_STATE__ERROR, err);
    }
  }

  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL flow_run_dbw__queue_worker (void* ptr, number_t action, number_t queue_size)
{
  int res;
  FlowRunDbw self = ptr;

  // local objects
  CapeErr err = cape_err_new ();

  while (self)
  {
    res = flow_run_dbw__run_step (&self, action, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    if (self)
    {
      res = flow_run_dbw__next (self, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }

exit_and_cleanup:

  // cleanup
  flow_run_dbw_del (&self);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

int flow_run_dbw_start (FlowRunDbw* p_self, number_t action, CapeUdc* p_params, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  // explicit set the initialize state
  self->state = FLOW_STATE__INIT;

  // take over the input params
  if (p_params)
  {
    cape_udc_replace_mv (&(self->params), p_params);
  }
  else
  {
    cape_udc_del (&(self->params));
  }

  // load the current step
  res = flow_run_dbw__continue (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_queue_add (self->queue, NULL, flow_run_dbw__queue_worker, NULL, self, action);
  *p_self = NULL;

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  // cleanup
  flow_run_dbw_del (p_self);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_continue (FlowRunDbw* p_self, number_t action, CapeUdc* p_params, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  // take over the input params
  if (p_params)
  {
    cape_udc_replace_mv (&(self->params), p_params);
  }
  else
  {
    cape_udc_del (&(self->params));
  }

  // load the current step
  res = flow_run_dbw__continue (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // add process to queue
  cape_queue_add (self->queue, NULL, flow_run_dbw__queue_worker, NULL, self, action);

  // ownership was transfered to queue user ptr
  *p_self = NULL;

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  // cleanup
  flow_run_dbw_del (p_self);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_inherit (FlowRunDbw self, number_t wfid, number_t syncid, number_t refid, CapeUdc* p_params, CapeErr err)
{
  int res;

  number_t psid;

  // local objects
  FlowRunDbw dbw_cloned = flow_run_dbw_clone (self, self->psid, self->sqid, refid);
  AdblTrx trx = NULL;

  if (p_params)
  {
    flow_run_dbw_tdata__merge_to (dbw_cloned, p_params);
  }

  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // create a new process task
  psid = flow_run_dbw_init (dbw_cloned, trx, wfid, syncid, FALSE, err);
  if (0 == psid)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // commit all changes
  adbl_trx_commit (&trx, err);

  // transfer ownership for queuing
  // -> continue processing in background
  res = flow_run_dbw_start (&dbw_cloned, FLOW_ACTION__PRIM, NULL, err);

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_sqt__check (FlowRunDbw self, number_t sequence_id, number_t* p_wfid, CapeErr err)
{
  int res;

  CapeUdc first_row;
  number_t wfid;

  // local objects
  CapeUdc query_results = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , self->psid);
    cape_udc_add_n      (params, "sqtid"         , sequence_id);

    cape_udc_add_n      (values, "wfid"          , 0);

    /*
     --- flow_process_worksteps_view ---

     select ps.id, ps.wfid, ws.sqtid from proc_tasks ps join proc_worksteps ws on ws.wfid = ps.wfid where ws.prev IS NULL;
     */

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_process_worksteps_view", &params, &values, err);
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

  wfid = cape_udc_get_n (first_row, "wfid", 0);
  if (wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't find wfid");
    goto exit_and_cleanup;
  }

  *p_wfid = wfid;
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_sqt__syncid_children (FlowRunDbw self, number_t sequence_id, number_t from_child, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  FlowRunDbw dbw_cloned = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "psid_parent"   , self->psid);
    cape_udc_add_n      (params, "active"        , 1);

    cape_udc_add_n      (values, "psid"          , 0);

    /*
     --- flow_sync_childs_view ---

     select ts.taid psid_parent, ts.id syncid, ps.id psid, ps.active from proc_task_sync ts join proc_tasks ps on ps.sync = ts.id;
     */

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_sync_childs_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    number_t psid = cape_udc_get_n (cursor->item, "psid", 0);

    if (psid && (psid != from_child))
    {
      // clone the task with different psid and sequence ID
      dbw_cloned = flow_run_dbw_clone (self, psid, sequence_id, self->refid);

      // load all data for the parent process
      res = flow_run_dbw__current_task_load (dbw_cloned, err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      // abort the parent process
      res = flow_run_dbw_sqt (&dbw_cloned, sequence_id, 0, self->psid, NULL, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  flow_run_dbw_del (&dbw_cloned);

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_sqt (FlowRunDbw* p_self, number_t sequence_id, number_t from_child, number_t from_parent, CapeUdc* p_params, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  number_t psid = 0;
  number_t wfid = 0;

  // local objects
  FlowRunDbw dbw_cloned = NULL;
  AdblTrx trx = NULL;

  if (self->syncid && (from_parent == 0))
  {
    res = flow_run_dbw__get_parent_process (self, &psid, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    // clone the task with different psid and sequence ID
    dbw_cloned = flow_run_dbw_clone (self, psid, sequence_id, self->refid);

    // load all data for the parent process
    res = flow_run_dbw__current_task_load (dbw_cloned, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    // abort the parent process
    res = flow_run_dbw_sqt (&dbw_cloned, sequence_id, self->psid, 0, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  // run for each found syncid the abort mechanism
  res = flow_run_dbw_sqt__syncid_children (self, sequence_id, self->psid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // check if a workflow was defined with this sequence id
  res = flow_run_dbw_sqt__check (self, sequence_id, &wfid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (wfid)
  {
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run sqt", "change sequence ID of psid = %i and wfid = %i", self->psid, wfid);

    // clone the task with different sequence ID
    dbw_cloned = flow_run_dbw_clone (self, self->psid, sequence_id, self->refid);

    // apply extra params from input
    if (dbw_cloned->tdata)
    {
      if (p_params)
      {
        cape_udc_merge_mv (dbw_cloned->tdata, p_params);
      }
    }
    else
    {
      if (p_params)
      {
        dbw_cloned->tdata = cape_udc_mv (p_params);
      }
    }

    trx = adbl_trx_new (self->adbl_session, err);
    if (NULL == trx)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }

    // create a new process task
    psid = flow_run_dbw_init (dbw_cloned, trx, wfid, 0, FALSE, err);
    if (0 == psid)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }

    // commit all changes
    adbl_trx_commit (&trx, err);

    // transfer ownership for queuing
    // -> continue processing in background
    res = flow_run_dbw_start (&dbw_cloned, FLOW_ACTION__PRIM, NULL, err);
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

  res = flow_run_dbw__task_deactivate (self, trx, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // commit all changes
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);

  flow_run_dbw_state_set (self, FLOW_STATE__ABORTED, NULL);

  flow_run_dbw_del (&dbw_cloned);

  // cleanup
  flow_run_dbw_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_once (FlowRunDbw* p_self, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  // this shall load all current states from the database
  res = flow_run_dbw__continue (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self->state = FLOW_STATE__NONE;

  /*
  if (self->state == FLOW_STATE__HALT)
  {
    // this is OK
  }
  else
  {
    // workaround
    self->state = FLOW_STATE__HALT;
  }
   */

  // run the next step
  res = flow_run_dbw__run_step (p_self, FLOW_ACTION__PRIM, err);

exit_and_cleanup:

  // cleanup
  flow_run_dbw_del (p_self);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_next (FlowRunDbw* p_self, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  // this shall load all current states from the database
  res = flow_run_dbw__continue (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = flow_run_dbw__next (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // continue in background process
  cape_queue_add (self->queue, NULL, flow_run_dbw__queue_worker, NULL, self, FLOW_ACTION__PRIM);

  // transfer ownership to background process
  *p_self = NULL;

exit_and_cleanup:

  // cleanup
  flow_run_dbw_del (p_self);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_set (FlowRunDbw* p_self, number_t action, CapeUdc* p_params, CapeErr err)
{
  int res;
  FlowRunDbw self = *p_self;

  // take over the input params
  if (p_params)
  {
    cape_udc_replace_mv (&(self->params), p_params);
  }
  else
  {
    cape_udc_del (&(self->params));
  }

  // this shall load all current states from the database
  res = flow_run_dbw__continue (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (self->state == FLOW_STATE__HALT)
  {
    // this is OK
  }
  else
  {
    // workaround
    self->state = FLOW_STATE__HALT;
  }

  // run the next step
  res = flow_run_dbw__run_step (p_self, action, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self = *p_self;

  if (self)
  {
    // add a special log entry, that this step was successfully set from outside
    res = flow_log_add (self->adbl_session, self->psid, self->wsid, self->log_type, FLOW_STATE__SET, NULL, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    // continue with the next step
    res = flow_run_dbw__next (self, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    // continue in background process
    cape_queue_add (self->queue, NULL, flow_run_dbw__queue_worker, NULL, self, action);

    // transfer ownership to background process
    *p_self = NULL;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  // its normal that the process might be ended
  if (res == CAPE_ERR_EOF)
  {
    res = CAPE_ERR_NONE;
    cape_err_clr (err);
  }

  if (res == CAPE_ERR_CONTINUE)
  {
    res = CAPE_ERR_NONE;
    cape_err_clr (err);
  }

  // cleanup
  flow_run_dbw_del (p_self);

  return res;
}

//-----------------------------------------------------------------------------

number_t flow_run_dbw_state_get (FlowRunDbw self)
{
  return self->state;
}

//-----------------------------------------------------------------------------

number_t flow_run_dbw_fctid_get (FlowRunDbw self)
{
  return self->fctid;
}

//-----------------------------------------------------------------------------

number_t flow_run_dbw_synct_get (FlowRunDbw self)
{
  return self->syncnt;
}

//-----------------------------------------------------------------------------

CapeUdc flow_run_dbw_rinfo_get (FlowRunDbw self)
{
  return self->rinfo;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_pdata__qbus (FlowRunDbw self, CapeString* p_module, CapeString* p_method, CapeUdc* p_cdata, CapeUdc* p_clist, CapeErr err)
{
  int res;

  const CapeString precondition_node_name = NULL;
  CapeUdc precondition_node = NULL;

  // local objects
  CapeString module = NULL;
  CapeString method = NULL;

  // objects needed to call remote QBUS method
  CapeUdc cdata = NULL;
  CapeUdc clist = NULL;

  if (self->pdata)
  {
    module = cape_udc_ext_s (self->pdata, "module");
    method = cape_udc_ext_s (self->pdata, "method");

    // check for clist
    clist = cape_udc_ext (self->pdata, "clist");

    // check for cdata
    cdata = cape_udc_ext (self->pdata, "cdata");

    if ((cdata == NULL) && (clist == NULL))
    {
      // support params
      CapeUdc params = cape_udc_ext (self->pdata, "params");
      if (params)
      {
        // check for clist
        clist = cape_udc_ext (params, "clist");

        // check for cdata
        cdata = cape_udc_ext (params, "cdata");

        if (cdata == NULL)
        {
          // old way
          cape_udc_replace_mv (&cdata, &params);
        }

        cape_udc_del (&params);
      }
    }

    // that's optional
    precondition_node_name = cape_udc_get_s (self->pdata, "precondition_node", NULL);
  }

  if (module == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'module' is empty or missing");
    goto exit_and_cleanup;
  }

  if (method == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'method' is empty or missing");
    goto exit_and_cleanup;
  }

  // check for the list in t_data
  if (self->tdata)
  {
    // optional
    if (precondition_node_name)
    {
      precondition_node = cape_udc_get (self->tdata, precondition_node_name);
    }
  }

  // check precondition
  if (precondition_node)
  {
    switch (cape_udc_type (precondition_node))
    {
      case CAPE_UDC_LIST:
      case CAPE_UDC_NODE:
      {
        if (cape_udc_size (precondition_node) == 0)
        {
          res = CAPE_ERR_EOF;
          goto exit_and_cleanup;
        }

        break;
      }
    }
  }

  if (cdata == NULL)
  {
    cape_log_fmt (CAPE_LL_TRACE, "FLOW", "dbw pdata", "module = %s, method = %s, params = none", module, method);

    cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  else
  {
    CapeString h = cape_json_to_s (cdata);

    cape_log_fmt (CAPE_LL_TRACE, "FLOW", "dbw pdata", "module = %s, method = %s, params = %s", module, method, h);

    cape_str_del (&h);
  }

  res = CAPE_ERR_NONE;

  cape_str_replace_mv (p_module, &module);
  cape_str_replace_mv (p_method, &method);

  cape_udc_replace_mv (p_cdata, &cdata);
  cape_udc_replace_mv (p_clist, &clist);

exit_and_cleanup:

  cape_str_del (&module);
  cape_str_del (&method);

  cape_udc_del (&cdata);
  cape_udc_del (&clist);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_xdata__split (FlowRunDbw self, CapeUdc* p_list, number_t* p_wfid, CapeString* p_refid_name, CapeErr err)
{
  int res;

  // names of objects in the user data
  const CapeString list_node_name = NULL;
  const CapeString refid_node_name = NULL;

  number_t wfid = 0;

  CapeUdc list = NULL;

  if (self->pdata)
  {
    list_node_name = cape_udc_get_s (self->pdata, "list_node", NULL);
    refid_node_name = cape_udc_get_s (self->pdata, "refid_node", NULL);

    wfid = cape_udc_get_n (self->pdata, "wfid", 0);
  }

  if (list_node_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'split_list' is empty or missing");
    goto exit_and_cleanup;
  }

  if (wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'wfid' is empty or missing");
    goto exit_and_cleanup;
  }

  // check for the list in t_data
  if (self->tdata)
  {
    list = cape_udc_ext (self->tdata, list_node_name);
  }

  if (list == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't find the list defined by 'split_list'");
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

  if (refid_node_name)
  {
    cape_str_replace_cp (p_refid_name, refid_node_name);
  }

  cape_udc_replace_mv (p_list, &list);
  *p_wfid = wfid;

exit_and_cleanup:

  cape_udc_del (&list);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_xdata__switch (FlowRunDbw self, CapeString* p_value, CapeUdc* p_switch_node, CapeErr err)
{
  int res;

  // names of objects in the user data
  const CapeString value_node_name = NULL;
  const CapeString refid_node_name = NULL;

  const CapeString precondition_node_name = NULL;

  CapeUdc value_node = NULL;
  CapeUdc precondition_node = NULL;

  // local objects
  CapeString value = NULL;
  CapeUdc switch_node = NULL;

  if (self->pdata)
  {
    value_node_name = cape_udc_get_s (self->pdata, "value_node", NULL);
    //refid_node_name = cape_udc_get_s (self->pdata, "refid_node", NULL);

    switch_node = cape_udc_ext (self->pdata, "switch");

    // that's optional
    precondition_node_name = cape_udc_get_s (self->pdata, "precondition_node", NULL);
  }

  if (value_node_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'value_node' is empty or missing");
    goto exit_and_cleanup;
  }

  if (switch_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'switch' is empty or missing");
    goto exit_and_cleanup;
  }

  // check for the list in t_data
  if (self->tdata)
  {
    value_node = cape_udc_get (self->tdata, value_node_name);

    // optional
    if (precondition_node_name)
    {
      precondition_node = cape_udc_get (self->tdata, precondition_node_name);
    }
  }

  if (value_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'value_node' is empty or missing");
    goto exit_and_cleanup;
  }

  // check precondition
  if (precondition_node)
  {
    switch (cape_udc_type (precondition_node))
    {
      case CAPE_UDC_LIST:
      case CAPE_UDC_NODE:
      {
        if (cape_udc_size (precondition_node) == 0)
        {
          res = CAPE_ERR_NONE;

          p_value = NULL;
          p_switch_node = NULL;

          goto exit_and_cleanup;
        }

        break;
      }
    }
  }

  switch (cape_udc_type (value_node))
  {
    case CAPE_UDC_STRING:
    {
      value = cape_str_cp (cape_udc_s (value_node, NULL));
      break;
    }
    case CAPE_UDC_NUMBER:
    {
      value = cape_str_n (cape_udc_n (value_node, 0));
      break;
    }
    case CAPE_UDC_FLOAT:
    {
      value = cape_str_f (cape_udc_f (value_node, .0));
      break;
    }
  }

  res = CAPE_ERR_NONE;

  cape_str_replace_mv (p_value, &value);
  cape_udc_replace_mv (p_switch_node, &switch_node);

exit_and_cleanup:

  cape_str_del (&value);
  cape_udc_del (&switch_node);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_xdata__if (FlowRunDbw self, CapeUdc* p_value_node, number_t* p_wfid, CapeErr err)
{
  int res;

  const CapeString variable_node_name = NULL;
  const CapeString params_node_name = NULL;

  CapeUdc seek_node = NULL;

  // to be on the safe side
  *p_value_node = NULL;
  *p_wfid = 0;

  if (self->pdata)
  {
    variable_node_name = cape_udc_get_s (self->pdata, "value_node", NULL);
    *p_wfid = cape_udc_get_n (self->pdata, "wfid", 0);

    // optional
    params_node_name = cape_udc_get_s (self->pdata, "params_node", NULL);
  }

  if (*p_wfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'wfid' is empty or missing");
    goto exit_and_cleanup;
  }

  if (variable_node_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'value_node' is empty or missing");
    goto exit_and_cleanup;
  }

  if (params_node_name)
  {
    if (self->tdata)
    {
      seek_node = cape_udc_get (self->tdata, params_node_name);
    }
  }
  else
  {
    seek_node = self->tdata;
  }

  if (seek_node)
  {
    *p_value_node = cape_udc_get (seek_node, variable_node_name);
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_pdata__logs_merge (FlowRunDbw self, CapeUdc params, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;
  int add_logs = FALSE;

  // start transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  if (self->pdata)
  {
    CapeUdc add_logs_node = cape_udc_get (self->pdata, "add_logs");

    if (add_logs_node)
    {
      add_logs = cape_udc_b (add_logs_node, cape_udc_n (add_logs_node, FALSE));
    }
  }

  // add logs
  if (add_logs)
  {
    CapeErr err = cape_err_new ();

    CapeUdc logs = cape_udc_new (CAPE_UDC_LIST, NULL);

    int res = flow_chain_get__run (trx, self->psid, logs, err);
    if (res)
    {
      cape_log_fmt (CAPE_LL_ERROR, "FLOW", "logs merge", "can't fetch logs: %s", cape_err_text (err));
    }
    else
    {
      cape_udc_add_name (params, &logs, "logs");
    }

    cape_udc_del (&logs);

    cape_err_del (&err);
  }

  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

void flow_run_dbw_tdata__merge_in (FlowRunDbw self, CapeUdc params)
{
  if (self->tdata)
  {
    // merge params
    cape_udc_merge_cp (params, self->tdata);
  }
}

//-----------------------------------------------------------------------------

void flow_run_dbw_tdata__merge_to (FlowRunDbw self, CapeUdc* p_params)
{
  if (self->tdata)
  {
    CapeUdc params = *p_params;
    if (params)
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (self->tdata, CAPE_DIRECTION_FORW);

      while (cape_udc_cursor_next (cursor))
      {
        CapeUdc h2 = cape_udc_get (params, cape_udc_name (cursor->item));
        if (h2 == NULL)
        {
          CapeUdc h1 = cape_udc_cursor_ext (self->tdata, cursor);
          cape_udc_add (params, &h1);
        }
      }

      cape_udc_cursor_del (&cursor);

      cape_udc_replace_mv (&(self->tdata), p_params);
    }
  }
  else
  {
    self->tdata = cape_udc_mv (p_params);
  }
}

//-----------------------------------------------------------------------------

CapeUdc flow_run_dbw__retrieve_var (CapeUdc node, const CapeString name)
{
  // try to split the name into its parts separated by the '.' character
  CapeList name_parts = cape_tokenizer_buf (name, cape_str_size (name), '.');

  CapeUdc current_node = node;

  CapeListCursor* cursor = cape_list_cursor_create (name_parts, CAPE_DIRECTION_FORW);

  while (current_node && cape_list_cursor_next (cursor))
  {
    const CapeString part_name = cape_list_node_data (cursor->node);

    current_node = cape_udc_get (current_node, part_name);
  }

  cape_list_cursor_destroy (&cursor);
  cape_list_del (&name_parts);

  return current_node;
}

//-----------------------------------------------------------------------------

int flow_run_dbw__udc_path__split (const CapeString var_name, CapeString* p_node, CapeString* p_var)
{
  return cape_tokenizer_split_last (var_name, '.', p_node, p_var);
}

//-----------------------------------------------------------------------------

int flow_run_dbw_xdata__var_copy (FlowRunDbw self, CapeErr err)
{
  int res;

  const CapeString var_name;
  const CapeString new_name;
  CapeUdc var_node;

  // local objects
  CapeUdc var_copy = NULL;

  if (NULL == self->pdata)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "pdata is empty");
    goto exit_and_cleanup;
  }

  if (NULL == self->tdata)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "tdata is empty");
    goto exit_and_cleanup;
  }

  var_name = cape_udc_get_s (self->pdata, "var_name", NULL);
  if (NULL == var_name)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'var_name' is empty or missing");
    goto exit_and_cleanup;
  }

  new_name = cape_udc_get_s (self->pdata, "new_name", NULL);
  if (NULL == new_name)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'new_name' is empty or missing");
    goto exit_and_cleanup;
  }

  var_node = cape_udc_get (self->tdata, var_name);
  if (NULL == var_node)
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "variable '%s' was not found", var_name);
    goto exit_and_cleanup;
  }

  // create the copy
  var_copy = cape_udc_cp (var_node);

  // add with new name
  cape_udc_add_name (self->tdata, &var_copy, new_name);

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_del (&var_copy);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_xdata__var_move (FlowRunDbw self, CapeErr err)
{
  int res;

  const CapeString var_name;
  const CapeString new_name;

  CapeUdc var_node_node;
  CapeUdc var_node_name;
  CapeUdc new_node_node;
  CapeUdc new_node_name;

  // local objects
  CapeString var_name_node = NULL;
  CapeString var_name_name = NULL;
  CapeString new_name_node = NULL;
  CapeString new_name_name = NULL;

  if (NULL == self->pdata)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "pdata is empty");
    goto exit_and_cleanup;
  }

  if (NULL == self->tdata)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "tdata is empty");
    goto exit_and_cleanup;
  }

  var_name = cape_udc_get_s (self->pdata, "var_name", NULL);
  if (NULL == var_name)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'var_name' is empty or missing");
    goto exit_and_cleanup;
  }

  new_name = cape_udc_get_s (self->pdata, "new_name", NULL);
  if (NULL == new_name)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'new_name' is empty or missing");
    goto exit_and_cleanup;
  }

  if (flow_run_dbw__udc_path__split (var_name, &var_name_node, &var_name_name))
  {
    // convert from name into node
    var_node_node = flow_run_dbw__retrieve_var (self->tdata, var_name_node);
    if (var_node_node == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "variable can't be found");
      goto exit_and_cleanup;
    }

    // convert from name into node
    var_node_name = cape_udc_ext (var_node_node, var_name_name);
  }
  else
  {
    // convert from name into node
    var_node_name = cape_udc_ext (self->tdata, var_name);
  }

  if (var_node_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "variable can't be found");
    goto exit_and_cleanup;
  }

  if (flow_run_dbw__udc_path__split (new_name, &new_name_node, &new_name_name))
  {
    new_node_node = flow_run_dbw__retrieve_var (self->tdata, new_name_node);

    // TODO: consideration to create the missing node
    if (new_node_node == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "destination can't be found");
      goto exit_and_cleanup;
    }

    cape_udc_add_name (new_node_node, &var_node_name, new_name_name);
  }
  else
  {
    cape_udc_add_name (self->tdata, &var_node_name, new_name);
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_str_del (&var_name_node);
  cape_str_del (&var_name_name);
  cape_str_del (&new_name_node);
  cape_str_del (&new_name_name);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_xdata__create_node (FlowRunDbw self, CapeErr err)
{
  int res;

  // names of objects in the user data
  const CapeString node_name = NULL;
  CapeUdc variables;
  CapeUdc varsmv;

  // local objects
  CapeUdc node_create = NULL;
  CapeUdcCursor* cursor = NULL;

  if (self->pdata)
  {
    node_name = cape_udc_get_s (self->pdata, "node_name", NULL);
    variables = cape_udc_get (self->pdata, "variables");
    varsmv = cape_udc_get (self->pdata, "varsmv");
  }

  if (node_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'node_name' is empty or missing");
    goto exit_and_cleanup;
  }

  if (self->tdata == NULL)
  {
    self->tdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  else
  {
    // check if the node already exists
    node_create = cape_udc_ext (self->tdata, node_name);
  }

  if (node_create == NULL)
  {
    node_create = cape_udc_new (CAPE_UDC_NODE, node_name);
  }

  if (variables)
  {
    switch (cape_udc_type (variables))
    {
      case CAPE_UDC_LIST:
      {
        cursor = cape_udc_cursor_new (variables, CAPE_DIRECTION_FORW);

        // try to find all variables
        while (cape_udc_cursor_next (cursor))
        {
          const CapeString name = cape_udc_s (cursor->item, NULL);
          if (name)
          {
            // check if this variable exists in tdata
            CapeUdc var = cape_udc_get (self->tdata, name);
            if (var == NULL)
            {
              res = cape_err_set_fmt (err, CAPE_ERR_MISSING_PARAM, "parameter '%s' is empty or missing", name);
              goto exit_and_cleanup;
            }

            {
              CapeUdc clone = cape_udc_cp (var);
              cape_udc_add (node_create, &clone);
            }
          }
        }

        break;
      }
      case CAPE_UDC_NODE:
      {

        break;
      }
    }
  }

  if (varsmv)
  {
    switch (cape_udc_type (varsmv))
    {
      case CAPE_UDC_NODE:
      {
        cursor = cape_udc_cursor_new (varsmv, CAPE_DIRECTION_FORW);

        // try to find all variables
        while (cape_udc_cursor_next (cursor))
        {
          // check if this variable exists in tdata
          CapeUdc var = cape_udc_ext (self->tdata, cape_udc_name (cursor->item));
          if (var)
          {
            cape_udc_add_name (node_create, &var, cape_udc_s (cursor->item, "var"));
          }
        }

        break;
      }
    }
  }

  // finally add the node
  cape_udc_add (self->tdata, &node_create);

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_udc_cursor_del (&cursor);

  cape_udc_del (&node_create);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_wait__init (FlowRunDbw self, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;

  const CapeString waitlist_node_name = NULL;
  const CapeString code_node_name = NULL;

  CapeUdc waitlist_node = NULL;
  CapeUdc code_node = NULL;

  // local objects
  CapeUdcCursor* cursor = NULL;
  CapeUdc values = NULL;

  if (self->pdata)
  {
    waitlist_node_name = cape_udc_get_s (self->pdata, "waitlist_node", NULL);

    // optional
    code_node_name = cape_udc_get_s (self->pdata, "code_node", NULL);
  }

  if (waitlist_node_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'waitlist_node' is empty or missing");
    goto exit_and_cleanup;
  }

  // check for the list in t_data
  if (self->tdata)
  {
    /*
    {
      CapeString h = cape_json_to_s (self->tdata);

      printf ("TDATA: %s\n", h);

      cape_str_del (&h);
    }
     */

    waitlist_node = cape_udc_get (self->tdata, waitlist_node_name);

    if (code_node_name)
    {
      // optional
      code_node = flow_run_dbw__retrieve_var (self->tdata, code_node_name);
    }
  }

  if (waitlist_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't find the list defined by 'waitlist_node'");
    goto exit_and_cleanup;
  }

  /*
  {
    CapeString h = cape_json_to_s (waitlist_node);

    printf ("WAITLIST: %s\n", h);

    cape_str_del (&h);
  }
   */

  // start transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  cursor = cape_udc_cursor_new (waitlist_node, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    number_t id;

    values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (values, "psid"          , self->psid);
    cape_udc_add_n      (values, "status"        , 0);

    switch (cape_udc_type (cursor->item))
    {
      case CAPE_UDC_STRING:
      {
        const CapeString uuid = cape_udc_s (cursor->item, NULL);
        if (uuid == NULL)
        {
          res = cape_err_set (err, CAPE_ERR_RUNTIME, "item of 'waitlist_node' has no uuid entry");
          goto exit_and_cleanup;
        }

        cape_udc_add_s_cp (values, "uuid", uuid);
        break;
      }
      case CAPE_UDC_NODE:
      {
        const CapeString uuid = cape_udc_get_s (cursor->item, "uuid", NULL);
        if (uuid == NULL)
        {
          res = cape_err_set (err, CAPE_ERR_RUNTIME, "item of 'waitlist_node' has no uuid entry");
          goto exit_and_cleanup;
        }

        cape_udc_add_s_cp (values, "uuid", uuid);
        break;
      }
      default:
      {
        res = cape_err_set (err, CAPE_ERR_RUNTIME, "type of 'waitlist_node' is not supported");
        goto exit_and_cleanup;

        break;
      }
    }

    if (code_node)
    {
      switch (cape_udc_type (code_node))
      {
        case CAPE_UDC_NUMBER:
        {
          // create a string from the number
          CapeString code = cape_str_n (cape_udc_n (code_node, 0));
          cape_udc_add_s_mv (values, "code", &code);

          break;
        }
        case CAPE_UDC_STRING:
        {
          const CapeString code = cape_udc_s (code_node, NULL);
          if (code)
          {
            cape_udc_add_s_cp (values, "code", code);
          }

          break;
        }
      }
    }

    // execute the query
    id = adbl_trx_insert (trx, "flow_wait_items", &values, err);
    if (0 == id)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  // always halt the process
  res = cape_udc_size (waitlist_node) == 0 ? CAPE_ERR_NONE : CAPE_ERR_CONTINUE;
  adbl_trx_commit (&trx, err);

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&values);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_wait__check_item (FlowRunDbw self, const CapeString uuid, const CapeString code, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;

  number_t missing_waits = 0;
  number_t itemid = 0;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "psid"          , self->psid);

    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "status"        , 0);
    cape_udc_add_s_cp   (values, "uuid"          , NULL);
    cape_udc_add_s_cp   (values, "code"          , NULL);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_wait_items", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow wait", "wait items for process = %i and psid = %i", cape_udc_size (query_results), self->psid);

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    const CapeString uuid_item = cape_udc_get_s (cursor->item, "uuid", NULL);
    const CapeString code_item = cape_udc_get_s (cursor->item, "code", NULL);

    number_t status = cape_udc_get_n (cursor->item, "status", 0);

    if (cape_str_equal (uuid_item, uuid))
    {
      cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow wait", "match uuid = '%s'", uuid);

      if (status)
      {
        res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "already set");
        goto exit_and_cleanup;
      }

      if (code_item)
      {
        if (!cape_str_equal (code_item, code))
        {
          cape_log_fmt (CAPE_LL_ERROR, "FLOW", "check item", "code missmatch: '%s' <> '%s'", code_item, code);

          res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "code missmatch");
          goto exit_and_cleanup;
        }
      }

      missing_waits++;
      itemid = cape_udc_get_n (cursor->item, "id", 0);
    }
    else
    {
      if (status == 0)
      {
        // is still missing
        missing_waits++;
      }
    }
  }

  // start transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  if (itemid)
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "id"            , itemid);
    cape_udc_add_n      (values, "status"        , 1);

    // update status
    res = adbl_trx_update (trx, "flow_wait_items", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow wait", "set new status");
    missing_waits--;
  }

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow wait", "wait items = %i", missing_waits);

  // commit everything
  adbl_trx_commit (&trx, err);

  // set the final result
  res = missing_waits == 0 ? CAPE_ERR_NONE : CAPE_ERR_CONTINUE;

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_sleep__init (FlowRunDbw self, CapeErr err)
{
  int res;

  number_t wait_in_seconds = 0;
  CapeDatetime dt_current;
  CapeDatetime dt_event;

  // local objects
  CapeUdc params = NULL;
  CapeUdc rinfo = NULL;

  if (self->pdata)
  {
    wait_in_seconds = cape_udc_get_n (self->pdata, "sec", 0);
  }

  if (wait_in_seconds == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'ms' is empty or missing");
    goto exit_and_cleanup;
  }

  params = cape_udc_new (CAPE_UDC_NODE, NULL);
  rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n (rinfo, "wpid", self->wpid);

  // get current datetime
  cape_datetime_utc (&dt_current);

  // add milliseconds
  cape_datetime__add_n (&dt_current, wait_in_seconds, &dt_event);

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow sleep", "wait for = %lu seconds", wait_in_seconds);

  // create a new timed job
  // the event will be handled in the main flow.c
  res = qjobs_add (self->jobs, &dt_event, 0, &params, rinfo, "FLOW", NULL, self->psid, self->wsid, "flowsecret", err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // always return to set the step to none continue
  res = CAPE_ERR_CONTINUE;

exit_and_cleanup:

  cape_udc_del (&params);
  cape_udc_del (&rinfo);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_dbw_condition (FlowRunDbw self)
{
  if (self->pdata)
  {
    const CapeString condition = cape_udc_get_s (self->pdata, "cond", NULL);

    if (cape_str_not_empty (condition))
    {
      if (self->tdata)
      {
        int ret, res;
        CapeErr err = cape_err_new ();

        res = cape_eval_b (condition, self->tdata, &ret, NULL, err);

        printf ("COND: %i\n", ret);

        cape_err_del (&err);

        if (res)
        {
          return TRUE;
        }

        return ret;
      }
    }
  }

  return TRUE;
}

//-----------------------------------------------------------------------------

number_t flow_run_dbw_sync__add (FlowRunDbw self, number_t cnt, CapeErr err)
{
  // local objects
  AdblTrx trx = NULL;

  // start transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    goto exit_and_cleanup;
  }

  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (values, "taid"      , self->psid);
    cape_udc_add_n (values, "wsid"      , self->wsid);
    cape_udc_add_n (values, "cnt"       , cnt);

    // execute the query
    self->syncmy = adbl_trx_insert (trx, "proc_task_sync", &values, err);
  }

  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "flow run", "sync object created: %i", self->syncmy);

  // commit everything
  adbl_trx_commit (&trx, err);

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  return self->syncmy;
}

//-----------------------------------------------------------------------------

int flow_log_add_trx (AdblTrx trx, number_t psid, number_t wsid, number_t log_type, number_t state, CapeUdc data, const CapeString vsec, CapeErr err)
{
  int res;

  // local objects
  CapeString h1 = NULL;

  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "log add", "log type = %i", log_type);

  switch (log_type)
  {
    case 1:   // log everything
    {
      number_t id;

      cape_log_fmt (CAPE_LL_TRACE, "FLOW", "log add", "add log entry for psid = %i, wsid = %i", psid, wsid);

      if (data)
      {
        h1 = cape_json_to_s (data);
        if (h1 == NULL)
        {
          res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialze data");
          goto exit_and_cleanup;
        }
      }

      {
        CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

        cape_udc_add_n (values, "psid"      , psid);
        cape_udc_add_n (values, "wsid"      , wsid);

        {
          CapeDatetime dt; cape_datetime_utc (&dt);
          cape_udc_add_d (values, "pot", &dt);
        }

        cape_udc_add_n (values, "state"     , state);

        if (h1)
        {
          cape_udc_add_s_mv (values, "data", &h1);
        }

        // execute the query
        id = adbl_trx_insert (trx, "flow_log", &values, err);
        if (id == 0)
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
      }

      break;
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_str_del (&h1);

  return res;
}

//-----------------------------------------------------------------------------

int flow_log_add (AdblSession adbl_session, number_t psid, number_t wsid, number_t log_type, number_t log_action, CapeUdc data, const CapeString vsec, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;

  // start transaction
  trx = adbl_trx_new (adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = flow_log_add_trx (trx, psid, wsid, log_type, log_action, data, vsec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // commit everything
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------
