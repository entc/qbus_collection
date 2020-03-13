#include "jobs_list.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct JobsList_s
{
  QBus qbus;                          // reference
  AdblSession adbl_session;           // reference
  QJobs jobs;                         // reference
  
  number_t wpid;
  number_t gpid;

  number_t joid;
  
  CapeDatetime* dt_start;
};

//-----------------------------------------------------------------------------

JobsList jobs_list_new (QBus qbus, AdblSession adbl_session, QJobs jobs)
{
  JobsList self = CAPE_NEW(struct JobsList_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->jobs = jobs;
  
  self->wpid = 0;
  self->gpid = 0;
  
  self->joid = 0;
  
  self->dt_start = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void jobs_list_del (JobsList* p_self)
{
  if (*p_self)
  {
    JobsList self = *p_self;
    
    cape_datetime_del (&(self->dt_start));
    
    CAPE_DEL (p_self, struct JobsList_s);
  }
}

//-----------------------------------------------------------------------------

int jobs_list_get (JobsList* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsList self = *p_self;

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
  
  // local vars (initialization)
  CapeUdc query_results = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"        , self->wpid);

    cape_udc_add_d      (values, "event_start" , NULL);
    cape_udc_add_d      (values, "event_stop"  , NULL);
    cape_udc_add_b      (values, "repeated"    , FALSE);
    cape_udc_add_b      (values, "active"      , FALSE);
    cape_udc_add_s_cp   (values, "modp"        , NULL);
    cape_udc_add_n      (values, "ref"         , 0);
    cape_udc_add_s_cp   (values, "data"        , NULL);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "jobs_list", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
  //-------------
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int jobs_list_add (JobsList* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsList self = *p_self;
  
  // local objects
  CapeUdc params = NULL;

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

  self->gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (self->gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  self->dt_start = cape_datetime_cp (cape_udc_get_d (qin->cdata, "event_start", NULL));
  if (self->dt_start == NULL)
  {
    const CapeString event_start_rel = cape_udc_get_s (qin->cdata, "event_start_rel", NULL);
    if (event_start_rel)
    {
      self->dt_start = cape_datetime_new ();
      
      // apply relative delta
      cape_datetime_utc__add_s (self->dt_start, event_start_rel);
    }
    else
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "dt_start is missing");
      goto exit_and_cleanup;
    }
  }
  
  // optional
  params = cape_udc_ext (qin->cdata, "params");

  // clean up
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
  
  res = qjobs_event (self->jobs, self->wpid, self->gpid, self->dt_start, 1, &params, "JOBS", "mjob", 0, 0, err);

exit_and_cleanup:
  
  cape_udc_del (&params);
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
