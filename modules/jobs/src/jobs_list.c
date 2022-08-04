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

  number_t rpid;
  
  CapeDatetime* dt_start;
  CapeUdc params;
  
  CapeString ref_mod;
  CapeString ref_umi;
  
  number_t ref_id1;
  number_t ref_id2;
  
  CapeString rinfo_encrypted;
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
  
  self->rpid = 0;
  
  self->dt_start = NULL;
  self->params = NULL;
  
  self->ref_mod = NULL;
  self->ref_umi = NULL;
  
  self->ref_id1 = 0;
  self->ref_id2 = 0;
  
  self->rinfo_encrypted = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void jobs_list_del (JobsList* p_self)
{
  if (*p_self)
  {
    JobsList self = *p_self;
    
    cape_datetime_del (&(self->dt_start));
    cape_udc_del (&(self->params));
    
    cape_str_del (&(self->ref_umi));
    cape_str_del (&(self->ref_mod));
    
    cape_str_del (&(self->rinfo_encrypted));

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
  
  self->ref_mod = cape_udc_ext_s (qin->cdata, "ref_mod");
  if (self->ref_mod == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_MOD");
    goto exit_and_cleanup;
  }
  
  self->ref_umi = cape_udc_ext_s (qin->cdata, "ref_umi");
  if (self->ref_umi == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_UMI");
    goto exit_and_cleanup;
  }
  
  self->ref_id1 = cape_udc_get_n (qin->cdata, "ref_id1", 0);
  if (self->ref_id1 == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_ID1");
    goto exit_and_cleanup;
  }

  // local vars (initialization)
  CapeUdc query_results = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"        , self->wpid);
    //cape_udc_add_n      (params, "gpid"        , self->gpid);
    cape_udc_add_s_mv   (params, "ref_mod"     , &(self->ref_mod));
    cape_udc_add_s_mv   (params, "ref_umi"     , &(self->ref_umi));
    cape_udc_add_n      (params, "ref_id1"     , self->ref_id1);

    cape_udc_add_n      (values, "id"          , 0);
    cape_udc_add_d      (values, "event_date"  , NULL);
    cape_udc_add_n      (values, "ref_id2"     , 0);
    cape_udc_add_node   (values, "params"      );

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
  CapeString vsec = NULL;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  vsec = cape_udc_ext_s (qin->cdata, "secret");
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "secret value is missing");
    goto exit_and_cleanup;
  }

  res = qjobs_add (self->jobs, self->dt_start, 0, &(self->params), qin->rinfo, self->ref_mod, self->ref_umi, self->ref_id1, self->ref_id2, vsec, err);

