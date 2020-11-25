#include "qwebs.h"
#include "qwebs_connection.h"

// cape includes
#include <aio/cape_aio_ctx.h>
#include <sys/cape_err.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

#include <stdio.h>

//-----------------------------------------------------------------------------

int __STDCALL main_on_json (void* user_ptr, QWebsRequest request, CapeErr err)
{
  printf ("ON JSON\n");
 
  qwebs_request_send_json (&request, NULL, err);
  
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  // local objects
  CapeAioContext aio_context = cape_aio_context_new ();
  QWebs webs = NULL;
  CapeUdc sites = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  res = cape_aio_context_open (aio_context, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_aio_context_set_interupts (aio_context, CAPE_AIO_ABORT, CAPE_AIO_ABORT, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_udc_add_s_cp (sites, "/", "public");
  
  webs = qwebs_new (sites, "127.0.0.1", 8082, 4, "pages", NULL);
  
  res = qwebs_reg (webs, "json", NULL, main_on_json, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qwebs_attach (webs, aio_context, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_aio_context_wait (aio_context, err);
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "QWEBS", "main", "fetched error: %s", cape_err_text (err));
  }
  
  cape_udc_del (&sites);
  qwebs_del (&webs);
  cape_aio_context_del (&aio_context);
  
  cape_err_del (&err);
  
  return res;
}

//-----------------------------------------------------------------------------

