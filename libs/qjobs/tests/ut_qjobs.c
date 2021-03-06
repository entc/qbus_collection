#include "qjobs.h"

#include <aio/cape_aio_ctx.h>

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  // local objects
  CapeAioContext aio = cape_aio_context_new ();
  
  AdblCtx adbl_ctx;
  AdblSession adbl_session;
  
  QJobs jobs = NULL;
  
  res = cape_aio_context_open (aio, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_aio_context_set_interupts (aio, CAPE_AIO_ABORT, CAPE_AIO_ABORT, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  adbl_ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
  if (adbl_ctx == NULL)
  {
    goto exit_and_cleanup;
  }
  
  adbl_session = adbl_session_open_file (adbl_ctx, "adbl_default.json", err);
  if (adbl_session == NULL)
  {
    goto exit_and_cleanup;
  }
  
  jobs = qjobs_new (adbl_session, "jobs_list");
  
  res = qjobs_init (jobs, aio, 500, NULL, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  {
    CapeDatetime dt; cape_datetime_utc__add_s (&dt, "m1");

    CapeString h = cape_datetime_s__std_msec (&dt);
    
    printf ("NEXT EVENT: %s\n", h);
    
    res = qjobs_add (jobs, &dt, 0, NULL, NULL, NULL, NULL, 0, 0, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = cape_aio_context_wait (aio, err);
  
exit_and_cleanup:
  
  qjobs_del (&jobs);
  
  cape_aio_context_del (&aio);
  
  cape_err_del (&err);
  
  return res;
}
