#include "jobs_sched.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

struct JobsScheduler_s
{
  QBus qbus;                          // reference
  AdblSession adbl_session;           // reference
  
  CapeAioTimer timer;
  
  number_t last_update;
  
  CapeUdc list;
};

//-----------------------------------------------------------------------------

JobsScheduler jobs_sched_new (QBus qbus, AdblSession adbl_session)
{
  JobsScheduler self = CAPE_NEW(struct JobsScheduler_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;

  self->timer = cape_aio_timer_new ();
  self->last_update = 0;
  
  self->list = NULL;

  return self;
}

//-----------------------------------------------------------------------------

void jobs_sched_del (JobsScheduler* p_self)
{
  if (*p_self)
  {
    JobsScheduler self = *p_self;
    
    cape_udc_del (&(self->list));
    
    CAPE_DEL (p_self, struct JobsScheduler_s);
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL jobs_sched__on_event__on_vsec (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  JobsScheduler self = ptr;

  // local objects
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
  

  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_str_del (&vsec);
  
  return res;
}

//-----------------------------------------------------------------------------

void jobs_sched__fetch (JobsScheduler self, CapeErr err)
{
  CapeUdc query_results = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_b      (params, "active"      , TRUE);
    
    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_d      (values, "event_start" , NULL);
    cape_udc_add_d      (values, "event_stop"  , NULL);
    cape_udc_add_b      (values, "repeated"    , FALSE);
    cape_udc_add_s_cp   (values, "modp"        , NULL);
    cape_udc_add_n      (values, "ref"         , 0);
    cape_udc_add_s_cp   (values, "data"        , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "jobs_list", &params, &values, err);
    if (query_results == NULL)
    {
      goto exit_and_cleanup;
    }
    
    cape_udc_replace_mv (&(self->list), &query_results);
  }
  
exit_and_cleanup:

  cape_udc_del (&query_results);
}

//-----------------------------------------------------------------------------

int __STDCALL jobs_sched__on_event (void* ptr)
{
  JobsScheduler self = ptr;
  
  if (self->last_update == 0)
  {
    CapeErr err = cape_err_new ();

    jobs_sched__fetch (self, err);
    
    self->last_update = 1;

    printf ("TRY #1\n");
    
    QBusM msg = qbus_message_new (NULL, NULL);
    
    // fetch last entries from database
    // first we need the vsec
    qbus_send (self->qbus, "AUTH", "getVaultSecret", msg, self, jobs_sched__on_event__on_vsec, err);

    cape_err_del (&err);
  }
  
  printf ("----------------------------------\n");
  
  {
    CapeDatetime dt;
    cape_datetime_utc (&dt);
    
    CapeUdcCursor* cursor = cape_udc_cursor_new (self->list, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      const CapeDatetime* event_start = cape_udc_get_d (cursor->item, "event_start", NULL);
      if (event_start)
      {
        printf ("time runs out: %i\n", cape_datetime_cmp (event_start, &dt));

        
        
        if (cape_datetime_cmp (event_start, &dt) < 0)
        {
          CapeString h = cape_json_to_s (cursor->item);
          
          
          printf ("time runs out: %s\n", h);
        }
      }
      else
      {
        
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  return TRUE;  // keep running
}

//-----------------------------------------------------------------------------

int jobs_sched_init (JobsScheduler self, QBus qbus, CapeErr err)
{
  int res;
  
  res = cape_aio_timer_set (self->timer, 5000, self, jobs_sched__on_event, err);
  if (res)
  {
    return res;
  }
  
  return cape_aio_timer_add (&(self->timer), qbus_aio (qbus));
}

//-----------------------------------------------------------------------------

int jobs_sched_add (JobsScheduler self, CapeErr err)
{
  self->last_update = 0;
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
