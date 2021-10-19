#include "qwebs.h"
#include "qwebs_connection.h"
#include "qwebs_files.h"
#include "qwebs_multipart.h"

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
//    QWebsApi self = *p_self;
    
    
    CAPE_DEL (p_self, struct QWebsApi_s);
  }
}

//-----------------------------------------------------------------------------

struct QWebs_s
{
  CapeMap request_apis;      // all api callbacks
  CapeMap request_page;      // all page callbacks

  CapeMap sites;
  
  CapeString host;
  
  number_t port;
  
  CapeString pages;
  
  CapeAioContext aio_attached;
  
  CapeAioAccept accept;
  
  CapeQueue queue;
  
  QWebsFiles files;
  
  CapeUdc route_list;
  
  QWebsEncoder encoder;

  CapeString identifier;
  CapeString provider;
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

static void __STDCALL qwebs__intern__on_sites_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

void qwebs__internal__convert_sites (QWebs self, CapeUdc sites)
{
  CapeUdcCursor* cursor = cape_udc_cursor_new (sites, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString site = cape_udc_s (cursor->item, NULL);
    const CapeString name = cape_udc_name (cursor->item);
    
    if (site && name)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "init", "set site '%s' = %s", name, site);
      
      cape_map_insert (self->sites, cape_str_cp (name), cape_str_cp (site));
    }
  }
  
  cape_udc_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

QWebs qwebs_new (CapeUdc sites, const CapeString host, number_t port, number_t threads, const CapeString pages, CapeUdc route_list, const CapeString identifier, const CapeString provider)
{
  QWebs self = CAPE_NEW (struct QWebs_s);
  
  self->host = cape_str_cp (host);
  self->port = port;
  
  self->pages = cape_str_cp (pages);
  
  self->request_apis = cape_map_new (NULL, qwebs__intern__on_api_del, NULL);
  self->request_page = cape_map_new (NULL, qwebs__intern__on_api_del, NULL);
  
  self->aio_attached = NULL;
  self->accept = NULL;
  
  self->queue = cape_queue_new ();
  
  self->files = qwebs_files_new (self);
  
  self->route_list = cape_udc_cp (route_list);
  
  self->sites = cape_map_new (NULL, qwebs__intern__on_sites_del, NULL);

  if (sites)
  {
    // convert site into a map
    // -> map can be altered later to fit more needs
    qwebs__internal__convert_sites (self, sites);
  }
  
  self->encoder = qwebs_encode_new ();

  self->identifier = cape_str_cp (identifier);
  self->provider = cape_str_cp (provider);

  return self;
}

//-----------------------------------------------------------------------------

void qwebs_del (QWebs* p_self)
{
  if (*p_self)
  {
    QWebs self = *p_self;
    
    cape_map_del (&(self->request_apis));
    cape_map_del (&(self->request_page));
    cape_map_del (&(self->sites));

    cape_str_del (&(self->host));    
    cape_str_del (&(self->pages));
    
    //cape_aio_accept_del (&(self->accept));
    
    cape_queue_del (&(self->queue));
    
    qwebs_files_del (&(self->files));
    
    cape_udc_del (&(self->route_list));
    
    qwebs_encode_del (&(self->encoder));
    
    cape_str_del (&(self->identifier));
    cape_str_del (&(self->provider));

    CAPE_DEL (p_self, struct QWebs_s);
  }
}

//-----------------------------------------------------------------------------

int qwebs_reg (QWebs self, const CapeString name, void* user_ptr, fct_qwebs__on_request on_request, CapeErr err)
{
  if (name)
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

  return cape_err_set (err, CAPE_ERR_RUNTIME, "API can't be registered");
}

//-----------------------------------------------------------------------------

int qwebs_reg_page (QWebs self, const CapeString page, void* user_ptr, fct_qwebs__on_request on_request, CapeErr err)
{
  if (page)
  {
    CapeMapNode n = cape_map_find (self->request_page, page);
    if (NULL == n)
    {
      QWebsApi api = qwebs_api_new (user_ptr, on_request);
      
      // transfer ownership to the map
      cape_map_insert (self->request_page, cape_str_cp (page), api);
      
      return CAPE_ERR_NONE;
    }
    else
    {
      return cape_err_set (err, CAPE_ERR_RUNTIME, "API was already registered");
    }
  }
  
  return cape_err_set (err, CAPE_ERR_RUNTIME, "API can't be registered");
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs__intern__on_connect (void* user_ptr, void* handle, const char* remote_host)
{
  QWebs self = user_ptr;
  
  // for each connection we create a connection object
  QWebsConnection connection = qwebs_connection_new (handle, self->queue, self, remote_host);
  
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
  if (name)
  {
    CapeMapNode n = cape_map_find (self->request_apis, name);
    if (n)
    {
      return cape_map_node_value (n);
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

QWebsApi qwebs_get_page (QWebs self, const CapeString page)
{
  if (page)
  {
    CapeMapNode n = cape_map_find (self->request_page, page);
    if (n)
    {
      return cape_map_node_value (n);
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

QWebsFiles qwebs_files (QWebs self)
{
  return self->files;
}

//-----------------------------------------------------------------------------

CapeString qwebs_url_encode (QWebs self, const CapeString url)
{
  return qwebs_encode_run (self->encoder, url);
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

const CapeString qwebs__intern__get_site (QWebs self, const CapeString part)
{
  CapeMapNode n = cape_map_find (self->sites, part);
  if (n)
  {
    return cape_map_node_value (n);
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

const CapeString qwebs_site (QWebs self, const char *bufdat, size_t buflen, CapeString* p_url)
{
  const CapeString ret;
  
  // local objects
  CapeString url = cape_str_sub (bufdat, buflen);
  
  if ('/' == *url)
  {
    number_t pos;
    if (cape_str_next (url + 1, '/', &pos))
    {
      CapeString h = cape_str_sub (url, pos + 1);
      
      ret = qwebs__intern__get_site (self, h);
      
      cape_str_del (&h);
      
      if (ret)
      {
        *p_url = cape_str_sub (url + pos + 1, cape_str_size (url) - pos - 1);
        goto exit_and_cleanup;
      }
    }
    else
    {
      ret = qwebs__intern__get_site (self, url);
      if (ret)
      {
        // this means the whole url is a site
        // -> re-write to /
        *p_url = cape_str_cp ("/");
        goto exit_and_cleanup;
      }
    }
  }
  
  ret = qwebs__intern__get_site (self, "/");
  *p_url = cape_str_mv (&url);
  
exit_and_cleanup:
  
  cape_str_del (&url);
  return ret;
}

//-----------------------------------------------------------------------------

const CapeString qwebs_identifier (QWebs self)
{
  return self->identifier;
}

//-----------------------------------------------------------------------------

const CapeString qwebs_provider (QWebs self)
{
  return self->provider;
}

//-----------------------------------------------------------------------------