exit_and_cleanup:
  
  cape_str_del (&vsec);
  
  jobs_list_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int jobs_list__input__start_date (JobsList self, QBusM qin, CapeErr err)
{
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
      return cape_err_set (err, CAPE_ERR_RUNTIME, "dt_start is missing");
    }
  }
  
  return CAPE_ERR_NONE;
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
  
  self->ref_mod = cape_udc_ext_s (qin->cdata, "ref_mod");
  if (self->ref_mod == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_MOD");
    goto exit_and_cleanup;
  }

  self->ref_umi = cape_udc_ext_s (qin->cdata, "ref_umi");
  if (self->ref_umi == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_UMI");
    goto exit_and_cleanup;
  }

  self->ref_id1 = cape_udc_get_n (qin->cdata, "ref_id1", 0);
  if (self->ref_id1 == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_ID1");
    goto exit_and_cleanup;
  }

  res = jobs_list__input__start_date (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // optional
  self->ref_id2 = cape_udc_get_n (qin->cdata, "ref_id2", 0);
  
  // optional
  self->params = cape_udc_ext (qin->cdata, "params");

  // clean up
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
  res = qbus_continue (self->qbus, "AUTH", "getVaultSecret", qin, (void**)p_self, jobs_list_add__on_vsec, err);

exit_and_cleanup:
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int jobs_list_set (JobsList* p_self, QBusM qin, QBusM qout, CapeErr err)
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
  
  self->rpid = cape_udc_get_n (qin->cdata, "rpid", 0);
  if (self->rpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_RPID");
    goto exit_and_cleanup;
  }

  res = jobs_list__input__start_date (self, qin, err);
  if (res)
  {
    // ignore
    cape_err_clr (err);
  }
  
  // optional
  params = cape_udc_ext (qin->cdata, "params");

  res = qjobs_set (self->jobs, self->rpid, self->dt_start, 0, params ? &params : NULL, NULL, err);
  
exit_and_cleanup:
  
  cape_udc_del (&params);
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int jobs_list_rm (JobsList* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsList self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

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
  
  self->ref_mod = cape_udc_ext_s (qin->cdata, "ref_mod");
  if (self->ref_mod == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_MOD");
    goto exit_and_cleanup;
  }

  self->ref_umi = cape_udc_ext_s (qin->cdata, "ref_umi");
  if (self->ref_umi == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_UMI");
    goto exit_and_cleanup;
  }
  
  self->ref_id1 = cape_udc_get_n (qin->cdata, "ref_id1", 0);
  if (self->ref_id1 == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.MISSING_REF_ID1");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"        , self->wpid);
    cape_udc_add_n      (params, "gpid"        , self->gpid);
    cape_udc_add_s_mv   (params, "ref_mod"     , &(self->ref_mod));
    cape_udc_add_s_mv   (params, "ref_umi"     , &(self->ref_umi));
    cape_udc_add_n      (params, "ref_id1"     , self->ref_id1);
    
    cape_udc_add_n      (values, "id"          , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "jobs_list", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    res = qjobs_rm (self->jobs, cape_udc_get_n (cursor->item, "id", 0), err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL jobs_list_run__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsList self = ptr;
  
  if (qin->err)
  {
    cape_log_fmt (CAPE_LL_ERROR, "JOBS", "list run", "on call: %s", cape_err_text (qin->err));
    
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  jobs_list_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL jobs_list_run__on_vsec (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsList self = ptr;
  
  // local objects
  CapeString vsec = NULL;
  CapeString h1 = NULL;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  vsec = cape_udc_ext_s (qin->cdata, "secret");
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "secret value is missing");
    goto exit_and_cleanup;
  }

  h1 = qcrypt__decrypt (vsec, self->rinfo_encrypted, err);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.DECRYPTION");
    goto exit_and_cleanup;
  }
  
  cape_udc_del (&(qin->rinfo));
  
  qin->rinfo = cape_json_from_s (h1);
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.JSON_DECODE");
    goto exit_and_cleanup;
  }
  
  // clean up
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
  
  qin->cdata = cape_udc_mv (&(self->params));
  
  res = qbus_continue (qbus, self->ref_mod, self->ref_umi, qin, (void**)&self, jobs_list_run__on_call, err);

exit_and_cleanup:
  
  cape_str_del (&vsec);
  cape_str_del (&h1);

  jobs_list_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int jobs_list_run (JobsList* p_self, QBusM qin, QJobsEvent event, CapeErr err)
{
  int res;
  JobsList self = *p_self;
  
  // check for module and method
  self->ref_mod = cape_str_mv (&(event->ref_mod));
  self->ref_umi = cape_str_mv (&(event->ref_umi));
  
  self->params = cape_udc_mv (&(event->params));
  self->rinfo_encrypted = cape_str_cp (event->rinfo_encrypted);
  
  if (self->ref_mod && self->ref_umi)
  {
    // clean up
    qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
    res = qbus_send (self->qbus, "AUTH", "getVaultSecret", qin, (void*)self, jobs_list_run__on_vsec, err);
    if (res == CAPE_ERR_NONE)
    {
      *p_self = NULL;
    }
  }
  else
  {
    res = CAPE_ERR_NONE;
  }
  
exit_and_cleanup:
  
  jobs_list_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
