#include "qbus.h" 
#include <adbl.h>

#include <sys/cape_log.h>
#include <sys/cape_queue.h>

#include "jobs_sched.h"
#include "jobs_list.h"

//-------------------------------------------------------------------------------------

struct JobsContext_s
{
  AdblCtx adbl_ctx;
  AdblSession adbl_session;
  
  JobsScheduler scheduler;

}; typedef struct JobsContext_s* JobsContext;

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_jobs__list_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  JobsContext ctx = ptr;
  
  // create a temporary object
  JobsList jobs_list = jobs_list_new (qbus, ctx->adbl_session, ctx->scheduler);
  
  // run the command
  return jobs_list_add (&jobs_list, qin, qout, err);
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
    
    jobs_sched_del (&(self->scheduler));
    
    CAPE_DEL (p_self, struct JobsContext_s);
  }
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
  
  ctx->scheduler = jobs_sched_new (qbus, ctx->adbl_session);
  
  res = jobs_sched_init (ctx->scheduler, qbus, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  *p_ptr = ctx;

  // -------- callback methods --------------------------------------------
  
  // validate all email addresses
  //   args: recipients
  qbus_register (qbus, "list_add"       , ctx, qbus_jobs__list_add, NULL, err);

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
