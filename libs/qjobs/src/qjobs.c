#include "qjobs.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

struct QJobs_s
{
  CapeAioTimer timer;
  
  AdblSession adbl_session;   // reference
  
  CapeString adbl_table;
  
  void* user_ptr;
  fct_qjobs__on_event user_fct;
  
  CapeUdc list;
  
  CapeDatetime* next_event;
};

//-----------------------------------------------------------------------------

QJobs qjobs_new (AdblSession adbl_session, const CapeString database_table)
{
  QJobs self = CAPE_NEW(struct QJobs_s);
  
  self->timer = cape_aio_timer_new ();
  
  self->adbl_session = adbl_session;
  
  self->adbl_table = cape_str_cp (database_table);
  
  self->list = NULL;
  self->next_event = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qjobs_del (QJobs* p_self)
{
  if(*p_self)
  {
    QJobs self = *p_self;
    
    cape_str_del (&(self->adbl_table));
    cape_udc_del (&(self->list));
    
    CAPE_DEL (p_self, struct QJobs_s);
  }
}

//-----------------------------------------------------------------------------

int qjobs__intern__fetch (QJobs self, CapeErr err)
{
  int res;
  CapeUdc query_results = NULL;
  
  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_n      (values, "gpid"        , 0);

    cape_udc_add_d      (values, "event_date " , NULL);
    cape_udc_add_n      (values, "repeats"     , 0);
    cape_udc_add_node   (values, "params"      );
    cape_udc_add_s_cp   (values, "ref_mod"     , NULL);
    cape_udc_add_s_cp   (values, "ref_umi"     , NULL);
    cape_udc_add_n      (values, "ref_id1"     , 0);
    cape_udc_add_n      (values, "ref_id2"     , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, self->adbl_table, NULL, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
    
    cape_udc_replace_mv (&(self->list), &query_results);
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs__intern__event_add (QJobs self, CapeDatetime* dt, CapeUdc* p_values, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  number_t id;
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // execute query
  id = adbl_trx_insert (adbl_trx, self->adbl_table, p_values, err);
  if (id == 0)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qjobs__on_event (void* ptr)
{
  int res;
  QJobs self = ptr;
  
  CapeErr err = cape_err_new ();
    
  if (self->list == NULL)
  {
    cape_log_msg (CAPE_LL_TRACE, "QJOBS", "jobs loop", "fetch from database");
    
    res = qjobs__intern__fetch (self, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  if (self->next_event)
  {
    CapeDatetime dt;
    
    // fetch current date
    cape_datetime_utc (&dt);
    
    // if next_event is in the past, trigger that event
    if (cape_datetime_cmp (self->next_event, &dt) <= 0)
    {
      cape_log_msg (CAPE_LL_TRACE, "QJOBS", "jobs loop", "event!");
      
      self->next_event = NULL;
    }
  }
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QJOBS", "jobs loop", cape_err_text (err));
  }
  
  cape_err_del (&err);
  
  return TRUE;  // keep running
}

//-----------------------------------------------------------------------------

int qjobs_init (QJobs self, CapeAioContext aio_ctx, number_t precision_in_ms, void* user_ptr, fct_qjobs__on_event user_fct, CapeErr err)
{
  int res;
  
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  res = cape_aio_timer_set (self->timer, precision_in_ms, self, qjobs__on_event, err);
  if (res)
  {
    return res;
  }
  
  return cape_aio_timer_add (&(self->timer), aio_ctx);
}

//-----------------------------------------------------------------------------

int qjobs_event (QJobs self, CapeDatetime* dt, number_t repeats, CapeUdc* p_params, const CapeString ref_mod, const CapeString ref_umi, number_t ref_id1, number_t ref_id2, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_d (values, "event_date", dt);
  cape_udc_add_n (values, "repeats", repeats);
  
  cape_udc_add_name (values, p_params, "params");

  cape_udc_add_s_cp (values, "ref_mod", ref_mod);
  cape_udc_add_s_cp (values, "ref_umi", ref_umi);

  cape_udc_add_n (values, "ref_id1", ref_id1);
  cape_udc_add_n (values, "ref_id2", ref_id2);
  
  // finally add it to the database
  res = qjobs__intern__event_add (self, dt, &values, err);
  
exit_and_cleanup:

  cape_udc_del (p_params);
  cape_udc_del (&values);
  return res;
}

//-----------------------------------------------------------------------------
