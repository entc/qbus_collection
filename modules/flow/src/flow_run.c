#include "flow_run.h"
#include "flow_run_dbw.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>

// qflow includes
#include <qflow.h>

//-----------------------------------------------------------------------------

struct FlowRun_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  CapeQueue queue;             // reference
  
  number_t wpid;               // workspace id
  
  CapeString name;
  CapeString module;
  CapeString method;

  CapeUdc params;
  CapeUdc rinfo;
  
  number_t raid;
  
  CapeStopTimer st;
};

//-----------------------------------------------------------------------------

FlowRun flow_run_new (QBus qbus, AdblSession adbl_session, CapeQueue queue)
{
  FlowRun self = CAPE_NEW(struct FlowRun_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->queue = queue;
  
  self->wpid = 0;
  
  self->module = NULL;
  self->method = NULL;
  
  self->params = NULL;
  self->rinfo = NULL;
  
  self->raid = 0;
  self->st = cape_stoptimer_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void flow_run_del (FlowRun* p_self)
{
  if (*p_self)
  {
    FlowRun self = *p_self;
    
    cape_str_del (&(self->module));
    cape_str_del (&(self->method));
    
    cape_udc_del (&(self->params));
    cape_udc_del (&(self->rinfo));
    
    cape_stoptimer_del (&(self->st));

    CAPE_DEL (p_self, struct FlowRun_s);
  }
}

//-----------------------------------------------------------------------------

int flow_run_add__create_run_in_database (FlowRun self, CapeErr err)
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

  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n     (values, "wpid"      , self->wpid);
    cape_udc_add_n     (values, "state"     , QFLOW_STATE_INITIAL);       // started

    cape_udc_add_s_cp  (values, "fdesc"     , self->name);
    
    {
      CapeDatetime dt; cape_datetime_utc (&dt);
      
      cape_udc_add_d   (values, "created"   , &dt);
    }

    cape_udc_add_s_cp  (values, "module"    , self->module);
    cape_udc_add_s_cp  (values, "method"    , self->method);

    // execute the query
    self->raid = adbl_trx_insert (trx, "flow_run", &values, err);
  }

  res = CAPE_ERR_NONE;
  
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_add__complete_run_in_database (FlowRun self, CapeUdc rdata, CapeErr err_run, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx trx = NULL;
  number_t rdata_id = 0;
  
  trx = adbl_trx_new (self->adbl_session, err);
  if (NULL == trx)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // only store the result in case there was no error
  if (rdata && (err_run == NULL))
  {
    rdata_id = flow_data_add (trx, rdata, err);
    if (0 == rdata_id)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (params, "id"        , self->raid);
    cape_udc_add_n (params, "wpid"      , self->wpid);

    if (err_run)
    {
      cape_udc_add_n    (values, "state"     , QFLOW_STATE_ERROR);

      cape_udc_add_n    (values, "err_code"  , cape_err_code (err_run));
      cape_udc_add_s_cp (values, "err_text"  , cape_err_text (err_run));
    }
    else
    {
      cape_udc_add_n    (values, "state"     , QFLOW_STATE_COMPLETE);
    }
    
    if (rdata_id)
    {
      cape_udc_add_n (values, "rdata"   , rdata_id);
    }

    cape_udc_add_f (values, "time_passed", cape_stoptimer_get (self->st));

    // execute the query
    res = adbl_trx_update (trx, "flow_run", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "run complete", "can't update 'flow_run': %s", cape_err_text (err));
  }
  
  adbl_trx_rollback (&trx, err);
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL flow_run__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowRun self = ptr;
  
  cape_stoptimer_stop (self->st);
  
  res = flow_run_add__complete_run_in_database (self, qin->cdata, qin->err, err);
  if (res)
  {
    
  }

exit_and_cleanup:
  
  flow_run_del (&self);
  
  cape_err_clr (err);
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL flow_run__queue_worker (void* ptr, number_t action, number_t queue_size)
{
  int res;
  FlowRun self = ptr;

  // local objects
  CapeErr err = cape_err_new ();
  QBusM msg = NULL;
  
  cape_stoptimer_start (self->st);
  
  msg = qbus_message_new (NULL, NULL);
  
  cape_udc_replace_mv (&(msg->rinfo), &(self->rinfo));
  cape_udc_replace_mv (&(msg->cdata), &(self->params));
  
  if (NULL == msg->cdata)
  {
    msg->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  
  cape_udc_add_n (msg->cdata, "raid", self->raid);
  
  res = qbus_send (self->qbus, self->module, self->method, msg, self, flow_run__on_call, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "flow run", "runtime error: %s", cape_err_text (err));

  }
  
  qbus_message_del (&msg);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

int flow_run_add (FlowRun* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowRun self = *p_self;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing wpid");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_run_add} missing cdata");
    goto exit_and_cleanup;
  }

  self->module = cape_udc_ext_s (qin->cdata, "module");
  if (NULL == self->module)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_run_add} missing module");
    goto exit_and_cleanup;
  }
  
  self->method = cape_udc_ext_s (qin->cdata, "method");
  if (NULL == self->method)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_run_add} missing method");
    goto exit_and_cleanup;
  }
  
  self->name = cape_udc_ext_s (qin->cdata, "fdesc");
  if (NULL == self->name)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_run_add} missing fdesc");
    goto exit_and_cleanup;
  }

  // use the input as params
  cape_udc_replace_mv (&(self->params), &(qin->cdata));
  
  // copy rinfo
  self->rinfo = cape_udc_cp (qin->rinfo);
  
  // create a new entry in the database
  res = flow_run_add__create_run_in_database (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // add to queue
  cape_queue_add (self->queue, NULL, flow_run__queue_worker, NULL, NULL, self, 0);
  *p_self = NULL;

  res = CAPE_ERR_NONE;
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
    
  cape_udc_add_n (qout->cdata, "raid", self->raid);
  
exit_and_cleanup:

  flow_run_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL flow_run_get__on_compare (CapeUdc obj1, CapeUdc obj2)
{
  number_t rsid1 = cape_udc_get_n (obj1, "rsid", 0);
  number_t rsid2 = cape_udc_get_n (obj2, "rsid", 0);
  
  return (rsid1 > rsid2) ? -1 : (rsid1 < rsid2) ? 1 : 0;
}

//-----------------------------------------------------------------------------

int flow_run_get (FlowRun* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowRun self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;
  CapeString rdata_text = NULL;
  CapeStopTimer st = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing wpid");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_run_get} missing cdata");
    goto exit_and_cleanup;
  }

  self->raid = cape_udc_get_n (qin->cdata, "raid", 0);
  if (0 == self->raid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_run_get} missing raid");
    goto exit_and_cleanup;
  }

  st = cape_stoptimer_new ();
  
  cape_stoptimer_start (st);
  
  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow run get", "try to find flowrun with raid = %lu", self->raid);
  
  // fetch main master entry
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "raid"             , self->raid);
    cape_udc_add_n      (params, "wpid"             , self->wpid);
    
    cape_udc_add_n      (values, "state"            , 0);
    cape_udc_add_s_cp   (values, "fdesc"            , NULL);
    cape_udc_add_d      (values, "created"          , NULL);
    cape_udc_add_f      (values, "time_passed"      , .0);
    cape_udc_add_n      (values, "rsid"             , 0);
    cape_udc_add_s_cp   (values, "name"             , NULL);
    cape_udc_add_n      (values, "max"              , 0);
    cape_udc_add_n      (values, "cur"              , 0);
    cape_udc_add_n      (values, "err_code"         , 0);
    cape_udc_add_s_cp   (values, "err_text"         , NULL);

    // default size is 4000 -> increase to 20000 bytes
    cape_udc_add_s_cp   (values, "rdata"          , "{\"size\": 20000}");

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_run_get_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cape_udc_sort_list (query_results, flow_run_get__on_compare);
  
  first_row = cape_udc_ext_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NO_DATASET");
    goto exit_and_cleanup;
  }

  cape_stoptimer_stop (st);
  
  cape_log_fmt (CAPE_LL_TRACE, "FLOW", "flow run get", "found entries = %i in %fs", cape_udc_size(query_results) + 1, cape_stoptimer_get (st) / 1000);

  // extract the rdata as text
  rdata_text = cape_udc_ext_s (first_row, "rdata");
  if (rdata_text)
  {
    // deserialize
    CapeUdc h = cape_json_from_s (rdata_text);
    if (h == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "rdata can't be deserialized");
      goto exit_and_cleanup;
    }
    
    cape_udc_add_name (first_row, &h, "rdata");
  }

  cape_udc_replace_mv (&(qout->cdata), &first_row);
  res = CAPE_ERR_NONE;
    
exit_and_cleanup:

  cape_stoptimer_del (&st);
  cape_str_del (&rdata_text);
  
  cape_udc_del (&query_results);
  cape_udc_del (&first_row);

  flow_run_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
