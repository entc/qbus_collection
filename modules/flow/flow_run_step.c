#include "flow_run_step.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>

//-----------------------------------------------------------------------------

struct FlowRunStep_s
{
  QBus qbus;
  FlowRunDbw dbw;
  
};

//-----------------------------------------------------------------------------

FlowRunStep flow_run_step_new (QBus qbus)
{
  FlowRunStep self = CAPE_NEW (struct FlowRunStep_s);
  
  self->qbus = qbus;
  self->dbw = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void flow_run_step_del (FlowRunStep* p_self, FlowRunDbw* p_dbw)
{
  if (*p_self)
  {
    FlowRunStep self = *p_self;

    // transfer back
    if (p_dbw && self->dbw)
    {
      *p_dbw = self->dbw;
      self->dbw = NULL;
    }
    
    flow_run_dbw_del (&(self->dbw));
    
    CAPE_DEL (p_self, struct FlowRunStep_s);
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL flow_run_step__method__syncron__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowRunStep self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }

  if (qin->cdata)
  {
    number_t cb_counter = cape_udc_get_n (qin->cdata, "cb_counter", 0);
    if (cb_counter)
    {
      // this is a special case to transfer the process flow to an external logic
      // -> sets the process to halt
      // -> at some point the flow_set method must be called to continue with this process
      
      // set the process to halt
      flow_run_dbw_state_set (self->dbw, FLOW_STATE__HALT, NULL);

      res = CAPE_ERR_NONE;
      goto exit_and_cleanup;
    }
    
    
    {
      CapeString h = cape_json_to_s (qin->cdata);
      
      printf ("................................................................................\n");
      printf ("RETURNED CDATA: %s\n", h);
    }
    
    flow_run_dbw_tdata__merge_to (self->dbw, &(qin->cdata));
  }
  else
  {
    printf ("................................................................................\n");
    printf ("RETURNED CDATA: NONE\n");
  }

  // set the step to done
  flow_run_dbw_state_set (self->dbw, FLOW_STATE__DONE, NULL);

  // transfer ownership for queuing
  res = flow_run_dbw_set (&(self->dbw), FALSE, FLOW_ACTION__PRIM, NULL, err);

exit_and_cleanup:
  
  if (res && self->dbw)
  {
    flow_run_dbw_state_set (self->dbw, FLOW_STATE__ERROR, err);
  }
  
  flow_run_step_del (&self, NULL);
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_run_step__method__syncron__call (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  int res;
  FlowRunStep self = *p_self;
  FlowRunDbw dbw = *p_dbw;
  
  // local objects
  CapeString module = NULL;
  CapeString method = NULL;
  CapeUdc params = NULL;

  res = flow_run_dbw_pdata__qbus (dbw, &module, &method, &params, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method sync", "---------------+----------------------------------+");
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "method sync", "          call | %s : %s                      |", module, method);
  cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method sync", "---------------+----------------------------------+");

  flow_run_dbw_pdata__logs_merge (dbw, params);
  
  flow_run_dbw_tdata__merge_in (dbw, params);
  
  // call
  if (self->qbus)
  {
    QBusM msg = qbus_message_new (NULL, NULL);
    
    cape_udc_replace_cp (&(msg->rinfo), flow_run_dbw_rinfo_get (dbw));
    cape_udc_replace_mv (&(msg->cdata), &params);

    // transfer owership of dbw to self objects
    self->dbw = dbw;
    *p_dbw = NULL;
    
    res = qbus_send (self->qbus, module, method, msg, self, flow_run_step__method__syncron__on_call, err);
    
    if (res == CAPE_ERR_NONE)
    {
      *p_self = NULL;
      res = CAPE_ERR_CONTINUE;
    }
    
    qbus_message_del (&msg);
  }
  
exit_and_cleanup:
  
  cape_str_del (&module);
  cape_str_del (&method);
  
  cape_udc_del (&params);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_step__method__syncron (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeUdc params, CapeErr err)
{
  // get the current state of the step
  switch (flow_run_dbw_state_get (*p_dbw))
  {
    case FLOW_STATE__NONE:
    {
      return flow_run_step__method__syncron__call (p_self, p_dbw, err);
    }
    case FLOW_STATE__HALT:
    {
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method sync", "---------------+----------------------------------+");
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method sync", "          call | continue                         |");
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method sync", "---------------+----------------------------------+");
      
      if (params)
      {
        const CapeString cb_log = cape_udc_get_s (params, "cb_log", NULL);
        if (cb_log)
        {
          flow_run_dbw__event_add (*p_dbw, CAPE_ERR_NONE, cb_log, 0, 0);
        }
      }
      
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_run_step__method__async (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  int res;
  FlowRunStep self = *p_self;
  FlowRunDbw dbw = *p_dbw;
  
  // local objects
  CapeString module = NULL;
  CapeString method = NULL;
  CapeUdc params = NULL;

  res = flow_run_dbw_pdata__qbus (dbw, &module, &method, &params, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "method sync", "+---------------+----------------------------------+");
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "method sync", "|          call | %s : %s                      |", module, method);
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "method sync", "+---------------+----------------------------------+");

  flow_run_dbw_pdata__logs_merge (dbw, params);
  
  flow_run_dbw_tdata__merge_in (dbw, params);

  // call
  if (self->qbus)
  {
    QBusM msg = qbus_message_new (NULL, NULL);
    
    cape_udc_replace_cp (&(msg->rinfo), flow_run_dbw_rinfo_get (dbw));
    cape_udc_replace_mv (&(msg->cdata), &params);

    // transfer owership of dbw to self objects
    self->dbw = dbw;
    *p_dbw = NULL;
    
    res = qbus_send (self->qbus, module, method, msg, NULL, NULL, err);
        
    qbus_message_del (&msg);
  }

exit_and_cleanup:
  
  cape_str_del (&module);
  cape_str_del (&method);
  
  cape_udc_del (&params);

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_step__method__wait (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeUdc params, CapeErr err)
{
  int res;

  // get the current state of the step
  switch (flow_run_dbw_state_get (*p_dbw))
  {
    case FLOW_STATE__NONE:
    {
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method wait", "---------------+----------------------------------+");
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method wait", "          wait | initialize                       |");
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method wait", "---------------+----------------------------------+");

      res = flow_run_dbw_wait__init (*p_dbw, err);
      break;
    }
    case FLOW_STATE__HALT:
    {
      const CapeString uuid;
      CapeString code = NULL;
      CapeUdc code_node;
      
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method wait", "---------------+----------------------------------+");
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method wait", "          wait | continue                         |");
      cape_log_msg (CAPE_LL_DEBUG, "FLOW", "method wait", "---------------+----------------------------------+");

      if (NULL == params)
      {
        res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "'params' is missing");
        goto exit_and_cleanup;
      }
      
      uuid = cape_udc_get_s (params, "uuid", NULL);
      if (NULL == uuid)
      {
        res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "'params' is missing");
        goto exit_and_cleanup;
      }

      // optional
      code_node = cape_udc_get (params, "code");
      
      if (code_node) switch (cape_udc_type (code_node))
      {
        case CAPE_UDC_STRING:
        {
          code = cape_str_cp (cape_udc_s (code_node, NULL));
          break;
        }
        case CAPE_UDC_NUMBER:
        {
          code = cape_str_n (cape_udc_n (code_node, 0));
          break;
        }
      }
      
      res = flow_run_dbw_wait__check_item (*p_dbw, uuid, code, err);
      
      cape_str_del (&code);
      break;
    }
    default:
    {
      res = CAPE_ERR_CONTINUE;
      break;
    }
  }
  
exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_step__method__split_add (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  int res;
  FlowRunStep self = *p_self;
  FlowRunDbw dbw = *p_dbw;

  CapeUdc list = NULL;
  CapeUdcCursor* cursor = NULL;
  
  number_t syncid = 0;
  number_t wfid;
    
  res = flow_run_dbw_xdata__split (dbw, &list, &wfid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // create sync object
  syncid = flow_run_dbw_sync__add (dbw, cape_udc_size (list), err);
  if (syncid == 0)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // transfer owership of dbw to self objects
  self->dbw = dbw;
  *p_dbw = NULL;

  // set state halt to commit all changes so far
  flow_run_dbw_state_set (dbw, FLOW_STATE__HALT, NULL);
  
  cursor = cape_udc_cursor_new (list, CAPE_DIRECTION_FORW);
  while (cape_udc_cursor_next (cursor))
  {
    FlowRunDbw dbw_cloned = flow_run_dbw_clone (dbw);

    {
      CapeUdc item = cape_udc_cursor_ext (list, cursor);

      flow_run_dbw_tdata__merge_to (dbw_cloned, &item);
    }
        
    // create a new process task
    res = flow_run_dbw_init (dbw_cloned, wfid, syncid, FALSE, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    // transfer ownership for queuing
    res = flow_run_dbw_set (&dbw_cloned, TRUE, FLOW_ACTION__PRIM, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_CONTINUE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&list);
  
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_step__method__split (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  FlowRunStep self = *p_self;

  // get the current state of the step
  switch (flow_run_dbw_state_get (*p_dbw))
  {
    case FLOW_STATE__NONE:
    {
      // get the syncronization counter
      number_t syncnt = flow_run_dbw_synct_get (*p_dbw);

      // do an extra check
      //if (syncnt == -1)
      {
        cape_log_msg (CAPE_LL_DEBUG, "FLOW", "run step", "---------------+----------------------------------+");
        cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "taskflow", "         split | create tasks                     |");
        cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "taskflow", "---------------+----------------------------------+");

        return flow_run_step__method__split_add (p_self, p_dbw, err);
      }
      
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_run_step__start_task (FlowRunStep* p_self, CapeErr err)
{
  
  
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeUdc flow_run_step__switch__seek (CapeUdc switch_node, const CapeString value)
{
  // check for primary as value
  CapeUdc switch_case = cape_udc_get (switch_node, value);

  // continue with default
  if (switch_case == NULL)
  {
    switch_case = cape_udc_get (switch_node, "default");
  }

  return switch_case;
}

//-----------------------------------------------------------------------------

int flow_run_step__switch__add (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  int res;
  FlowRunStep self = *p_self;
  FlowRunDbw dbw = *p_dbw;

  CapeUdc switch_case;
  
  // local objects
  CapeString value = NULL;
  CapeUdc switch_node = NULL;
  
  res = flow_run_dbw_xdata__switch (dbw, &value, &switch_node, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
      
  if (NULL == value)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "no value defined");
    goto exit_and_cleanup;
  }
  
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", " VALUE         | value %8s                   |", value);
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", "---------------+----------------------------------+");

  // try to find a case which fits to the values content
  switch_case = flow_run_step__switch__seek (switch_node, value);
  if (switch_case)
  {
    number_t wfid = cape_udc_n (switch_case, 0);
    number_t syncid;
    
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", " CASE          | match, wfid: %4i                |", wfid);
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", "---------------+----------------------------------+");
    
    // create sync object
    syncid = flow_run_dbw_sync__add (dbw, 1, err);
    if (syncid == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
    
    // transfer owership of dbw to self objects
    self->dbw = dbw;
    *p_dbw = NULL;

    // set state halt to commit all changes so far
    flow_run_dbw_state_set (dbw, FLOW_STATE__HALT, NULL);

    FlowRunDbw dbw_cloned = flow_run_dbw_clone (dbw);

    // create a new process task
    res = flow_run_dbw_init (dbw_cloned, wfid, syncid, FALSE, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    // transfer ownership for queuing
    res = flow_run_dbw_set (&dbw_cloned, TRUE, FLOW_ACTION__PRIM, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    res = CAPE_ERR_CONTINUE;
  }
  else
  {
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", " CASE          | no match -> continue             |");
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", "---------------+----------------------------------+");

    res = CAPE_ERR_NONE;
  }
  
exit_and_cleanup:

  cape_str_del (&value);
  
  return res;
}

//-----------------------------------------------------------------------------

int flow_run_step__switch (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  FlowRunStep self = *p_self;

  // get the current state of the step
  switch (flow_run_dbw_state_get (*p_dbw))
  {
    case FLOW_STATE__NONE:
    {
      // get the syncronization counter
      number_t syncnt = flow_run_dbw_synct_get (*p_dbw);

      // do an extra check
      //if (syncnt == -1)
      {
        cape_log_msg (CAPE_LL_DEBUG, "FLOW", "run step", "---------------+----------------------------------+");
        cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "taskflow", "        switch | create tasks                     |");
        cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "taskflow", "---------------+----------------------------------+");

        return flow_run_step__switch__add (p_self, p_dbw, err);
      }
      
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_run_step__if__add (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  int res;
  FlowRunStep self = *p_self;
  FlowRunDbw dbw = *p_dbw;

  CapeUdc value_node = NULL;
  number_t wfid = 0;
  
  res = flow_run_dbw_xdata__if (dbw, &value_node, &wfid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (value_node)
  {
    number_t syncid;

    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "switch add", " IF            | found                            |");

    // create sync object
    syncid = flow_run_dbw_sync__add (dbw, 1, err);
    if (syncid == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }

    // transfer owership of dbw to self objects
    self->dbw = dbw;
    *p_dbw = NULL;

    // set state halt to commit all changes so far
    flow_run_dbw_state_set (dbw, FLOW_STATE__HALT, NULL);

    FlowRunDbw dbw_cloned = flow_run_dbw_clone (dbw);

    // create a new process task
    res = flow_run_dbw_init (dbw_cloned, wfid, syncid, FALSE, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    // transfer ownership for queuing
    res = flow_run_dbw_set (&dbw_cloned, TRUE, FLOW_ACTION__PRIM, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    res = CAPE_ERR_CONTINUE;
  }
  else
  {
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "if add", " IF            | not found -> continue            |");
    cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "if add", "---------------+----------------------------------+");

    res = CAPE_ERR_NONE;
  }
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "if add", " ERROR         | %s", cape_err_text (err));
    cape_log_fmt (CAPE_LL_ERROR, "FLOW", "if add", "---------------+----------------------------------+");
  }

  return res;
}

//-----------------------------------------------------------------------------

int flow_run_step__if (FlowRunStep* p_self, FlowRunDbw* p_dbw, CapeErr err)
{
  FlowRunStep self = *p_self;

  // get the current state of the step
  switch (flow_run_dbw_state_get (*p_dbw))
  {
    case FLOW_STATE__NONE:
    {
      // get the syncronization counter
      number_t syncnt = flow_run_dbw_synct_get (*p_dbw);

      //if (syncnt == -1)
      {
        cape_log_msg (CAPE_LL_DEBUG, "FLOW", "run step", "---------------+----------------------------------+");
        cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "taskflow", "            if | create tasks                     |");
        cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "taskflow", "---------------+----------------------------------+");

        return flow_run_step__if__add (p_self, p_dbw, err);
      }
      
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_run_step__place_message (FlowRunStep* p_self, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_run_step_set (FlowRunStep* p_self, FlowRunDbw* p_dbw, number_t action, CapeUdc params, CapeErr err)
{
  int res;
  FlowRunStep self = *p_self;

  // support global actions
  switch (action)
  {
    case FLOW_ACTION__ABORT:
    {
      int res;
      
      /*
      ProcTaskflow taskflow = proc_taskflow_new (self->qmod, self->adbo, self->mutex, self->wpid, self->page);
      
      // continue with a taskflow
      // change the sequence ID to abort
      res = proc_taskflow_seqt (&taskflow, din, dout, self->taid, self->syncid, PROC_TASK_SQTID__ABORT, err);

      proc_task_del (p_self);
       */
      
      return res;
    }
    case FLOW_ACTION__NONE:
    {
      cape_log_msg (CAPE_LL_WARN, "FLOW", "run step", "no action was given");
      
      res = CAPE_ERR_NONE;
      goto exit_and_cleanup;
    }
  }
  
  cape_log_fmt (CAPE_LL_DEBUG, "FLOW", "run step", "run step with action = %i, fctid = %i", action, flow_run_dbw_fctid_get (*p_dbw));

  switch (flow_run_dbw_fctid_get (*p_dbw))
  {
    case 3:    // call another module's method (syncron)
    {
      res = flow_run_step__method__syncron (p_self, p_dbw, params, err);
      break;
    }
    case 4:    // call another module's method (async)
    {
      res = flow_run_step__method__async (p_self, p_dbw, err);
      break;
    }
    case 5:    // wait
    {
      res = flow_run_step__method__wait (p_self, p_dbw, params, err);
      break;
    }
    case 10:   // split into several taskflows, sync at the end and continue this taskflow
    {
      res = flow_run_step__method__split (p_self, p_dbw, err);
      break;
    }
    case 11:   // start another task
    {
      res = flow_run_step__start_task (p_self, err);
      break;
    }
    case 12:   // switch
    {
      res = flow_run_step__switch (p_self, p_dbw, err);
      break;
    }
    case 13:   // if
    {
      res = flow_run_step__if (p_self, p_dbw, err);
      break;
    }
    case 21:   // place a message on the desktop
    {
      res = flow_run_step__place_message (p_self, err);
      break;
    }
    case 50:   // (variable) copy
    {
      res = flow_run_dbw_xdata__var_copy (*p_dbw, err);
      break;
    }
    case 51:   // (variable) create node
    {
      res = flow_run_dbw_xdata__create_node (*p_dbw, err);
      break;
    }
    default:
    {
      cape_log_fmt (CAPE_LL_TRACE, "FLOW", "run step", "step with fctid = %i is not implemented", flow_run_dbw_fctid_get (*p_dbw));

      res = CAPE_ERR_NONE;
      break;
    }
  }

exit_and_cleanup:

  flow_run_step_del (p_self, p_dbw);
  return res;
}

//-----------------------------------------------------------------------------

