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

struct WebsAuth_s
{
  QBus qbus;
  QWebsRequest request;
  
  CapeString module;
  CapeString method;

  // the data for QBUS
  CapeUdc clist;
  CapeUdc cdata;
  CapeUdc files;
  
  CapeList files_to_delete;
  
  // extra features
  CapeString extra_token;

  // shortcuts from the header values
  const CapeString mime;
  const CapeString auth;
  
}; typedef struct WebsAuth_s* WebsAuth;

//-----------------------------------------------------------------------------

struct WebsStream_s
{
  CapeString image;
  
  CapeMutex mutex;
  
  number_t cnt;
  number_t len;
  
}; typedef struct WebsStream_s* WebsStream;

//-----------------------------------------------------------------------------

void webs_del (WebsAuth* p_self)
{
  if (*p_self)
  {
    WebsAuth self = *p_self;
    
    cape_str_del (&(self->module));
    cape_str_del (&(self->method));
    
    cape_udc_del (&(self->cdata));
    cape_udc_del (&(self->clist));
    cape_udc_del (&(self->files));
    
    cape_list_del (&(self->files_to_delete));
    
    cape_str_del (&(self->extra_token));

    CAPE_DEL (p_self, struct WebsAuth_s);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL webs__files__on_del (void* ptr)
{
  CapeString file = ptr;
  
  cape_fs_file_del (file, NULL);
  
  cape_str_del (&file);
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__file__on_vsec (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  WebsAuth self = ptr;

  CapeString vsec = NULL;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  vsec = cape_udc_ext_s (qin->cdata, "secret");
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "secret value is missing");
    goto exit_and_cleanup;
  }
  
  cape_udc_add_s_mv (self->cdata, "vsec", &vsec);
  
  qwebs_request_send_file (&(self->request), self->cdata, err);
  webs_del (&self);

  return CAPE_ERR_NONE;

exit_and_cleanup:

  qwebs_request_send_json (&(self->request), qin->cdata, err);
  webs_del (&self);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__auth__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  WebsAuth self = ptr;
  
  if (qin->err)
  {
    cape_err_set (err, cape_err_code (qin->err), cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  switch (qin->mtype)
  {
    case QBUS_MTYPE_JSON:
    {
      qwebs_request_send_json (&(self->request), qin->cdata, err);
      break;
    }
    case QBUS_MTYPE_FILE:
    {
      cape_log_msg (CAPE_LL_DEBUG, "WEBS", "on call", "handle FILE type");

      // if the file is encrypted check if we have the vsec for it
      if (cape_udc_get_b (qin->cdata, "encrypted", FALSE))
      {
        const CapeString vsec = cape_udc_get_s (qin->cdata, "vsec", NULL);
        if (NULL == vsec)
        {
          // transfer cdata to the local object
          cape_udc_replace_mv (&(self->cdata), &(qin->cdata));
          
          // clean up
          qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
          
          return qbus_continue (self->qbus, "AUTH", "getVaultSecret", qin, (void**)&self, qbus_webs__file__on_vsec, err);
        }
      }
      
      qwebs_request_send_file (&(self->request), qin->cdata, err);
      break;
    }
    default:
    {
      cape_err_set (err, CAPE_ERR_WRONG_VALUE, "not supported mtype");
      goto exit_and_cleanup;
    }
  }

  webs_del (&self);
  return CAPE_UDC_NODE;
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "WEBS", "on call", cape_err_text (err));
  }
  
  if (qin->cdata)
  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "on call", "having payload = %s", h);
    
    cape_str_del (&h);
  }
  
  qwebs_request_send_json (&(self->request), qin->cdata, err);
  webs_del (&self);
  
  return CAPE_UDC_NODE;
}

//-----------------------------------------------------------------------------

CapeString webs_http_parse_line (const CapeString line, const CapeString key_to_seek)
{
  CapeListCursor* cursor;
  CapeList tokens;
  CapeString ret = NULL;
  int run = TRUE;
  
  // split the string into its parts
  tokens = cape_tokenizer_buf (line, cape_str_size (line), ';');
  
  // run through all parts to find the one which fits the key
  cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
  while (run && cape_list_cursor_next (cursor))
  {
    CapeString token = cape_str_trim_utf8 (cape_list_node_data (cursor->node));
    
    CapeString key = NULL;
    CapeString val = NULL;
    
    if (cape_tokenizer_split (token, '=', &key, &val))
    {
      if (cape_str_equal (key_to_seek, key))
      {
        ret = cape_str_trim_c (val, '"');
        run = FALSE;
      }
    }
    
    cape_str_del (&token);
    cape_str_del (&key);
    cape_str_del (&val);
  }
  
  // cleanup
  cape_list_cursor_destroy (&cursor);
  cape_list_del (&tokens);
  
  return ret;
}

