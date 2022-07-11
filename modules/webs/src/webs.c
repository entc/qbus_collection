#include "webs_post.h"
#include "webs_json.h"
#include "webs_enjs.h"
#include "webs_imca.h"

#include "qbus.h"

// qwebs includes
#include "qwebs.h"
#include "qwebs_connection.h"
#include "qwebs_multipart.h"

// qcrypt includes
#include <qcrypt.h>

// cape includes
#include <aio/cape_aio_ctx.h>
#include <sys/cape_err.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_tokenizer.h>
#include <fmt/cape_json.h>
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

struct WebsContext_s
{
  QWebs webs;
  WebsStream s;
  
}; typedef struct WebsContext_s* WebsContext;

//-----------------------------------------------------------------------------

static void __STDCALL webs__files__on_del (void* ptr)
{
  CapeString file = ptr;
  
  cape_fs_file_rm (file, NULL);
  
  cape_str_del (&file);
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__json (void* user_ptr, QWebsRequest request, CapeErr err)
{
  WebsJson webs_json = webs_json_new (user_ptr, request);
  
  return webs_json_run (&webs_json, err);  
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__enjs (void* user_ptr, QWebsRequest request, CapeErr err)
{
  WebsEnjs webs_enjs = webs_enjs_new (user_ptr, request);
  
  return webs_enjs_run (&webs_enjs, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__rest (void* user_ptr, QWebsRequest request, CapeErr err)
{
  CapeList url_parts = qwebs_request_clist (request);
  if (cape_list_size (url_parts) >= 1)
  {
    const CapeString module = NULL;
    const CapeString method = NULL;
    
    CapeUdc clist;
    CapeString token = NULL;
    
    CapeListCursor* cursor = cape_list_cursor_create (url_parts, CAPE_DIRECTION_FORW);
    
    if (cape_list_cursor_next (cursor))
    {
      module = cape_list_node_data (cursor->node);
    }
    
    method = qwebs_request_method (request);
    
    // check for token
    {
      const CapeString special = NULL;
      CapeListCursor c2;
      
      // clone cursor
      c2.direction = cursor->direction;
      c2.node = cursor->node;
      c2.position = cursor->position;
      
      if (cape_list_cursor_next (&c2))
      {
        special = cape_list_node_data (c2.node);
        
        if (cape_str_equal (special, "__T"))
        {
          cape_list_cursor_next (cursor);
          
          // do we have a token?
          if (cape_list_cursor_next (&c2))
          {
            cape_list_cursor_next (cursor);
            
            token = cape_str_cp (cape_list_node_data (c2.node));
          }
        }
      }
    }
    
    clist = cape_udc_new (CAPE_UDC_LIST, NULL);
    
    while (cape_list_cursor_next (cursor))
    {
      cape_udc_add_s_cp (clist, NULL, cape_list_node_data (cursor->node));
    }
    
    cape_list_cursor_destroy (&cursor);
    
    if (module && method)
    {
      WebsJson webs_json = webs_json_new (user_ptr, request);
      
      webs_json_set (webs_json, module, method, &clist, &token);

      // check for authentication
      return webs_json_run_gen (&webs_json, err);
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__imca (void* user_ptr, QWebsRequest request, CapeErr err)
{
  return webs_stream_get (user_ptr, request, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__post (void* user_ptr, QWebsRequest request, CapeErr err)
{
  WebsPost webs_post = webs_post_new (user_ptr, request);
  
  return webs_post_run (&webs_post, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__modules_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  // get a list of all known modules in the qbus subsystem
  qout->cdata = qbus_modules (qbus);
  
  // debug
  {
    CapeString h = cape_json_to_s (qout->cdata);
    
    cape_log_fmt (CAPE_LL_TRACE, "WSRV", "modules get", "got json: %s", h);
    
    cape_str_del (&h);
  }
  
  return CAPE_ERR_NONE;  
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__stream_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  return webs_stream_add (ptr, qin, qout, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__stream_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  return webs_stream_set (ptr, qin, qout, err);
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__stream__on_timer (void* ptr)
{
  webs_stream_reset (ptr);
  return TRUE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__restapi_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  // get a list of all known modules in the qbus subsystem
  qout->cdata = qbus_modules (qbus);
  
  // debug
  {
    CapeString h = cape_json_to_s (qout->cdata);
    
    cape_log_fmt (CAPE_LL_TRACE, "WSRV", "modules get", "got rest: %s", h);
    
    cape_str_del (&h);
  }
  
  return CAPE_ERR_NONE;  
}

//-------------------------------------------------------------------------------------

int __STDCALL qbus_webs__on_raise (void* user_ptr, number_t type, QWebsRequest request)
{
  switch (type)
  {
    case QWEBS_RAISE_TYPE__MINOR:
    {
      qbus_log_msg (user_ptr, qwebs_request_remote (request), "access file forbidden");
      break;
    }
    case QWEBS_RAISE_TYPE__CRITICAL:
    {
      qbus_log_msg (user_ptr, qwebs_request_remote (request), "access file critical");
      break;
    }
  }
}

//-------------------------------------------------------------------------------------

WebsContext qbus_webs__context_new ()
{
  WebsContext self = CAPE_NEW (struct WebsContext_s);
  
  self->webs = NULL;
  self->s = webs_stream_new ();
    
  return self;
}

//-------------------------------------------------------------------------------------

void qbus_webs__context_del (WebsContext* p_self)
{
  if (*p_self)
  {
    WebsContext self = *p_self;
    
    qwebs_del (&(self->webs));
    webs_stream_del (&(self->s));
    
    CAPE_DEL (p_self, struct WebsContext_s);
  }
}

//-------------------------------------------------------------------------------------

int qbus_webs__context_init (WebsContext self, QBus qbus, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc sites = cape_udc_cp (qbus_config_node (qbus, "sites"));
  
  if (sites)
  {
    const CapeString site = qbus_config_s (qbus, "site", "public");
    if (cape_str_not_empty (site))
    {
      cape_udc_add_s_cp (sites, "/", site);
    }
  }
  
  const CapeString host = qbus_config_s (qbus, "host", "127.0.0.1");
  
  // this is the directory to find error pages
  const CapeString pages = qbus_config_s (qbus, "pages", "pages");
  
  number_t port = qbus_config_n (qbus, "port", 8082);
  number_t threads = qbus_config_n (qbus, "threads", 4);
  
  CapeUdc route_list = qbus_config_node (qbus, "route_list");
  
  const CapeString identifier = qbus_config_s (qbus, "identifier", "QWebs");
  const CapeString provider = qbus_config_s (qbus, "provider", "QBUS - Webs Module");
  
  // create a new QWEBS instance
  self->webs = qwebs_new (sites, host, port, threads, pages, route_list, identifier, provider);
  
  res = qwebs_reg (self->webs, "json", qbus, qbus_webs__json, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qwebs_reg (self->webs, "enjs", qbus, qbus_webs__enjs, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qwebs_reg (self->webs, "rest", qbus, qbus_webs__rest, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qwebs_reg (self->webs, "imca", self->s, qbus_webs__imca, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qwebs_reg (self->webs, "post", qbus, qbus_webs__post, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qwebs_attach (self->webs, qbus_aio (qbus), err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // register a callback in case a security issue was reported
  qwebs_set_raise (self->webs, qbus, qbus_webs__on_raise);
  
exit_and_cleanup:
  
  cape_udc_del (&sites);
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_webs_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  
  // local objects
  WebsContext ctx = qbus_webs__context_new ();

  // run the init procedure
  res = qbus_webs__context_init (ctx, qbus, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // --------- RESTAPI callbacks -----------------------------
  
  qbus_register (qbus, "GET", ctx->webs, qbus_webs__restapi_get, NULL, err);
  
  // --------- register callbacks -----------------------------
  
  qbus_register (qbus, "modules_get", ctx->webs, qbus_webs__modules_get, NULL, err);
  
  qbus_register (qbus, "stream_add", ctx->s, qbus_webs__stream_add, NULL, err);
  qbus_register (qbus, "stream_set", ctx->s, qbus_webs__stream_set, NULL, err);

  // --------- register callbacks -----------------------------
  
  *p_ptr = ctx;
  ctx = NULL;
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  qbus_webs__context_del (&ctx);
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_webs_done (QBus qbus, void* ptr, CapeErr err)
{
  WebsContext ctx = ptr;
  
  qbus_webs__context_del (&ctx);
  
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("WEBS", NULL, qbus_webs_init, qbus_webs_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
