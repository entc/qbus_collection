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
  number_t joid;
  
  CapeDatetime* dt_start;
  CapeString modp;
  number_t ref;
  CapeUdc data;
};

//-----------------------------------------------------------------------------

JobsList jobs_list_new (QBus qbus, AdblSession adbl_session, QJobs jobs)
{
  JobsList self = CAPE_NEW(struct JobsList_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->jobs = jobs;
  
  self->wpid = 0;
  self->joid = 0;
  
  self->dt_start = NULL;
  self->modp = NULL;
  self->ref = 0;
  self->data = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void jobs_list_del (JobsList* p_self)
{
  if (*p_self)
  {
    JobsList self = *p_self;
    
    cape_datetime_del (&(self->dt_start));
    cape_str_del (&(self->modp));
    cape_udc_del (&(self->data));
    
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

static int __STDCALL jobs_list_add__on_vsec (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsList self = ptr;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  
  CapeString vsec = NULL;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't get cdata from tpro_template_get");
    goto exit_and_cleanup;
  }
  
  vsec = cape_udc_ext_s (qin->cdata, "secret");
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "secret value is missing");
    goto exit_and_cleanup;
  }

  // h1
  h1 = cape_json_to_s (self->data);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize data");
    goto exit_and_cleanup;
  }
  
  // encrypt the json data
  h2 = qcrypt__encrypt (vsec, h1, err);
  if (h2 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    // prepare the insert query
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // insert values
    cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
    cape_udc_add_n      (values, "wpid"         , self->wpid);
    cape_udc_add_d      (values, "event_start"  , self->dt_start);
    cape_udc_add_s_cp   (values, "modp"         , self->modp);
    cape_udc_add_b      (values, "repeated"     , FALSE);
    cape_udc_add_b      (values, "active"       , TRUE);
    
    if (self->ref)
    {
      cape_udc_add_n      (values, "ref"        , self->ref);
    }
    
    cape_udc_add_s_mv   (values, "data"         , &h2);
    
    // execute query
    self->joid = adbl_trx_insert (adbl_trx, "jobs_list", &values, err);
    if (self->joid == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  //res = qjobs_event (self->jobs, CapeDatetime* dt, CapeUdc* p_params, err);
  
  
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
  // add output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qout->cdata, "joid", self->joid);
  
exit_and_cleanup:
  
  cape_str_del (&h1);
  cape_str_del (&h2);
  
  adbl_trx_rollback (&adbl_trx, err);
  
  jobs_list_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int jobs_list_add (JobsList* p_self, QBusM qin, QBusM qout, CapeErr err)
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
  
  self->modp = cape_udc_ext_s (qin->cdata, "modp");
  if (self->modp == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "modp is missing");
    goto exit_and_cleanup;
  }
  
  self->data = cape_udc_ext (qin->cdata, "data");
  if (self->data == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "data is missing");
    goto exit_and_cleanup;
  }

  // optional
  self->ref = cape_udc_get_n (qin->cdata, "ref", 0);
  
  // clean up
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
  
  return qbus_continue (self->qbus, "AUTH", "getVaultSecret", qin, (void**)p_self, jobs_list_add__on_vsec, err);

exit_and_cleanup:
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
