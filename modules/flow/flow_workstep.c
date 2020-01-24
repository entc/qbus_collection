#include "flow_workstep.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct FlowWorkstep_s
{
  QBus qbus;                   // reference
  AdblSession adbl_session;    // reference
  
  number_t wpid;               // workspace id
  number_t wfid;               // workflow id
};

//-----------------------------------------------------------------------------

FlowWorkstep flow_workstep_new (QBus qbus, AdblSession adbl_session)
{
  FlowWorkstep self = CAPE_NEW(struct FlowWorkstep_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  
  self->wpid = 0;
  self->wfid = 0;
  
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
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "missing wpid");
  }
  
  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_check} missing cdata");
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int flow_workstep_add (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
  exit_and_cleanup:
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep_set (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int flow_workstep_rm (FlowWorkstep* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  FlowWorkstep self = *p_self;
  
  res = flow_workstep__intern__qin_check (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->wfid = cape_udc_get_n (qin->cdata, "wfid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
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
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{flow_workstep_set} missing parameter 'wfid'");
    goto exit_and_cleanup;
  }
  
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  flow_workstep_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
