#include "webs_post.h"
#include "webs_json.h"

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

static void __STDCALL webs__files__on_del (void* ptr)
{
  CapeString file = ptr;
  
  cape_fs_file_del (file, NULL);
  
  cape_str_del (&file);
}

//-----------------------------------------------------------------------------

struct WebsStream_s
{
  CapeString image;
  
  CapeMutex mutex;
  
  number_t cnt;
  number_t len;
  
}; typedef struct WebsStream_s* WebsStream;

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__json (void* user_ptr, QWebsRequest request, CapeErr err)
{
  WebsJson webs_json = webs_json_new (user_ptr, request);
  
  return webs_json_run (&webs_json, err);  
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__rest (void* user_ptr, QWebsRequest request, CapeErr err)
{
  CapeList url_parts = qwebs_request_clist (request);
  if (cape_list_size (url_parts) >= 1)
  {
    const CapeString module;
    const CapeString method;
    
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
    
    WebsJson webs_json = webs_json_new (user_ptr, request);

    webs_json_set (webs_json, module, method, &clist, &token);

    // check for authentication
    return webs_json_run_gen (&webs_json, err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__imca (void* user_ptr, QWebsRequest request, CapeErr err)
{
  WebsStream self = user_ptr;
  
  QWebsRequest h = request;  
  
  cape_mutex_lock (self->mutex);
  
  if (self->image)
  {
    qwebs_request_send_buf (&h, self->image, err);
  }
  else
  {
    qwebs_request_send_json (&h, NULL, err);    
  }

  cape_mutex_unlock (self->mutex);
  
  return CAPE_ERR_CONTINUE;
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
  int res;
  WebsStream self = ptr;
  
  // local objects
  CapeString token = NULL;


  token = cape_str_uuid ();
  
  printf ("register stream\n");


  res = CAPE_ERR_NONE;
  qout->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_s_mv (qout->pdata, "token", &token);
  
exit_and_cleanup:
  
  cape_str_del (&token);
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__stream_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  WebsStream self = ptr;
  
  // local objects
  CapeString image = NULL;
  const CapeString token;
  
  if (NULL == qin->pdata)
  {
    // post some weired message
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "no security role found");
    goto exit_and_cleanup;
  }
  
  token = cape_udc_get_s (qin->pdata, "token", NULL);
  if (NULL == token)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "no token");
    goto exit_and_cleanup;
  }
  
  if (NULL == qin->cdata)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is NULL");
    goto exit_and_cleanup;
  }
  
  image = cape_udc_ext_s (qin->cdata, "image");
  if (NULL == image)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "image is NULL");
    goto exit_and_cleanup;
  }
  
  cape_mutex_lock (self->mutex);
  
  self->cnt++;
  self->len = self->len + cape_str_size (image);
  
  cape_str_replace_mv (&(self->image), &image);

  cape_mutex_unlock (self->mutex);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&image);
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__stream__on_timer (void* ptr)
{
  WebsStream self = ptr;
  
  cape_mutex_lock (self->mutex);

  cape_log_fmt (CAPE_LL_DEBUG, "WSRV", "stream stats", "stream: %li images/s, %f kb/s", self->cnt, (double)self->len / 1024);

  self->cnt = 0;
  self->len = 0;
  
  cape_mutex_unlock (self->mutex);
  
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

static int __STDCALL qbus_webs_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;

  // local objects
  CapeUdc sites = cape_udc_cp (qbus_config_node (qbus, "sites"));
  
  const CapeString site = qbus_config_s (qbus, "site", "public");
  if (cape_str_not_empty (site))
  {
    cape_udc_add_s_cp (sites, "/", site);
  }
  
  const CapeString host = qbus_config_s (qbus, "host", "127.0.0.1");

  // this is the directory to find error pages
  const CapeString pages = qbus_config_s (qbus, "pages", "pages");
  
  number_t port = qbus_config_n (qbus, "port", 8082);
  number_t threads = qbus_config_n (qbus, "threads", 4);
  
  CapeUdc route_list = qbus_config_node (qbus, "route_list");
  
  QWebs webs = qwebs_new (sites, host, port, threads, pages, route_list);
  
  CapeAioTimer timer = cape_aio_timer_new ();

  WebsStream s = CAPE_NEW (struct WebsStream_s);

  s->image = NULL;
  s->mutex = cape_mutex_new ();
  s->cnt = 0;
  s->len = 0;
  
  res = qwebs_reg (webs, "json", qbus, qbus_webs__json, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qwebs_reg (webs, "rest", qbus, qbus_webs__rest, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qwebs_reg (webs, "imca", s, qbus_webs__imca, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qwebs_reg (webs, "post", qbus, qbus_webs__post, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qwebs_attach (webs, qbus_aio (qbus), err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  /*
  res = cape_aio_timer_set (timer, 1000, s, qbus_webs__stream__on_timer, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_aio_timer_add (&timer, qbus_aio (qbus));
  if (res)
  {
    goto exit_and_cleanup;
  }
  */

  // --------- RESTAPI callbacks -----------------------------
  
  qbus_register (qbus, "GET", webs, qbus_webs__restapi_get, NULL, err);
  
  // --------- register callbacks -----------------------------
  
  qbus_register (qbus, "modules_get", webs, qbus_webs__modules_get, NULL, err);
  
  qbus_register (qbus, "stream_add", s, qbus_webs__stream_add, NULL, err);
  qbus_register (qbus, "stream_set", s, qbus_webs__stream_set, NULL, err);

  // --------- register callbacks -----------------------------
  
  *p_ptr = webs;
  
  return CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return cape_err_code (err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_webs_done (QBus qbus, void* ptr, CapeErr err)
{
  QWebs webs = ptr;
  
  qwebs_del (&webs);
  
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("WEBS", NULL, qbus_webs_init, qbus_webs_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
