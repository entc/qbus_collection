#include "qwebs.h"
#include "qwebs_connection.h"
#include "qwebs_files.h"

// cape includes
#include <aio/cape_aio_sock.h>
#include <sys/cape_socket.h>
#include <stc/cape_map.h>
#include <sys/cape_queue.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

struct QWebsApi_s
{
  fct_qwebs__on_request on_request;
  
  void* user_ptr;
  
};

//-----------------------------------------------------------------------------

QWebsApi qwebs_api_new (void* user_ptr, fct_qwebs__on_request on_request)
{
  QWebsApi self = CAPE_NEW (struct QWebsApi_s);

  self->on_request = on_request;
  self->user_ptr = user_ptr;
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_api_del (QWebsApi* p_self)
{
  if (*p_self)
  {
    // QWebsApi self = *p_self;
    
    
    
    CAPE_DEL (p_self, struct QWebsApi_s);
  }
}

//-----------------------------------------------------------------------------

struct QWebs_s
{
  CapeMap request_apis;
  
  CapeString host;
  
  number_t port;
  
  CapeString pages;
  
  CapeAioContext aio_attached;
  
  CapeAioAccept accept;
  
  CapeQueue queue;
  
  QWebsFiles files;
  
  CapeUdc route_list;
};

//-----------------------------------------------------------------------------

static void __STDCALL qwebs__intern__on_api_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QWebsApi api = val; qwebs_api_del (&api);
  }
}

//-----------------------------------------------------------------------------

QWebs qwebs_new (const CapeString site, const CapeString host, number_t port, number_t threads, const CapeString pages, CapeUdc route_list)
{
  QWebs self = CAPE_NEW (struct QWebs_s);
  
  self->host = cape_str_cp (host);
  self->port = port;
  
  self->pages = cape_str_cp (pages);
  
  self->request_apis = cape_map_new (NULL, qwebs__intern__on_api_del, NULL);
  
  self->aio_attached = NULL;
  self->accept = NULL;
  
  self->queue = cape_queue_new ();
  
  self->files = qwebs_files_new (site, self);
  
  self->route_list = cape_udc_cp (route_list);
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_del (QWebs* p_self)
{
  if (*p_self)
  {
    QWebs self = *p_self;
    
    cape_map_del (&(self->request_apis));
    
    cape_str_del (&(self->host));    
    cape_str_del (&(self->pages));
    
    //cape_aio_accept_del (&(self->accept));
    
    cape_queue_del (&(self->queue));
    
    qwebs_files_del (&(self->files));
    
    cape_udc_del (&(self->route_list));
    
    CAPE_DEL (p_self, struct QWebs_s);
  }
}

//-----------------------------------------------------------------------------

int qwebs_reg (QWebs self, const CapeString name, void* user_ptr, fct_qwebs__on_request on_request, CapeErr err)
{
  CapeMapNode n = cape_map_find (self->request_apis, name);
  
  if (NULL == n)
  {
    QWebsApi api = qwebs_api_new (user_ptr, on_request);

    // transfer ownership to the map
    cape_map_insert (self->request_apis, cape_str_cp (name), api);
    
    return CAPE_ERR_NONE;
  }
  else
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "API was already registered");
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs__intern__on_connect (void* user_ptr, void* handle, const char* remote_host)
{
  QWebs self = user_ptr;
  
  // for each connection we create a connection object
  QWebsConnection connection = qwebs_connection_new (handle, self->queue, self);
  
  cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on connect", "new connection from %s", remote_host);
  
  // attach the connection to the AIO subsytem
  qwebs_connection_attach (connection, self->aio_attached);
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs__intern__on_done (void* user_ptr)
{
  QWebs self = user_ptr;
  
  //cape_aio_accept_del (&(self->accept));
}

//-----------------------------------------------------------------------------

const CapeString qwebs_pages (QWebs self)
{
  if (self)
  {
    return self->pages;
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

int qwebs_route (QWebs self, const CapeString name)
{
  CapeUdc h = cape_udc_get (self->route_list, name);
  
  return h ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------

int qwebs_attach (QWebs self, CapeAioContext aio_context, CapeErr err)
{
  int res;
  
  res = cape_queue_start (self->queue, 1, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  void* handle = cape_sock__tcp__srv_new (self->host, self->port, err);
  if (NULL == handle)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->aio_attached = aio_context;
  
  // create a new acceptor
  self->accept = cape_aio_accept_new (handle);
  
  // set the callbacks
  cape_aio_accept_callback (self->accept, self, qwebs__intern__on_connect, qwebs__intern__on_done);
  
  // add the acceptor to the AIO context
  cape_aio_accept_add (&(self->accept), aio_context);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

QWebsApi qwebs_get_api (QWebs self, const CapeString name)
{
  CapeMapNode n = cape_map_find (self->request_apis, name);
  
  if (n)
  {
    return cape_map_node_value (n);
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

QWebsFiles qwebs_files (QWebs self)
{
  return self->files;
}

//-----------------------------------------------------------------------------

int qwebs_api_call (QWebsApi self, QWebsRequest request, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->on_request)
  {
    res = self->on_request (self->user_ptr, request, err);
  }
  
  return res;
}

//-----------------------------------------------------------------------------
