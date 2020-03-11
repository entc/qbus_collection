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
};

//-----------------------------------------------------------------------------

QJobs qjobs_new (AdblSession adbl_session, const CapeString database_table)
{
  QJobs self = CAPE_NEW(struct QJobs_s);
  
  self->timer = cape_aio_timer_new ();
  
  self->adbl_session = adbl_session;
  
  self->adbl_table = cape_str_cp (database_table);
  
  return self;
}

//-----------------------------------------------------------------------------

void qjobs_del (QJobs* p_self)
{
  if(*p_self)
  {
    QJobs self = *p_self;
    
    cape_str_del (&(self->adbl_table));
    
    CAPE_DEL (p_self, struct QJobs_s);
  }
}

//-----------------------------------------------------------------------------

int __STDCALL qjobs__on_event (void* ptr)
{
  QJobs self = ptr;
  
  if (self->user_fct)
  {
    
    
  }
  
  return TRUE;  // keep running
}

//-----------------------------------------------------------------------------

int qjobs_init (QJobs self, CapeAioContext aio_ctx, void* user_ptr, fct_qjobs__on_event user_fct, CapeErr err)
{
  int res;
  
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  res = cape_aio_timer_set (self->timer, 5000, self, qjobs__on_event, err);
  if (res)
  {
    return res;
  }
  
  return cape_aio_timer_add (&(self->timer), aio_ctx);
}

//-----------------------------------------------------------------------------

int qjobs_event (QJobs self, CapeDatetime* dt, CapeUdc* p_params, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------
