#include "qbus.h" 
#include <adbl.h>

#include <sys/cape_log.h>
#include <sys/cape_queue.h>

#include "jobs_list.h"

// qjobs includes
#include <qjobs.h>

//-------------------------------------------------------------------------------------

struct JobsContext_s
{
  AdblCtx adbl_ctx;
  AdblSession adbl_session;
  
  QJobs jobs;

}; typedef struct JobsContext_s* JobsContext;

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_jobs__list_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  JobsContext ctx = ptr;
  
  // create a temporary object
  JobsList jobs_list = jobs_list_new (qbus, ctx->adbl_session, ctx->jobs);
  
  // run the command
  return jobs_list_add (&jobs_list, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_jobs__list_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  JobsContext ctx = ptr;
  
  // create a temporary object
  JobsList jobs_list = jobs_list_new (qbus, ctx->adbl_session, ctx->jobs);
  
  // run the command
  return jobs_list_get (&jobs_list, qin, qout, err);
}

//-------------------------------------------------------------------------------------

void qbus_jobs__ctx_del (JobsContext* p_self)
{
  if (*p_self)
  {
    JobsContext self = *p_self;
    
    if (self->adbl_session)
    {
      adbl_session_close (&(self->adbl_session));
    }
    
    if (self->adbl_ctx)
    {
      adbl_ctx_del (&(self->adbl_ctx));
    }
    
    qjobs_del (&(self->jobs));
    
    CAPE_DEL (p_self, struct JobsContext_s);
  }
}

//-------------------------------------------------------------------------------------

CapeString __STDCALL qbus_jobs__on_event (void* user_ptr, CapeUdc params, number_t wpid, number_t gpid, const CapeString rinfo_encrypted)
{
  JobsContext ctx = user_ptr;
  
  
  
  return NULL;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_jobs_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  JobsContext ctx = CAPE_NEW (struct JobsContext_s);

  ctx->adbl_ctx = NULL;
  ctx->adbl_session = NULL;

  ctx->adbl_ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
  if (ctx->adbl_ctx == NULL)
  {
    goto exit_and_cleanup;
  }
  
  ctx->adbl_session = adbl_session_open_file (ctx->adbl_ctx, "adbl_default.json", err);
  if (ctx->adbl_session == NULL)
  {
    goto exit_and_cleanup;
  }
  
  ctx->jobs = qjobs_new (ctx->adbl_session, "jobs_list");
  
  res = qjobs_init (ctx->jobs, qbus_aio (qbus), 1000, ctx, qbus_jobs__on_event, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  *p_ptr = ctx;

  // -------- callback methods --------------------------------------------
  
  // add a new event to the list
  //   args: 
  qbus_register (qbus, "list_add"       , ctx, qbus_jobs__list_add, NULL, err);

  // get all job events
  //   args: 
  qbus_register (qbus, "list_get"       , ctx, qbus_jobs__list_get, NULL, err);

  // -------- callback methods --------------------------------------------

  return CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_log_msg (CAPE_LL_ERROR, "JOBS", "init", cape_err_text (err));
  
  qbus_jobs__ctx_del (&ctx);
  *p_ptr = NULL;
  
  return cape_err_code (err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_jobs_done (QBus qbus, void* ptr, CapeErr err)
{
  JobsContext ctx = ptr;
  
  qbus_jobs__ctx_del (&ctx);
  
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("JOBS", NULL, qbus_jobs_init, qbus_jobs_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
