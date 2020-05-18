#include "flow_chain.h"
#include "flow_run_dbw.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>

//-----------------------------------------------------------------------------

struct FlowChain_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  
  number_t wpid;               // workspace id
  number_t psid;               // process id
  
  CapeUdc pdatas;
};

//-----------------------------------------------------------------------------

FlowChain flow_chain_new (QBus qbus, AdblSession adbl_session)
{
  FlowChain self = CAPE_NEW(struct FlowChain_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  
  self->wpid = 0;
  self->psid = 0;
  
  self->pdatas = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void flow_chain_del (FlowChain* p_self)
{
  if (*p_self)
  {
    FlowChain self = *p_self;
    
    cape_udc_del (&(self->pdatas));
    
    CAPE_DEL (p_self, struct FlowChain_s);
  }
}

//-----------------------------------------------------------------------------

int flow_chain__intern__qin_check (FlowChain self, QBusM qin, CapeErr err)
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
    return cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
  }
  
  self->psid = cape_udc_get_n (qin->cdata, "psid", 0);
  if (self->psid == 0)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_process_get} missing parameter 'psid'");
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeUdc flow_chain_get__next__fetch (AdblTrx trx, number_t syncid, CapeErr err)
{
  CapeUdc ret = NULL;
  
  // local objects
  CapeUdc query_results = NULL;
  
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "chain get", "{fetch} get log of task, syncid = %i", syncid);

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "sync"          , syncid);

    cape_udc_add_n      (values, "id"            , 0);
    
    // execute the query
    query_results = adbl_trx_query (trx, "proc_tasks", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
  }
  
  cape_udc_replace_mv (&ret, &query_results);

exit_and_cleanup:
  
  return ret;
}

//-----------------------------------------------------------------------------

int flow_chain_get__next (AdblTrx trx, CapeUdc item, CapeUdc logs, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc items = NULL;
  CapeUdcCursor* cursor = NULL;
  
  number_t syncid = cape_udc_get_n (item, "syncid", 0);
  if (syncid == 0)
  {
    return CAPE_ERR_NONE;
  }
  
  items = flow_chain_get__next__fetch (trx, syncid, err);
  if (NULL == items)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // iterate
  cursor = cape_udc_cursor_new (items, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    number_t psid = cape_udc_get_n (cursor->item, "id", 0);
    if (psid)
    {
      res = flow_chain_get__fetch (trx, psid, logs, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  res = CAPE_ERR_NONE;
    
  //-------------
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&items);

  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_get__fetch (AdblTrx trx, number_t psid, CapeUdc logs, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "chain get", "{fetch} get log of process, psid = %i", psid);

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "taid"          , psid);

    cape_udc_add_n      (values, "taid"          , 0);
    cape_udc_add_d      (values, "created"       , NULL);
    cape_udc_add_n      (values, "wsid"          , 0);
    cape_udc_add_n      (values, "state"         , 0);
    cape_udc_add_n      (values, "cnt"           , 0);
    cape_udc_add_s_cp   (values, "name"          , NULL);
    cape_udc_add_n      (values, "fctid"         , 0);
    cape_udc_add_n      (values, "syncid"        , 0);
    cape_udc_add_n      (values, "syncnt"        , 0);
    cape_udc_add_n      (values, "vdataid"       , 0);
    cape_udc_add_s_cp   (values, "vdata"         , 0);
    cape_udc_add_n      (values, "gpid"          , 0);
    cape_udc_add_s_cp   (values, "owner"         , NULL);
    cape_udc_add_n      (values, "refid"         , 0);

    /*
    --- proc_task_logs_view ---

    select tl.id, tl.taid, tl.created, tl.wsid, tl.state, tl.cnt, name, fctid, ts.id syncid, ts.cnt syncnt, (select group_concat(ta.id) from proc_tasks ta where ta.sync = ts.id) tas, pd.id vdataid, pd.content vdata, tl.gpid, tl.owner, tl.refid from proc_task_logs tl join proc_worksteps ws on ws.id = tl.wsid left join proc_task_sync ts on ts.id = tl.sync left join proc_data pd on pd.id = tl.vdata;

     */
    // execute the query
    query_results = adbl_trx_query (trx, "proc_task_logs_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    // create a new sublist of logs
    CapeUdc logs_the_next_level = cape_udc_new (CAPE_UDC_LIST, "logs");

    res = flow_chain_get__next (trx, cursor->item, logs_the_next_level, err);
    
    // convert vdata
    {
      CapeUdc vdata_node = cape_udc_ext (cursor->item, "vdata");
      if (vdata_node)
      {
        // convert to node
        CapeUdc vdata = cape_json_from_s (cape_udc_s (vdata_node, NULL));
        if (vdata)
        {
          cape_udc_add_name (cursor->item, &vdata, "vdata");
        }
        
        cape_udc_del (&vdata_node);
      }
    }
    
    if (res)
    {
      cape_udc_del (&logs_the_next_level);
      goto exit_and_cleanup;
    }
    
    if (cape_udc_size (logs_the_next_level))
    {
      cape_udc_add (cursor->item, &logs_the_next_level);
    }
    else
    {
      cape_udc_del (&logs_the_next_level);
    }

    // add this to logs
    {
      CapeUdc h = cape_udc_cursor_ext (query_results, cursor);
      cape_udc_add (logs, &h);
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_get (FlowChain* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowChain self = *p_self;
  
  // local objects
  AdblTrx trx = NULL;
  CapeUdc logs = NULL;
  
  res = flow_chain__intern__qin_check (self, qin, err);
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
  
  // create the main logs list
  logs = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  // start with the initial fetching
  res = flow_chain_get__fetch (trx, self->psid, logs, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_udc_replace_mv (&(qout->cdata), &logs);
  
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;
    
exit_and_cleanup:

  adbl_trx_rollback (&trx, err);

  cape_udc_del (&logs);
  
  flow_chain_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_data__seek (FlowChain self, number_t psid, CapeUdc data, CapeErr err);

//-----------------------------------------------------------------------------

int flow_chain_data__next (FlowChain self, CapeUdc next, CapeUdc data, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdcCursor* cursor = cape_udc_cursor_new (next, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    res = flow_chain_data__seek (self, cape_udc_n (cursor->item, 0), data, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_data__seek (FlowChain self, number_t psid, CapeUdc data, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "chain get", "{fetch} get log of process, psid = %i", psid);

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"            , psid);

    cape_udc_add_list   (values, "next");
    cape_udc_add_node   (values, "data");

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_chain_data_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    // continue recursive
    {
      CapeUdc next = cape_udc_get (cursor->item, "next");
      if (next)
      {
        res = flow_chain_data__next (self, next, data, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
      }
    }
    
    // data
    {
      CapeUdc content = cape_udc_ext (cursor->item, "data");
      if (content)
      {
        cape_udc_add (data, &content);
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

int flow_chain_data (FlowChain* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowChain self = *p_self;
  
  // local objects
  CapeUdc data = NULL;
  
  res = flow_chain__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  data = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  res = flow_chain_data__seek (self, self->psid, data, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_udc_replace_mv (&(qout->cdata), &data);
  res = CAPE_ERR_NONE;
    
exit_and_cleanup:

  cape_udc_del (&data);

  flow_chain_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
