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
  number_t state = FLOW_STATE__NONE;
  
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

void flow_chain_item__del (FlowChainItem* p_self)
{
  if (*p_self)
  {
    FlowChainItem self = *p_self;
    
    cape_udc_del (&(self->item));
    cape_udc_del (&(self->states));
    
    CAPE_DEL (&self, struct FlowChainItem_s);
  }
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

void __STDCALL flow_chain_get__logs__on_del (void* key, void* val)
{
  {
    FlowChainItem fci = val; flow_chain_item__del (&fci);    
  }
}

//-----------------------------------------------------------------------------

int flow_chain_get__run (AdblTrx trx, number_t psid, CapeUdc logs, CapeErr err)
{
  int res;

  // local objects
  CapeMap logs_map = cape_map_new (cape_map__compare__n, flow_chain_get__logs__on_del, NULL);
  
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

void __STDCALL flow_chain_log__logs__on_del (void* key, void* val)
{
  {
    CapeUdc log_item = val;
    cape_udc_del (&log_item);
  }
}

//-----------------------------------------------------------------------------

void flow_chain_log__append (CapeUdc item, CapeMap log_items, const CapeString lx_id, const CapeString tx_refid, const CapeString lx_state, const CapeString lx_pot, const CapeString lx_text, const CapeString wx_name, const CapeString wx_tag)
{
  number_t log_id = cape_udc_get_n (item, lx_id, 0);
  if (log_id)
  {
    // check if the entry already exists
    CapeMapNode n = cape_map_find (log_items, (void*)log_id);
    if (n == NULL)
    {
      CapeUdc log_item = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n (log_item, "logid", log_id);
      cape_udc_add_n (log_item, "refid", cape_udc_get_n (item, tx_refid, 0));
      cape_udc_add_n (log_item, "state", cape_udc_get_n (item, lx_state, 0));
      cape_udc_add_d (log_item, "pot", cape_udc_get_d (item, lx_pot, NULL));
      
      {
        const CapeString h1 = cape_udc_get_s (item, lx_text, NULL);
        if (h1)
        {
          CapeUdc h2 = cape_json_from_s (h1);
          if (h2)
          {
            cape_udc_add_name (log_item, &h2, "log");
          }
        }
      }
      {
        CapeString h1 = cape_udc_ext_s (item, wx_name);
        if (h1)
        {
          cape_udc_add_s_mv (log_item, "step", &h1);
        }
      }
      {
        CapeString h1 = cape_udc_ext_s (item, wx_tag);
        if (h1)
        {
          cape_udc_add_s_mv (log_item, "tag", &h1);
        }
      }

      cape_map_insert (log_items, (void*)log_id, log_item);
    }
  }
}

//-----------------------------------------------------------------------------

void flow_chain_log__convert_to_cdata (CapeMap log_items, CapeUdc cdata)
{
  // local objects
  CapeMapCursor* cursor = cape_map_cursor_create (log_items, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    // transfer ownership
    CapeUdc item = cape_map_node_value (cursor->node);
    cape_map_node_set (cursor->node, NULL);

    cape_udc_add (cdata, &item);
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

int flow_chain_log__ps (FlowChain self, number_t psid, CapeUdc cdata, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  CapeMap log_items = NULL;

  // get the psid's having the refid
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "t1_psid"          , psid);
    
    cape_udc_add_n      (values, "t1_refid"         , 0);
    cape_udc_add_n      (values, "l1_id"            , 0);
    cape_udc_add_d      (values, "l1_pot"           , NULL);
    cape_udc_add_n      (values, "l1_state"         , 0);
    cape_udc_add_s_cp   (values, "l1_text"          , NULL);
    cape_udc_add_s_cp   (values, "w1_name"          , NULL);
    cape_udc_add_s_cp   (values, "w1_tag"           , NULL);
    cape_udc_add_n      (values, "t2_refid"         , 0);
    cape_udc_add_n      (values, "l2_id"            , 0);
    cape_udc_add_d      (values, "l2_pot"           , NULL);
    cape_udc_add_n      (values, "l2_state"         , 0);
    cape_udc_add_s_cp   (values, "l2_text"          , NULL);
    cape_udc_add_s_cp   (values, "w2_name"          , NULL);
    cape_udc_add_s_cp   (values, "w2_tag"           , NULL);
    cape_udc_add_n      (values, "t3_refid"         , 0);
    cape_udc_add_n      (values, "l3_id"            , 0);
    cape_udc_add_d      (values, "l3_pot"           , NULL);
    cape_udc_add_n      (values, "l3_state"         , 0);
    cape_udc_add_s_cp   (values, "l3_text"          , NULL);
    cape_udc_add_s_cp   (values, "w3_name"          , NULL);
    cape_udc_add_s_cp   (values, "w3_tag"           , NULL);
    cape_udc_add_n      (values, "t4_refid"         , 0);
    cape_udc_add_n      (values, "l4_id"            , 0);
    cape_udc_add_d      (values, "l4_pot"           , NULL);
    cape_udc_add_n      (values, "l4_state"         , 0);
    cape_udc_add_s_cp   (values, "l4_text"          , NULL);
    cape_udc_add_s_cp   (values, "w4_name"          , NULL);
    cape_udc_add_s_cp   (values, "w4_tag"           , NULL);
    cape_udc_add_n      (values, "t5_refid"         , 0);
    cape_udc_add_n      (values, "l5_id"            , 0);
    cape_udc_add_d      (values, "l5_pot"           , NULL);
    cape_udc_add_n      (values, "l5_state"         , 0);
    cape_udc_add_s_cp   (values, "l5_text"          , NULL);
    cape_udc_add_s_cp   (values, "w5_name"          , NULL);
    cape_udc_add_s_cp   (values, "w5_tag"           , NULL);

    /*
     flow_log_view
     
     select
     
     t1.id t1_psid, t2.id t2_psid, t3.id t3_psid, t4.id t4_psid, t5.id t5_psid,
     
     t1.refid t1_refid, l1.id l1_id, l1.pot l1_pot, l1.state l1_state, l1.data l1_text,
     t2.refid t2_refid, l2.id l2_id, l2.pot l2_pot, l2.state l2_state, l2.data l2_text,
     t3.refid t3_refid, l3.id l3_id, l3.pot l3_pot, l3.state l3_state, l3.data l3_text,
     t4.refid t4_refid, l4.id l4_id, l4.pot l4_pot, l4.state l4_state, l4.data l4_text,
     t5.refid t5_refid, l5.id l5_id, l5.pot l5_pot, l5.state l5_state, l5.data l5_text,
     
     w1.name w1_name, w1.tag w1_tag,
     w2.name w2_name, w2.tag w2_tag,
     w3.name w3_name, w3.tag w3_tag,
     w4.name w4_name, w4.tag w4_tag,
     w5.name w5_name, w5.tag w5_tag

     from proc_tasks t1 left join proc_task_sync s1 on s1.taid = t1.id left join proc_tasks t2 on t2.sync = s1.id left join proc_task_sync s2 on s2.taid = t2.id left join proc_tasks t3 on t3.sync = s2.id left join proc_task_sync s3 on s3.taid = t3.id left join proc_tasks t4 on t4.sync = s3.id left join proc_task_sync s4 on s4.taid = t4.id left join proc_tasks t5 on t5.sync = s4.id
     
     left join flow_log l1 on l1.psid = t1.id
     left join flow_log l2 on l2.psid = t2.id
     left join flow_log l3 on l3.psid = t3.id
     left join flow_log l4 on l4.psid = t4.id
     left join flow_log l5 on l5.psid = t5.id
     
     left join proc_worksteps w1 on w1.id = l1.wsid
     left join proc_worksteps w2 on w2.id = l2.wsid
     left join proc_worksteps w3 on w3.id = l3.wsid
     left join proc_worksteps w4 on w4.id = l4.wsid
     left join proc_worksteps w5 on w5.id = l5.wsid
     ;
     */
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_log_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  // create a map to flatten the logs tree
  log_items = cape_map_new (cape_map__compare__n, flow_chain_log__logs__on_del, NULL);
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    flow_chain_log__append (cursor->item, log_items, "l1_id", "t1_refid", "l1_state", "l1_pot", "l1_text", "w1_name", "w1_tag");
    flow_chain_log__append (cursor->item, log_items, "l2_id", "t2_refid", "l2_state", "l2_pot", "l2_text", "w2_name", "w2_tag");
    flow_chain_log__append (cursor->item, log_items, "l3_id", "t3_refid", "l3_state", "l3_pot", "l3_text", "w3_name", "w3_tag");
    flow_chain_log__append (cursor->item, log_items, "l4_id", "t4_refid", "l4_state", "l4_pot", "l4_text", "w4_name", "w4_tag");
    flow_chain_log__append (cursor->item, log_items, "l5_id", "t5_refid", "l5_state", "l5_pot", "l5_text", "w5_name", "w5_tag");
  }
  
  flow_chain_log__convert_to_cdata (log_items, cdata);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  cape_map_del (&log_items);

  return res;
}

//-----------------------------------------------------------------------------

int flow_chain_log (FlowChain* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowChain self = *p_self;
  
  number_t refid;
  number_t gpid;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  CapeUdc cdata = NULL;

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

  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
    goto exit_and_cleanup;
  }

  refid = cape_udc_get_n (qin->cdata, "refid", 0);
  if (refid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "refid is missing");
    goto exit_and_cleanup;
  }

  // get the psid's having the refid
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "refid"            , refid);
    cape_udc_add_n      (params, "wpid"             , self->wpid);
    cape_udc_add_n      (params, "gpid"             , gpid);

    cape_udc_add_n      (values, "psid"             , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "flow_instance", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  // create the result structure
  cdata = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    // continue with the process specific collecting
    res = flow_chain_log__ps (self, cape_udc_get_n (cursor->item, "psid", 0), cdata, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (&(qout->cdata), &cdata);
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  cape_udc_del (&cdata);

  flow_chain_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