//-----------------------------------------------------------------------------

int webs_add_file (WebsAuth self, const char* bufdat, number_t buflen, CapeErr err)
{
  int res;
  
  // local objects
  CapeString uuid = cape_str_uuid ();
  CapeFileHandle fh = cape_fh_new ("/tmp", uuid);

  res = cape_fh_open (fh, O_CREAT | O_RDWR | O_TRUNC, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_fh_write_buf (fh, bufdat, buflen);
  
  if (NULL == self->files)
  {
    self->files = cape_udc_new (CAPE_UDC_LIST, NULL);
  }
  
  // add to the QBUS files list
  cape_udc_add_s_cp (self->files, NULL, cape_fh_file (fh));
  
  // add to the list of files which needs to be deleted
  cape_list_push_back (self->files_to_delete, cape_str_cp (cape_fh_file (fh)));
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_fh_del (&fh);
  cape_str_del (&uuid);
  
  return res;
}

//-----------------------------------------------------------------------------

void webs_add_param (WebsAuth self, const CapeString key, CapeString* p_val)
{
  if (self->cdata == NULL)
  {
    self->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  
  cape_udc_add_s_mv (self->cdata, key, p_val);
}

//-----------------------------------------------------------------------------

void __STDCALL webs__mp__on_part (void* ptr, const char* bufdat, number_t buflen, CapeMap part_values)
{
  WebsAuth self = ptr;
  
  CapeMapNode n = cape_map_find (part_values, "CONTENT-DISPOSITION");
  if (n)
  {
    const CapeString disposition = cape_map_node_value (n);

    cape_log_fmt (CAPE_LL_TRACE, "WEBS", "multipart", "found disposition: %s", disposition);

    CapeString name = webs_http_parse_line (disposition, "name");
    if (name)
    {
      if (cape_str_equal (name, "file"))  // content is file
      {
        CapeErr err = cape_err_new ();
   
        // write a tmp file
        if (webs_add_file (self, bufdat, buflen, err))
        {
          cape_log_fmt (CAPE_LL_ERROR, "WEBS", "multipart", "can't create temporary file: %s", cape_err_text (err));
        }
        
        cape_err_del (&err);
      }
      else // a parameter
      {
        CapeString val = cape_str_sub (bufdat, buflen);
        webs_add_param (self, name, &val);
      }
      
      cape_str_del (&name);
    }
  }
}

//-----------------------------------------------------------------------------

void webs__check_body (WebsAuth self)
{
  // get the body as stream from the incoming request
  CapeStream body = qwebs_request_body (self->request);

  if (self->mime)
  {
    cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "handling content type = %s", self->mime);

    // check mime type
    if (cape_str_begins (self->mime, "multipart"))
    {
      CapeString boundary = webs_http_parse_line (self->mime, "boundary");
      if (boundary)
      {
        QWebsMultipart mp = qwebs_multipart_new (boundary, self, webs__mp__on_part);
        
        qwebs_multipart_process (mp, cape_stream_data (body), cape_stream_size (body));
        
        qwebs_multipart_del (&mp);
        cape_str_del (&boundary);
      }
    }
    else
    {
      if (cape_stream_size (body) > 0)
      {
        // try to convert into a UDC container
        self->cdata = cape_json_from_buf (cape_stream_data (body), cape_stream_size (body));
      }
    }
  }
}

//-----------------------------------------------------------------------------

int webs_auth_call (WebsAuth* p_self, QBusM qin, CapeUdc* p_cdata, CapeErr err)
{
  WebsAuth self = *p_self;

  cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "got info, continue with module = %s, method = %s", self->module, self->method);

  webs__check_body (self);
  
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);

  // transfer the cdata
  cape_udc_replace_mv (&(qin->cdata), &(self->cdata));

  if (p_cdata)
  {
    if (qin->cdata)
    {
      cape_udc_merge_mv (qin->cdata, p_cdata);
    }
    else
    {
      qin->cdata = cape_udc_mv (p_cdata);
    }
  }
  
  // transfer the clist
  cape_udc_replace_mv (&(qin->clist), &(self->clist));

  // transfer the files
  cape_udc_replace_mv (&(qin->files), &(self->files));

  return qbus_continue (self->qbus, self->module, self->method, qin, (void**)&self, qbus_webs__auth__on_call, err);
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__auth__on_ui (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  WebsAuth self = ptr;

  if (qin->err)
  {
    cape_err_set (err, cape_err_code (qin->err), cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  CapeUdc cdata = cape_udc_mv (&(qin->cdata));
  
  return webs_auth_call (&self, qin, &cdata, err);
  
exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "on ui", "auth UI failed = %s", cape_err_text (err));
  }
  
  if (qin->cdata)
  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "on ui", "having payload = %s", h);
    
    cape_str_del (&h);
  }

  qwebs_request_send_json (&(self->request), qin->cdata, err);
  webs_del (&self);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int webs__check_header (WebsAuth self, CapeErr err)
{
  int res;
  CapeMap header_values = qwebs_request_headers (self->request);

  // check header values
  {
    CapeMapNode n = cape_map_find (header_values, "Content-Type");
    if (n)
    {
      self->mime = cape_map_node_value (n);
    }
  }
  
  // check for auth
  {
    CapeMapNode n = cape_map_find (header_values, "Authorization");
    if (n)
    {
      self->auth = cape_map_node_value (n);
    }
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int webs_run (WebsAuth* p_self, CapeErr err)
{
  int res;
  WebsAuth self = *p_self;
  
  res = webs__check_header (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "rest request with module = %s, method = %s", self->module, self->method);

  if (self->auth)
  {
    CapeString auth_type = NULL;
    CapeString auth_cont = NULL;
    
    if (cape_tokenizer_split (self->auth, ' ', &auth_type, &auth_cont))
    {
      int res;
      
      QBusM msg = qbus_message_new (NULL, NULL);
      
      cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "ask AUTH for info");

      qbus_message_clr (msg, CAPE_UDC_NODE);
      
      cape_udc_add_s_cp (msg->cdata, "type", auth_type);
      
      // content needs to be base64 encrypted
      {
        CapeString h = qcrypt__encode_base64_o (auth_cont, cape_str_size (auth_cont));
        cape_udc_add_s_mv (msg->cdata, "content", &h);
      }
      
      res = qbus_send (self->qbus, "AUTH", "getUI", msg, self, qbus_webs__auth__on_ui, err);
      
      qbus_message_del (&msg);
      
      *p_self = NULL;
      
      return CAPE_ERR_CONTINUE;
    }
  }
  else if (self->extra_token)
  {
    int res;
    
    QBusM msg = qbus_message_new (NULL, NULL);
    
    cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "ask AUTH for info");

    qbus_message_clr (msg, CAPE_UDC_NODE);
    
    cape_udc_add_s_cp (msg->cdata, "type", "Token");

    {
      CapeUdc extras = cape_udc_new (CAPE_UDC_NODE, "extras");
      
      cape_udc_add_s_mv (extras, "__T", &(self->extra_token));
      
      cape_udc_add (msg->cdata, &extras);
    }
    
    res = qbus_send (self->qbus, "AUTH", "getUI", msg, self, qbus_webs__auth__on_ui, err);
    
    qbus_message_del (&msg);
    
    *p_self = NULL;
    
    return CAPE_ERR_CONTINUE;
  }
  
  {
    QBusM msg = qbus_message_new (NULL, NULL);
   
    res = webs_auth_call (p_self, msg, NULL, err);
    
    qbus_message_del (&msg);
  }
  
  return res;
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "webs run", "got error: %s", cape_err_text(err));
  }

  // done
  qwebs_request_send_json (&(self->request), NULL, err);
  webs_del (&self);
  
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__json (void* user_ptr, QWebsRequest request, CapeErr err)
{
  CapeList clist = qwebs_request_clist (request);
  if (cape_list_size (clist) >= 2)
  {
    const CapeString module;
    const CapeString method;
    
    CapeListCursor* cursor = cape_list_cursor_create (clist, CAPE_DIRECTION_FORW);
    
    if (cape_list_cursor_next (cursor))
    {
      module = cape_list_node_data (cursor->node);
    }

    if (cape_list_cursor_next (cursor))
    {
      method = cape_list_node_data (cursor->node);
    }

    cape_list_cursor_destroy (&cursor);
    
    WebsAuth webs_auth = CAPE_NEW (struct WebsAuth_s);
    
    webs_auth->module = cape_str_cp (module);
    cape_str_to_upper (webs_auth->module);
    
    webs_auth->method = cape_str_cp (method);
    
    webs_auth->mime = NULL;
    webs_auth->auth = NULL;
    
    webs_auth->qbus = user_ptr;
    webs_auth->request = request;
    
    webs_auth->clist = NULL;
    webs_auth->cdata = NULL;
    webs_auth->files = NULL;
    
    webs_auth->files_to_delete = cape_list_new (webs__files__on_del);
    webs_auth->extra_token = NULL;
    
    // check for authentication
    return webs_run (&webs_auth, err);
  }

  return CAPE_ERR_NONE;
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
    
    WebsAuth webs_auth = CAPE_NEW (struct WebsAuth_s);
    
    webs_auth->module = cape_str_cp (module);
    cape_str_to_upper (webs_auth->module);
    
    webs_auth->method = cape_str_cp (method);
    
    webs_auth->qbus = user_ptr;
    webs_auth->request = request;
    
    webs_auth->clist = clist;
    webs_auth->cdata = NULL;
    webs_auth->files = NULL;

    webs_auth->files_to_delete = cape_list_new (webs__files__on_del);
    webs_auth->extra_token = token;

    // check for authentication
    return webs_run (&webs_auth, err);
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

static int __STDCALL qbus_webs__post__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_webs__post (void* user_ptr, QWebsRequest request, CapeErr err)
{
  WebsStream self = user_ptr;
  
  if (cape_str_equal (qwebs_request_method (request), "POST"))
  {
    CapeString location = NULL;
    
    CapeUdc post_values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    //name=sadsad&name=sadsad&name=sadasd&name=sadas&name=2312&name=sadsd&Street=sad&email=23%40asdasd&phone=213213
    CapeStream body = qwebs_request_body (request);
    
    CapeList values = cape_tokenizer_buf (cape_stream_data(body), cape_stream_size(body), '&');
    if (values)
    {
      CapeListCursor* cursor = cape_list_cursor_create (values, CAPE_DIRECTION_FORW);

      while (cape_list_cursor_next (cursor))
      {
        CapeString key = NULL;
        CapeString val = NULL;
        
        if (cape_tokenizer_split (cape_list_node_data(cursor->node), '=', &key, &val))
        {
          cape_udc_add_s_mv (post_values, key, &val);
        }
        
        cape_str_del (&key);
        cape_str_del (&val);
      }
      
      cape_list_cursor_destroy (&cursor);
    }
    
    cape_list_del (&values);
    
    location = cape_str_cp( cape_udc_get_s (post_values, "location", NULL));
    
    // the body should be formatted with post values
    printf ("BODY: '%s'\n", cape_stream_get (body));
    
    CapeList url_parts = qwebs_request_clist (request);
    if (cape_list_size (url_parts))
    {
      const CapeString module;
      const CapeString method;
      
      CapeListCursor* cursor = cape_list_cursor_create (url_parts, CAPE_DIRECTION_FORW);
      
      if (cape_list_cursor_next (cursor))
      {
        module = cape_list_node_data (cursor->node);
      }

      if (cape_list_cursor_next (cursor))
      {
        method = cape_list_node_data (cursor->node);
      }

      cape_list_cursor_destroy (&cursor);

      {
        int res;
        QBusM msg = qbus_message_new (NULL, NULL);
        
        qbus_message_clr (msg, CAPE_UDC_UNDEFINED);
              
        msg->cdata = cape_udc_mv (&post_values);
        
        res = qbus_send (user_ptr, module, method, msg, NULL, qbus_webs__post__on_call, err);
        
        qbus_message_del (&msg);
      }
    }

    {
      QWebsRequest h = request;
      
      if (location)
      {
        qwebs_request_redirect (&h, location);
      }
      else
      {
        qwebs_request_send_json (&h, NULL, err);
      }
      
    }
    
    cape_udc_del(&post_values);    
    cape_str_del(&location);
  }
  else
  {
    QWebsRequest h = request;

    qwebs_request_send_json (&h, NULL, err);
  }
  
  return CAPE_ERR_CONTINUE;
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
