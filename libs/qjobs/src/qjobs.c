#include "qjobs.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <aio/cape_aio_timer.h>
#include <sys/cape_mutex.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct QJobs_s
{
  CapeAioTimer timer;
  
  AdblSession adbl_session;   // reference
  
  CapeString adbl_table;
  
  void* user_ptr;
  fct_qjobs__on_event user_fct;
  
  CapeUdc list;
  CapeMutex mutex;

  const CapeDatetime* next_event;
  CapeUdc next_list_item;
  
};

//-----------------------------------------------------------------------------

QJobs qjobs_new (AdblSession adbl_session, const CapeString database_table)
{
  QJobs self = CAPE_NEW(struct QJobs_s);
  
  self->timer = cape_aio_timer_new ();
  
  self->adbl_session = adbl_session;
  
  self->adbl_table = cape_str_cp (database_table);
  
  self->list = NULL;
  self->mutex = cape_mutex_new ();
  
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
    cape_mutex_del (&(self->mutex));
    
    CAPE_DEL (p_self, struct QJobs_s);
  }
}

//-----------------------------------------------------------------------------

void qjobs__intern__next_event (QJobs self)
{
  CapeUdcCursor* cursor = cape_udc_cursor_new (self->list, CAPE_DIRECTION_FORW);
  
  self->next_event = NULL;
  self->next_list_item = NULL;
  
  cape_log_fmt (CAPE_LL_TRACE, "QJOBS", "next event", "seek new event");
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeDatetime* dt = cape_udc_get_d (cursor->item, "event_date", NULL);
    if (dt)
    {
      if (self->next_event)
      {
        if (cape_datetime_cmp (dt, self->next_event) < 0)
        {
          self->next_event = dt;
          self->next_list_item = cursor->item;
        }
      }
      else
      {
        self->next_event = dt;
        self->next_list_item = cursor->item;
      }
    }
  }
  
  if (self->next_event)
  {
    CapeString h = cape_datetime_s__str (self->next_event);
    
    cape_log_fmt (CAPE_LL_TRACE, "QJOBS", "next event", "found next event = %s", h);
    
    cape_str_del (&h);
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "QJOBS", "next event", "no event found");
  }
  
  cape_udc_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

