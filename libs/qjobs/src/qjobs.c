#include "qjobs.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <aio/cape_aio_timer.h>

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
    cape_udc_add_n      (values, "repeats"     , 0);
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

int qjobs__intern__event_update (QJobs self, number_t id, const CapeString next_date, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  
  CapeDatetime dt;
  
  cape_datetime__add_s (self->next_event, next_date, &dt);
  
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

int qjobs__intern__event_run (QJobs self, CapeString* p_next_run, CapeErr err)
{
  cape_log_msg (CAPE_LL_TRACE, "QJOBS", "jobs loop", "event!");
  
  number_t wpid = cape_udc_get_n (self->next_list_item, "wpid", 0);
  number_t gpid = cape_udc_get_n (self->next_list_item, "gpid", 0);

  number_t r1id = cape_udc_get_n (self->next_list_item, "ref_id1", 0);
  number_t r2id = cape_udc_get_n (self->next_list_item, "ref_id2", 0);

  const CapeString rinfo = cape_udc_get_s (self->next_list_item, "rinfo", NULL);
  
  if (self->user_fct)
  {
    *p_next_run = self->user_fct (self->user_ptr, cape_udc_get (self->next_list_item, "params"), wpid, gpid, rinfo, r1id, r2id);
  }
  
  return CAPE_ERR_NONE;
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
      CapeString next_run = NULL;
      
      res = qjobs__intern__event_run (self, &next_run, err);
      
      if (next_run == NULL)
      {
        res = qjobs__intern__event_rm (self, cape_udc_get_n (self->next_list_item, "id", 0), err);
      }
      else
      {
        res = qjobs__intern__event_update (self, cape_udc_get_n (self->next_list_item, "id", 0), next_run, err);
      }

      cape_str_del (&next_run);
      
      res = qjobs__intern__fetch (self, err);
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

int qjobs_event (QJobs self, CapeDatetime* dt, number_t repeats, CapeUdc* p_params, CapeUdc rinfo, const CapeString ref_mod, const CapeString ref_umi, number_t ref_id1, number_t ref_id2, const CapeString vsec, CapeErr err)
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
  cape_udc_add_n (values, "repeats", repeats);
  
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
  
  // finally add it to the database
  res = qjobs__intern__event_add (self, dt, &values, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qjobs__intern__fetch (self, err);
  
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
