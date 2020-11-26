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

int __STDCALL main_on_page (void* user_ptr, QWebsRequest request, CapeErr err)
{
  printf ("ON PAGE\n");
  
  // get the dictonary of the query part of the request
  CapeMap query_values = qwebs_request_query (request);
  
  // test with http://127.0.0.1:8080/hidden.html?M0:O1=123
  
  // check for a certain key "M0:O1"
  CapeMapNode n = cape_map_find (query_values, "M0:O1");
  if (n)
  {
    // retrieve the value
    const CapeString value = (const CapeString)cape_map_node_value (n);
    
    qwebs_request_send_buf (&request, value, "text/html", err);
  }
  else
  {
    qwebs_request_send_buf (&request, "<h1>hidden callback</h1>", "text/html", err);
  }
  
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  // local objects
  CapeAioContext aio_context = NULL;
  QWebs webs = NULL;
  CapeUdc sites = NULL;
  
  // allocate memory for the AIO subsystem
  aio_context = cape_aio_context_new ();
  
  // try to start the AIO
  res = cape_aio_context_open (aio_context, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // enable interupt by ctrl-c
  res = cape_aio_context_set_interupts (aio_context, CAPE_AIO_ABORT, CAPE_AIO_ABORT, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // create a node to contain all sites
  sites = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  // add the default site 'public'
  cape_udc_add_s_cp (sites, "/", "public");

  // allocate memory and initialize the qwebs library context
  webs = qwebs_new (sites, "127.0.0.1", 8080, 4, "pages", NULL);
  
  // register an API which can be called by http://127.0.0.1/json/
  res = qwebs_reg (webs, "json", NULL, main_on_json, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qwebs_reg_page (webs, "hidden.html", NULL, main_on_page, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // finally attach the qwebs library to the AIO subsystem
  // -> this will enable all events
  // -> at this point request can be processes
  res = qwebs_attach (webs, aio_context, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // handle all events
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

