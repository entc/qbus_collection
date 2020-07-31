#include "flow_chain.h"
#include "flow_run_dbw.h"
#include "flow_run_step.h"
#include "flow_process.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>
#include <stc/cape_map.h>

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

CapeUdc flow_chain_get__next__fetch (AdblTrx trx, number_t psid, number_t wsid, CapeErr err)
{
  CapeUdc ret = NULL;
  
  // local objects
  CapeUdc query_results = NULL;
  
  //cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "chain get", "{fetch} get log of task, syncid = %i", syncid);

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "taid"          , psid);
    cape_udc_add_n      (params, "wsid"          , wsid);

    cape_udc_add_n      (values, "psid_client"   , 0);
    
    /*
     flow_process_sync_view
     
     select taid, wsid, ps.id psid_client from proc_task_sync ts left join proc_tasks ps on ps.sync = ts.id;
     */
    
    // execute the query
    query_results = adbl_trx_query (trx, "flow_process_sync_view", &params, &values, err);
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

int flow_chain_get__next (AdblTrx trx, CapeUdc item, CapeUdc logs, number_t psid, number_t wsid, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc items = NULL;
  CapeUdcCursor* cursor = NULL;
  
  items = flow_chain_get__next__fetch (trx, psid, wsid, err);
  if (NULL == items)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // iterate
  cursor = cape_udc_cursor_new (items, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    number_t psid_client = cape_udc_get_n (cursor->item, "psid_client", 0);
    if (psid_client)
    {
      res = flow_chain_get__run (trx, psid_client, logs, err);
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

struct FlowChainItem_s
{
  CapeUdc item;
  CapeUdc states;
  
}; typedef struct FlowChainItem_s* FlowChainItem;

//-----------------------------------------------------------------------------

void flow_chain_item__apply (FlowChainItem self, CapeUdc item)
{
  CapeUdc log_entry = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  {
    CapeUdc h = cape_udc_ext (item, "pot");
    if (h)
    {
      cape_udc_add (log_entry, &h);
    }
  }
  {
    CapeUdc h = cape_udc_ext (item, "state");
    if (h)
    {
      cape_udc_add (log_entry, &h);
    }
  }
  {
    CapeUdc h = cape_udc_ext (item, "data");
    if (h)
    {
      cape_udc_add (log_entry, &h);
    }
  }
  
  if (cape_udc_size (log_entry))
  {
    cape_udc_add (self->states, &log_entry);
  }
  else
  {
    cape_udc_del (&log_entry);
  }
}

//-----------------------------------------------------------------------------

int flow_chain_item__prev (FlowChainItem self, number_t* p_prev)
{
  number_t prev = *p_prev;
  
  if (cape_udc_get_n (self->item, "prev", 0) == prev)
  {
    *p_prev = cape_udc_get_n (self->item, "wsid", 0);
    
    // avoid double checkings
    return *p_prev != prev;
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

CapeUdc flow_chain_item__lot (FlowChainItem self, int* p_passed, number_t wsid)
{
  number_t state;
  
  cape_udc_add_name (self->item, &(self->states), "states");
  
  CapeUdc h1 = cape_udc_ext (self->item, "current_state");
  CapeUdc h2 = cape_udc_ext (self->item, "current_step");

  if (h2)
  {
    // correct states
    number_t current_step = cape_udc_n (h2, 0);
    
    if (current_step == wsid)
    {
      *p_passed = FALSE;
      
      if (h1)
      {
        state = cape_udc_n (h1, FLOW_STATE__INIT);
      }
      else
      {
        state = FLOW_STATE__INIT;
      }
    }
    else
    {
      if (*p_passed)
      {
        state = FLOW_STATE__DONE;
      }
      else
      {
        state = FLOW_STATE__NONE;
      }
    }
  }
  
  cape_udc_add_n (self->item, "state", state);
  
  cape_udc_del (&h1);
  cape_udc_del (&h2);

  return cape_udc_mv (&(self->item));
}

//-----------------------------------------------------------------------------

FlowChainItem flow_chain_item__new (CapeUdc* p_item)
{
  FlowChainItem self = CAPE_NEW (struct FlowChainItem_s);

  self->item = cape_udc_mv (p_item);
  self->states = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  flow_chain_item__apply (self, self->item);

  return self;
}

//-----------------------------------------------------------------------------

int flow_chain_get__tol__prev (CapeMap logs_map, CapeUdc logs, number_t* p_prev, int* p_passed)
{
  int ret = FALSE;
  CapeMapCursor* cursor = cape_map_cursor_create (logs_map, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    FlowChainItem fci = cape_map_node_value (cursor->node);

    if (flow_chain_item__prev (fci, p_prev))
    {
      CapeUdc h = flow_chain_item__lot (fci, p_passed, *p_prev);

      cape_udc_add (logs, &h);
      
      // continue with next round
      ret = TRUE;
      goto exit_and_cleanup;
    }
  }
  
exit_and_cleanup:

  cape_map_cursor_destroy (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

void flow_chain_get__tol (CapeMap logs_map, CapeUdc logs)
{
  number_t prev = 0;
  int passed = TRUE;
  
  while (flow_chain_get__tol__prev (logs_map, logs, &prev, &passed));
}

//-----------------------------------------------------------------------------

int flow_chain_get__fetch (AdblTrx trx, number_t psid, CapeMap logs_map, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "psid"          , psid);
    cape_udc_add_n      (params, "sqtid"         , FLOW_SQTID__DEFAULT);

    cape_udc_add_n      (values, "wsid"          , 0);
    cape_udc_add_n      (values, "wfid"          , 0);
    cape_udc_add_s_cp   (values, "name"          , NULL);
    cape_udc_add_n      (values, "fctid"         , 0);
    cape_udc_add_n      (values, "prev"          , 0);
    cape_udc_add_s_cp   (values, "tag"           , NULL);
    cape_udc_add_n      (values, "log"           , 0);

    cape_udc_add_n      (values, "current_step"  , 0);
    cape_udc_add_n      (values, "current_state" , 0);

    cape_udc_add_d      (values, "pot"           , NULL);
    cape_udc_add_n      (values, "state"         , 0);
    cape_udc_add_node   (values, "data"          );

    cape_udc_add_n      (values, "syncid"        , 0);

    /*
     flow_chain_view
     
     select ps.id psid, ws.id wsid, ws.sqtid, ws.wfid, ws.prev, ws.name, ws.fctid, ws.tag, ws.log, ps.current_step, ps.current_state, fl.pot, fl.state, fl.data, ps.sync syncid from proc_tasks ps join proc_worksteps ws on ws.wfid = ps.wfid left join flow_log fl on fl.psid = ps.id and fl.wsid = ws.id left join proc_task_sync ts on ts.id = ps.sync;
     */

    // execute the query
    query_results = adbl_trx_query (trx, "flow_chain_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  // collect all items to group them by wsid
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    // get the step id
    number_t wsid = cape_udc_get_n (cursor->item, "wsid", 0);
    
    CapeMapNode n = cape_map_find (logs_map, (void*)wsid);
    if (n)
    {
      FlowChainItem fci = cape_map_node_value (n);
      
      flow_chain_item__apply (fci, cursor->item);
    }
    else
    {
      CapeUdc h = cape_udc_cursor_ext (query_results, cursor);
      
      cape_udc_add_n (h, "psid", psid);
      
      FlowChainItem fci = flow_chain_item__new (&h);
      
      cape_map_insert (logs_map, (void*)wsid, fci);
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_get__sub_logs (AdblTrx trx, number_t psid, CapeUdc logs, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdcCursor* cursor = cape_udc_cursor_new (logs, CAPE_DIRECTION_FORW);
  CapeUdc logs_the_next_level = NULL;
  
  while (cape_udc_cursor_next (cursor))
  {
    number_t wsid = cape_udc_get_n (cursor->item, "wsid", 0);
    if (wsid)
    {
      // create a new sublist of logs
      logs_the_next_level = cape_udc_new (CAPE_UDC_LIST, "logs");

      res = flow_chain_get__next (trx, cursor->item, logs_the_next_level, psid, wsid, err);
      if (res)
      {
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
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&logs_the_next_level);
  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_get__run (AdblTrx trx, number_t psid, CapeUdc logs, CapeErr err)
{
  int res;

  // local objects
  CapeMap logs_map = cape_map_new (cape_map__compare__n, NULL, NULL);
  
  // fetch items from the database
  res = flow_chain_get__fetch (trx, psid, logs_map, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // construct the items of the logs
  flow_chain_get__tol (logs_map, logs);
  
  // add the recursive log structure
  res = flow_chain_get__sub_logs (trx, psid, logs, err);
  
exit_and_cleanup:
  
  cape_map_del (&logs_map);
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
  res = flow_chain_get__run (trx, self->psid, logs, err);
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
