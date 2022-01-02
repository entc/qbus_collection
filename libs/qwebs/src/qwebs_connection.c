#include "qwebs_connection.h"
#include "qwebs.h"
#include "qwebs_files.h"
#include "http_parser.h"
#include "qwebs_response.h"

// cape includes
#include <aio/cape_aio_sock.h>
#include <sys/cape_socket.h>
#include <stc/cape_map.h>
#include <stc/cape_list.h>
#include <sys/cape_log.h>
#include <fmt/cape_tokenizer.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

void qwebs_connection_send (QWebsConnection, CapeStream*);

void qwebs_connection_inc (QWebsConnection);

void qwebs_connection_dec (QWebsConnection);

//-----------------------------------------------------------------------------

struct QWebsRequest_s
{
  QWebs webs;                      // reference
  
  QWebsApi api;                    // reference
  
  QWebsConnection conn;            // reference
  
  CapeString method;
  CapeString url;
  CapeString mime;
  
  CapeMap header_values;
  CapeMap option_values;
  
  CapeList url_values;
  
  CapeStream body_value;
  
  // temporary members
  
  CapeString last_header_field;

  int is_complete;
  int is_processed;
  
  const CapeString site;
};

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_request__intern__on_headers_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

QWebsRequest qwebs_request_new (QWebs webs, QWebsConnection conn)
{
  QWebsRequest self = CAPE_NEW (struct QWebsRequest_s);

  self->webs = webs;
  self->conn = conn;
  
  self->api = NULL;
  
  self->method = NULL;
  self->url = NULL;
  
  // this will be set in API mode
  self->header_values = NULL;
  self->option_values = NULL;
  
  self->url_values = NULL;
  self->body_value = NULL;
  
  self->last_header_field = NULL;
  
  self->is_complete = FALSE;
  self->is_processed = FALSE;
  
  qwebs_connection_inc (self->conn);
  
  self->site = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_request_del (QWebsRequest* p_self)
{
  if (*p_self)
  {
    QWebsRequest self = *p_self;
    
    qwebs_connection_dec (self->conn);

    cape_str_del (&(self->method));
    cape_str_del (&(self->url));

    cape_map_del (&(self->header_values));
    cape_list_del (&(self->url_values));
    cape_stream_del (&(self->body_value));
    
    cape_str_del (&(self->last_header_field));
    
    CAPE_DEL (p_self, struct QWebsRequest_s);
  }
}

//-----------------------------------------------------------------------------

CapeMap qwebs_request__internal__convert_query (const CapeString query)
{
  CapeMap ret = cape_map_new (NULL, qwebs_request__intern__on_headers_del, NULL);

  CapeList values = cape_tokenizer_buf__noempty (query, cape_str_size (query), '&');
  
  CapeListCursor* cursor = cape_list_cursor_create (values, CAPE_DIRECTION_FORW);
  while (cape_list_cursor_next (cursor))
  {
    const CapeString value = cape_list_node_data (cursor->node);
    
    CapeString key = NULL;
    CapeString val = NULL;
    
    if (cape_tokenizer_split (value, '=', &key, &val))
    {
      cape_map_insert (ret, key, val);
    }
    else
    {
      cape_str_del (&key);
      cape_str_del (&val);
    }
  }
  
  cape_list_del (&values);
  return ret;
}

//-----------------------------------------------------------------------------

static int qwebs_request__internal__on_url (http_parser* parser, const char *at, size_t length)
{
  QWebsRequest self = parser->data;
  
  // this will re-write the url
  // -> checks all sites for re-writing
  self->site = qwebs_site (self->webs, at, length, &(self->url));
  
  cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on url", "access: %s", self->url);
  
  // create a map for all header values
  self->header_values = cape_map_new (NULL, qwebs_request__intern__on_headers_del, NULL);

  if ('/' == *(self->url))
  {
    CapeString url = NULL;
    CapeString query = NULL;
    
    if (cape_tokenizer_split (self->url, '?', &url, &query))
    {
      cape_str_replace_mv (&(self->url), &url);
      
      // parse the options into a map
      self->option_values = qwebs_request__internal__convert_query (query);
      
      cape_str_del (&query);
    }

    // split the url into its parts
    self->url_values = cape_tokenizer_buf__noempty (self->url + 1, cape_str_size (self->url) - 1, '/');

    if (cape_list_size (self->url_values) >= 1)
    {
      CapeListNode n = cape_list_node_front (self->url_values);
      
      // get the first part
      const CapeString first_part = cape_list_node_data (n);
      
      if (qwebs_route (self->webs, first_part))
      {
        cape_str_replace_cp (&(self->url), "/index.html");
      }
      else if (cape_list_size (self->url_values) >= 2)
      {
        // anaylse the URL if we have an API or not
        self->api = qwebs_get_api (self->webs, first_part);
        
        if (self->api)
        {
          // reduce the url values by one
          {
            CapeListNode n = cape_list_node_front (self->url_values);
            
            cape_list_node_erase (self->url_values, n);
          }
          
          self->body_value = cape_stream_new ();
        }
      }
      else
      {
        self->api = qwebs_get_page (self->webs, self->url + 1);
      }
    }    
    else
    {
      if (cape_str_equal ("/", self->url))
      {
        cape_str_replace_cp (&(self->url), "/index.html");
      }

      self->api = qwebs_get_page (self->webs, self->url);
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------

static int qwebs_request__internal__on_header_field (http_parser* parser, const char *at, size_t length)
{
  QWebsRequest self = parser->data;

  if (self->header_values)
  {
    CapeString h = cape_str_sub (at, length);
    
    cape_str_replace_mv (&(self->last_header_field), &h);
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

static int qwebs_request__internal__on_header_value (http_parser* parser, const char *at, size_t length)
{
  QWebsRequest self = parser->data;
  
  if (self->header_values)
  {
    if (self->last_header_field)
    {
      CapeString h = cape_str_sub (at, length);
      //printf ("HEADER VALUE: %s = %s\n", self->last_header_field, h);
      
      // transfer ownership to the map
      cape_map_insert (self->header_values, self->last_header_field, h);
      self->last_header_field = NULL;
    }
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

static int qwebs_request__internal__on_body (http_parser* parser, const char* at, size_t length)
{
  QWebsRequest self = parser->data;
  
  //printf ("---------------------------------------------------------------------------\n");
  //printf ("%.*s\n", (int)length, at);
  //printf ("---------------------------------------------------------------------------\n");

  if (self->api)
  {
    cape_stream_append_buf (self->body_value, at, length);
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

static int qwebs_request__internal__on_message_begin (http_parser* parser)
{
  QWebsRequest self = parser->data;

  self->is_complete = FALSE;

  return 0;
}

//-----------------------------------------------------------------------------

static int qwebs_request__internal__on_message_complete (http_parser* parser)
{
  QWebsRequest self = parser->data;

  self->is_complete = TRUE;
  
  return 0;
}

//-----------------------------------------------------------------------------

void qwebs_request_send_json (QWebsRequest* p_self, CapeUdc content, number_t ttl, CapeErr err)
{
  if (*p_self)
  {
    QWebsRequest self = *p_self;
    
    // local objects
    CapeStream s = cape_stream_new ();
    
    if (cape_err_code (err))
    {
      // create the HTTP error response
      qwebs_response_err (s, self->webs, content, "application/json", err);
    }
    else
    {
      // create the JSON response
      qwebs_response_json (s, self->webs, content, ttl);
    }
    
    qwebs_connection_send (self->conn, &s);
    qwebs_request_del (p_self);
  }
}

//-----------------------------------------------------------------------------

void qwebs_request_send_file (QWebsRequest* p_self, CapeUdc file_node, CapeErr err)
{
  if (*p_self)
  {
    QWebsRequest self = *p_self;
    
    // local objects
    CapeStream s = cape_stream_new ();
    
    qwebs_response_file (s, self->webs, file_node);
    
    qwebs_connection_send (self->conn, &s);
    qwebs_request_del (p_self);
  }
}

//-----------------------------------------------------------------------------

void qwebs_request_send_image (QWebsRequest* p_self, const CapeString image_as_base64, CapeErr err)
{
  if (*p_self)
  {
    QWebsRequest self = *p_self;
    
    // local objects
    CapeStream s = cape_stream_new ();
    
    qwebs_response_image (s, self->webs, image_as_base64);
    
    qwebs_connection_send (self->conn, &s);
    qwebs_request_del (p_self);
  }
}

//-----------------------------------------------------------------------------

void qwebs_request_send_buf (QWebsRequest* p_self, const CapeString buf, const CapeString mime_type, number_t ttl, CapeErr err)
{
  if (*p_self)
  {
    QWebsRequest self = *p_self;
    
    // local objects
    CapeStream s = cape_stream_new ();
    
    qwebs_response_buf (s, self->webs, buf, mime_type, ttl);
    
    qwebs_connection_send (self->conn, &s);
    qwebs_request_del (p_self);
  }
}

//-----------------------------------------------------------------------------

void qwebs_request_redirect (QWebsRequest* p_self, const CapeString url)
{
  if (*p_self)
  {
    QWebsRequest self = *p_self;
    
    // local objects
    CapeStream s = cape_stream_new ();
    
    qwebs_response_redirect (s, self->webs, url);
    
    qwebs_connection_send (self->conn, &s);
    qwebs_request_del (p_self);
  }
}

//-----------------------------------------------------------------------------

void qwebs_request_switching_protocols (QWebsRequest* p_self, QWebsUpgrade upgrade, const CapeString upgrade_name)
{
  int res;
  QWebsRequest self = *p_self;
  
  // local objects
  CapeErr err = cape_err_new ();
  CapeMap return_headers = cape_map_new (NULL, qwebs_request__intern__on_headers_del, NULL);
  CapeStream s = cape_stream_new ();

  res = qwebs_upgrade_call (upgrade, self, return_headers, err);
  if (res)
  {
    
  }
  
  qwebs_response_sp (s, self->webs, upgrade_name, return_headers);

  qwebs_connection_send (self->conn, &s);
  
  // keep connection
  qwebs_connection_inc (self->conn);
  
  qwebs_upgrade_conn (upgrade, self->conn);
  
exit_and_cleanup:

  cape_stream_del (&s);
  cape_map_del (&return_headers);
  cape_err_del (&err);
  
  qwebs_request_del (p_self);
}

//-----------------------------------------------------------------------------

void qwebs_request_stream_init (QWebsRequest self, const CapeString boundary, const CapeString mime_type, CapeErr err)
{
  // local objects
  CapeStream s = cape_stream_new ();

  qwebs_response_mp_init (s, self->webs, boundary, mime_type);
  
  qwebs_connection_send (self->conn, &s);
}

//-----------------------------------------------------------------------------

void qwebs_request_stream_buf (QWebsRequest self, const CapeString boundary, const char* bufdat, number_t buflen, const CapeString mime_type, CapeErr err)
{
  // local objects
  CapeStream s = cape_stream_new ();
  
  qwebs_response_mp_part (s, self->webs, boundary, mime_type, bufdat, buflen);
  
  qwebs_connection_send (self->conn, &s);
}

//-----------------------------------------------------------------------------

void qwebs_request_api (QWebsRequest* p_self)
{
  int res;
  QWebsRequest self = *p_self;

  // local objects
  CapeErr err = cape_err_new ();
  
  res = qwebs_api_call (self->api, self, err);
  if (res)
  {
    *p_self = NULL;
    goto exit_and_cleanup;
  }
  
  qwebs_request_send_json (p_self, NULL, 0, err);

exit_and_cleanup:
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

CapeList qwebs_request_clist (QWebsRequest self)
{
  return self->url_values;
}

//-----------------------------------------------------------------------------

CapeMap qwebs_request_headers (QWebsRequest self)
{
  return self->header_values;
}

//-----------------------------------------------------------------------------

CapeMap qwebs_request_query (QWebsRequest self)
{
  return self->option_values;
}

//-----------------------------------------------------------------------------

CapeStream qwebs_request_body (QWebsRequest self)
{
  return self->body_value;
}

//-----------------------------------------------------------------------------

const CapeString qwebs_request_method (QWebsRequest self)
{
  return self->method;
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_request__internal__on_run (void* ptr, number_t pos)
{
  QWebsRequest self = ptr;
 
  if (self->api)
  {
    qwebs_request_api (&self);
  }
  else
  {
    CapeStream s = qwebs_files_get (qwebs_files (self->webs), self, self->site, self->url);

    if (s)
    {
      qwebs_connection_send (self->conn, &s);
    }
    
    qwebs_request_del (&self);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_request__internal__on_del (void* ptr, number_t pos)
{
}

//-----------------------------------------------------------------------------

struct QWebsConnection_s
{
  QWebs webs;                      // reference
  CapeQueue queue;                 // reference

  CapeAioSocket aio_socket;
  CapeAioContext aio_attached;
  
  CapeList send_cache;
  
  http_parser parser;
  http_parser_settings settings;
  
  CapeMutex mutex;
  
  int close_connection;           // closes the connection for each request
  int active;
  
  CapeString remote;
  
  fct_qwebs__on_recv on_recv;
};

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_connection__cache__on_del (void* ptr)
{
  CapeStream s = ptr; cape_stream_del (&s);
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_connection__http__on_recv (QWebsConnection, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

QWebsConnection qwebs_connection_new (void* handle, CapeQueue queue, QWebs webs, const CapeString remote)
{
  QWebsConnection self = CAPE_NEW (struct QWebsConnection_s);
  
  self->webs = webs;
  self->queue = queue;
  
  self->aio_socket = cape_aio_socket_new (handle);
  
  http_parser_init (&(self->parser), HTTP_REQUEST);
  
  // initialize the HTTP parser
  http_parser_settings_init (&(self->settings));

  // set some callbacks
  self->settings.on_message_begin = qwebs_request__internal__on_message_begin;
  self->settings.on_url = qwebs_request__internal__on_url;
  self->settings.on_status = NULL;
  self->settings.on_header_field = qwebs_request__internal__on_header_field;
  self->settings.on_header_value = qwebs_request__internal__on_header_value;
  self->settings.on_headers_complete = NULL;
  self->settings.on_body = qwebs_request__internal__on_body;
  self->settings.on_message_complete = qwebs_request__internal__on_message_complete;
  self->settings.on_chunk_header = NULL;
  self->settings.on_chunk_complete = NULL;
  
  self->send_cache = cape_list_new (qwebs_connection__cache__on_del);
  self->mutex = cape_mutex_new ();
  
  self->close_connection = FALSE;
  self->active = FALSE;
  
  self->remote = cape_str_cp (remote);
  
  self->on_recv = qwebs_connection__http__on_recv;
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_connection_del (QWebsConnection* p_self)
{
  if (*p_self)
  {
    QWebsConnection self = *p_self;
    
    cape_list_del (&(self->send_cache));
    cape_mutex_del (&(self->mutex));
    
    cape_str_del (&(self->remote));
    
    CAPE_DEL (p_self, struct QWebsConnection_s);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_connection__internal__on_send_ready (void* ptr, CapeAioSocket socket, void* userdata)
{
  QWebsConnection self = ptr;

  CapeStream s;
  
  // check for userdata
  if (userdata)
  {
    // userdata is always a stream
    s = userdata;
    
    // cleanup stream
    cape_stream_del (&s);
  }
  
  cape_mutex_lock (self->mutex);

  s = cape_list_pop_front (self->send_cache);

  cape_mutex_unlock (self->mutex);
  
  if (s)
  {
    // if we do have a stream send it to the socket
    cape_aio_socket_send (self->aio_socket, self->aio_attached, cape_stream_get (s), cape_stream_size (s), s);
  }
  else if (self->active == FALSE)
  {
    // some proxies or browser can't handle connections to stay alive
    if (self->close_connection)
    {
      cape_log_fmt (CAPE_LL_TRACE, "WEBS", "on recv", "close connection");

      // close it
      cape_aio_socket_close (self->aio_socket, self->aio_attached);
    }
  }
}

//-----------------------------------------------------------------------------

void qwebs_request_complete (QWebsRequest* p_self, const CapeString method)
{
  QWebsRequest self = *p_self;
  
  self->method = cape_str_cp (method);
  
  //printf ("METHOD %s (COMPLETE %i)\n", request->method, request->is_complete);
  
  if (self->is_complete)
  {
    {
      CapeMapNode n = cape_map_find (self->header_values, "Upgrade");
      if (n)
      {
        const CapeString name = cape_map_node_value (n);
        
        QWebsUpgrade upgrade = qwebs_get_upgrade (self->webs, name);

        cape_log_fmt (CAPE_LL_DEBUG, "QWEBS", "request complete", "found 'upgrade' in headers with value = %s", name);
        
        if (upgrade)
        {
          qwebs_request_switching_protocols (p_self, upgrade, name);
          
          return;
        }
        else
        {
          // send error back and close connection

          qwebs_request_del (p_self);
          return;
        }
      }
    }
    
    {
      CapeMapNode n = cape_map_find (self->header_values, "Connection");
      if (n)
      {
        const CapeString connection_type = cape_map_node_value (n);
        
        //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "on recv", "connection type: %s", connection_type);
        
        self->conn->close_connection = cape_str_equal (connection_type, "close");
      }
      else
      {
        cape_log_fmt (CAPE_LL_WARN, "WEBS", "on recv", "connection type unknown");
      }
    }
    
    if (self->api)
    {
      qwebs_request_api (p_self);
    }
    else
    {
      CapeStream s = qwebs_files_get (qwebs_files (self->webs), self, self->site, self->url);
      if (s)
      {
        qwebs_connection_send (self->conn, &s);
      }
      
      qwebs_request_del (p_self);
    }
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_connection__http__on_recv (QWebsConnection self, const char* bufdat, number_t buflen)
{
  if (NULL == self->parser.data)
  {
    self->parser.data = qwebs_request_new (self->webs, self);
    self->active = TRUE;
  }
  
  //int bytes_processed = http_parser_execute (&(self->parser), &(self->settings), bufdat, buflen);
  http_parser_execute (&(self->parser), &(self->settings), bufdat, buflen);
  
  //printf ("BYTES PROCESSED: %i\n", bytes_processed);
  
  if (self->parser.http_errno > 0)
  {
    CapeString h = cape_str_catenate_3 (http_errno_name (self->parser.http_errno), " : ", http_errno_description ((enum http_errno)self->parser.http_errno));
    
    cape_log_fmt (CAPE_LL_ERROR, "QWEBS", "on recv", "parser returned an error [%i]: %s", self->parser.http_errno, h);
    
    cape_str_del (&h);
    
    // close it
    cape_aio_socket_close (self->aio_socket, self->aio_attached);
    
    return;
  }
  
  if (http_body_is_final (&(self->parser)))
  {
    return;
  }
  
  qwebs_request_complete ((QWebsRequest*)&(self->parser.data), http_method_str (self->parser.method));
  
  return;
  
  {
    QWebsRequest request = self->parser.data;
    
    request->method = cape_str_cp (http_method_str (self->parser.method));
    
    //printf ("METHOD %s (COMPLETE %i)\n", request->method, request->is_complete);
    
    if (request->is_complete)
    {
      
      {
        CapeMapNode n = cape_map_find (request->header_values, "Connection");
        if (n)
        {
          const CapeString connection_type = cape_map_node_value (n);
          
          //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "on recv", "connection type: %s", connection_type);
          
          self->close_connection = cape_str_equal (connection_type, "close");
        }
        else
        {
          cape_log_fmt (CAPE_LL_WARN, "WEBS", "on recv", "connection type unknown");
        }
      }
      
      if (request->api)
      {
        qwebs_request_api (&request);
      }
      else
      {
        //printf ("FILE '%s'\'%s'\n", request->site, request->url);
        
        CapeStream s = qwebs_files_get (qwebs_files (self->webs), request, request->site, request->url);
        if (s)
        {
          qwebs_connection_send (self, &s);
        }
        
        qwebs_request_del (&request);
      }
      
      // this is faster but it block the process and results in a zombie
      //cape_queue_add (self->queue, NULL, qwebs_request__internal__on_run, qwebs_request__internal__on_del, self->parser.data, 0);
      
      self->parser.data = NULL;
    }
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_connection__internal__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  QWebsConnection self = ptr;
  
  self->on_recv (self, bufdat, buflen);
}

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_connection__internal__on_done (void* ptr, void* userdata)
{
  QWebsConnection self = ptr;
  
  // check for userdata
  if (userdata)
  {
    // userdata is always a stream
    CapeStream s = userdata;
    
    // cleanup stream
    cape_stream_del (&s);
  }

  qwebs_connection_del (&self);
}

//-----------------------------------------------------------------------------

void qwebs_connection_attach (QWebsConnection self, CapeAioContext aio_context)
{
  // set the callbacks
  cape_aio_socket_callback (self->aio_socket, self, qwebs_connection__internal__on_send_ready, qwebs_connection__internal__on_recv, qwebs_connection__internal__on_done);
  
  {
    CapeAioSocket h = self->aio_socket;
    
    // attach it to the AIO subsytem
    cape_aio_socket_add_r (&h, aio_context);
  }
  
  self->aio_attached = aio_context;
}

//-----------------------------------------------------------------------------

void qwebs_connection_send (QWebsConnection self, CapeStream* p_stream)
{
  cape_mutex_lock (self->mutex);
  
  // TODO: memory leak
  cape_list_push_back (self->send_cache, *p_stream);
  *p_stream = NULL;

  cape_mutex_unlock (self->mutex);

  cape_aio_socket_markSent (self->aio_socket, self->aio_attached);
}

//-----------------------------------------------------------------------------

void qwebs_connection_inc (QWebsConnection self)
{
  cape_aio_socket_inref (self->aio_socket);
}

//-----------------------------------------------------------------------------

void qwebs_connection_dec (QWebsConnection self)
{
  cape_aio_socket_unref (self->aio_socket);
  self->active = FALSE;
}

//-----------------------------------------------------------------------------

void qwebs_connection_upgrade (QWebsConnection self, fct_qwebs__on_recv on_recv)
{
  self->on_recv = on_recv;
}

//-----------------------------------------------------------------------------

CapeString qwebs_request_remote (QWebsRequest self)
{
  CapeString remote = NULL;
  
  // if we have no real ip address
  if (cape_str_equal (self->conn->remote, "0.0.0.0"))
  {
    // check for proxy forward
    CapeMapNode n = cape_map_find (self->header_values, "X-Forwarded-For");
    if (n)
    {
      remote = cape_str_cp ((CapeString)cape_map_node_value (n));
    }
  }
  
  if (remote == NULL)
  {
    remote = cape_str_cp (self->conn->remote);
  }

  return remote;
}

//-----------------------------------------------------------------------------