int qjobs__intern__fetch (QJobs self, CapeErr err)
{
  int res;
  CapeUdc query_results = NULL;
  
  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (values, "id"          , 0);

    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_n      (values, "gpid"        , 0);

    cape_udc_add_d      (values, "event_date"  , NULL);
    cape_udc_add_n      (values, "period"      , 0);
    cape_udc_add_node   (values, "params"      );
    cape_udc_add_s_cp   (values, "rinfo"       , NULL);
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
  
  qjobs__intern__next_event (self);
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs__intern__event_add (QJobs self, CapeUdc* p_values, CapeErr err)
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

int qjobs__intern__event_set (QJobs self, CapeUdc* p_params, CapeUdc* p_values, CapeErr err)
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
  res = adbl_trx_update (adbl_trx, self->adbl_table, p_params, p_values, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs__intern__event_rm (QJobs self, number_t id, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (params, "id", id);
    
    // execute query
    res = adbl_trx_delete (adbl_trx, self->adbl_table, &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs__intern__event_update (QJobs self, CapeDatetime* current, number_t id, number_t period, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  
  CapeDatetime dt;
  cape_datetime__add_n (self->next_event, period, &dt);

  // check if the time is in the future
  if (cape_datetime_cmp (&dt, current) <= 0)
  {
    time_t nn = cape_datetime_n__unix (current);
    time_t nv = cape_datetime_n__unix (self->next_event);
    
    time_t delta = nn - nv;
    
    number_t multi_period = (number_t)((double)delta / period + 1);
    
    cape_log_fmt (CAPE_LL_TRACE, "QJOBS", "event update", "adjust event date into future: %i times", multi_period);
    
    cape_datetime__add_n (self->next_event, multi_period * period, &dt);
  }
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (params, "id", id);
    cape_udc_add_d (values, "event_date", &dt);
    cape_udc_add_d (values, "preev_date", self->next_event);

    // execute query
    res = adbl_trx_update (adbl_trx, self->adbl_table, &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs__intern__event_run (QJobs self, number_t rpid, CapeErr err)
{
  int res = CAPE_ERR_EOF;
  cape_log_msg (CAPE_LL_TRACE, "QJOBS", "jobs loop", "event!");
  
  number_t wpid = cape_udc_get_n (self->next_list_item, "wpid", 0);
  number_t gpid = cape_udc_get_n (self->next_list_item, "gpid", 0);

  if (self->user_fct)
  {
    struct QJobsEvent_s event;
    
    event.rpid = rpid;
    event.wpid = wpid;
    event.gpid = gpid;
    
    event.params = cape_udc_ext (self->next_list_item, "params");
    event.rinfo_encrypted = cape_udc_get_s (self->next_list_item, "rinfo", NULL);
    
    event.r1id = cape_udc_get_n (self->next_list_item, "ref_id1", 0);
    event.r2id = cape_udc_get_n (self->next_list_item, "ref_id2", 0);
    
    event.ref_mod = cape_udc_ext_s (self->next_list_item, "ref_mod");
    event.ref_umi = cape_udc_ext_s (self->next_list_item, "ref_umi");

    res = self->user_fct (self, &event, self->user_ptr, err);
    
    cape_str_del (&(event.ref_mod));
    cape_str_del (&(event.ref_umi));
    
    cape_udc_del (&(event.params));
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qjobs__intern__run (QJobs self, number_t rpid, number_t period, CapeDatetime* dt, CapeErr err)
{
  int res = qjobs__intern__event_run (self, rpid, err);
  
  switch (res)
  {
    case CAPE_ERR_EOF:
    {
      res = qjobs__intern__event_rm (self, rpid, err);
      break;
    }
    case CAPE_ERR_NONE:
    {
      res = qjobs__intern__event_update (self, dt, rpid, period, err);
      break;
    }
  }
  
  res = qjobs__intern__fetch (self, err);
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qjobs__on_event (void* ptr)
{
  int res;
  QJobs self = ptr;
  
  CapeErr err = cape_err_new ();
  
  cape_mutex_lock (self->mutex);
  
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
      res = qjobs__intern__run (self, cape_udc_get_n (self->next_list_item, "id", 0), cape_udc_get_n (self->next_list_item, "period", 0), &dt, err);

        /*
      res = qjobs__intern__event_run (self, cape_udc_get_n (self->next_list_item, "id", 0), err);
      
      switch (res)
      {
        case CAPE_ERR_EOF:
        {
          res = qjobs__intern__event_rm (self, cape_udc_get_n (self->next_list_item, "id", 0), err);
          break;
        }
        case CAPE_ERR_NONE:
        {
          res = qjobs__intern__event_update (self, &dt, cape_udc_get_n (self->next_list_item, "id", 0), cape_udc_get_n (self->next_list_item, "period", 0), err);
          break;
        }
      }
      
      res = qjobs__intern__fetch (self, err);
      */
    }
  }
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QJOBS", "jobs loop", cape_err_text (err));
  }
  
  cape_err_del (&err);
  
  cape_mutex_unlock (self->mutex);

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

int qjobs_add (QJobs self, const CapeDatetime* dt, number_t period, CapeUdc* p_params, CapeUdc rinfo, const CapeString ref_mod, const CapeString ref_umi, number_t ref_id1, number_t ref_id2, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeString h1 = NULL;
  CapeString h2 = NULL;

  if (rinfo)
  {
    // rinfo contains personal data -> this must be encrypted
    if (vsec == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "can't create event with rinfo and no vsec");
      goto exit_and_cleanup;
    }
    
    // add workspace and user info (this is needed to decrypt rinfo)
    cape_udc_add_n (values, "wpid", cape_udc_get_n (rinfo, "wpid", 0));
    cape_udc_add_n (values, "gpid", cape_udc_get_n (rinfo, "gpid", 0));

    // serialize rinfo
    h1 = cape_json_to_s (rinfo);
    if (h1 == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize rinfo");
      goto exit_and_cleanup;
    }
    
    // encrypt rinfo
    h2 = qcrypt__encrypt (vsec, h1, err);
    if (h2 == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
    
    // finally add to the values
    cape_udc_add_s_mv (values, "rinfo", &h2);
  }
  
  cape_udc_add_d (values, "event_date", dt);
  cape_udc_add_n (values, "period", period);
  
  if (p_params)
  {
    cape_udc_add_name (values, p_params, "params");
  }

  if (ref_mod)
  {
    cape_udc_add_s_cp (values, "ref_mod", ref_mod);
  }
  
  if (ref_umi)
  {
    cape_udc_add_s_cp (values, "ref_umi", ref_umi);
  }

  if (ref_id1)
  {
    cape_udc_add_n (values, "ref_id1", ref_id1);
  }
  
  if (ref_id2)
  {
    cape_udc_add_n (values, "ref_id2", ref_id2);
  }
  
  if (rinfo)
  {
    
  }
  
  cape_mutex_lock (self->mutex);

  // finally add it to the database
  res = qjobs__intern__event_add (self, &values, err);
  if (res)
  {
    cape_mutex_unlock (self->mutex);
    goto exit_and_cleanup;
  }
  
  res = qjobs__intern__fetch (self, err);

  cape_mutex_unlock (self->mutex);

exit_and_cleanup:

  if (p_params)
  {
    cape_udc_del (p_params);
  }
  
  cape_str_del (&h1);
  cape_str_del (&h2);
  cape_udc_del (&values);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs_set (QJobs self, number_t rpid, CapeDatetime* dt, number_t period_in_s, CapeUdc* p_params, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  // add params
  cape_udc_add_n (params, "id", rpid);
  
  if (dt)
  {
    cape_udc_add_d (values, "event_date", dt);
  }
  
  if (p_params)
  {
    cape_udc_add_name (values, p_params, "params");
  }
  
  cape_mutex_lock (self->mutex);

  // finally update it in the database
  res = qjobs__intern__event_set (self, &params, &values, err);
  if (res)
  {
    cape_mutex_unlock (self->mutex);
    goto exit_and_cleanup;
  }
  
  res = qjobs__intern__fetch (self, err);
  
  cape_mutex_unlock (self->mutex);

exit_and_cleanup:
  
  if (p_params)
  {
    cape_udc_del (p_params);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qjobs_rm (QJobs self, number_t jobid, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx trx = NULL;

  trx = adbl_trx_new (self->adbl_session, err);
  if (trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n (params, "id", jobid);
    
    res = adbl_trx_delete (trx, self->adbl_table, &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&trx, err);
  
  res = qjobs__intern__fetch (self, err);

exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int qjobs_run (QJobs self, number_t rpid, CapeErr err)
{
  int res;
  
  CapeUdc first_row;

  // local objects
  CapeUdc query_results = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"          , rpid);
    cape_udc_add_n      (values, "id"          , 0);
    
    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_n      (values, "gpid"        , 0);
    
    cape_udc_add_d      (values, "event_date"  , NULL);
    cape_udc_add_n      (values, "period"      , 0);
    cape_udc_add_node   (values, "params"      );
    cape_udc_add_s_cp   (values, "rinfo"       , NULL);
    cape_udc_add_s_cp   (values, "ref_mod"     , NULL);
    cape_udc_add_s_cp   (values, "ref_umi"     , NULL);
    cape_udc_add_n      (values, "ref_id1"     , 0);
    cape_udc_add_n      (values, "ref_id2"     , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, self->adbl_table, &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }    
  }
  
  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NOT_FOUND");
    goto exit_and_cleanup;
  }
  
  cape_udc_replace_mv (&(self->list), &query_results);
  self->next_list_item = first_row;
  
  {
    CapeDatetime dt;
    
    // fetch current date
    cape_datetime_utc (&dt);
    
    res = qjobs__intern__run (self, cape_udc_get_n (self->next_list_item, "id", 0), cape_udc_get_n (self->next_list_item, "period", 0), &dt, err);    
  }

exit_and_cleanup:

  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

void qjobs_continue (QJobs self, number_t rpid, number_t multiplier)
{
  

}

//-----------------------------------------------------------------------------

/*
CREATE TABLE `jobs_list` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
                          `wpid` int(10),
                          `gpid` int(10),
                          `event_date` datetime NOT NULL,
                          `repeats` int(10) NOT NULL,
                          `rinfo` varchar(2000) DEFAULT NULL,
                          `params` varchar(2000) DEFAULT NULL,
                          `ref_mod` varchar(8) DEFAULT NULL,
                          `ref_umi` varchar(30) DEFAULT NULL,
                          `ref_id1` int(10) DEFAULT NULL,
                          `ref_id2` int(10) DEFAULT NULL,
                          PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=14 DEFAULT CHARSET=utf8
*/
