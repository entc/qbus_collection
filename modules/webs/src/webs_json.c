#include "webs_json.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>
#include <fmt/cape_tokenizer.h>
#include <sys/cape_file.h>

// qwebs includes
#include "qwebs_connection.h"
#include "qwebs_multipart.h"

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct WebsJson_s
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
  CapeString extra_token_t;
  CapeString extra_token_p;

  // shortcuts from the header values
  const CapeString mime;
  const CapeString auth;
};

//-----------------------------------------------------------------------------

static void __STDCALL webs__files__on_del (void* ptr)
{
  CapeString file = ptr;
  
  cape_fs_file_del (file, NULL);
  
  cape_str_del (&file);
}

//-----------------------------------------------------------------------------

WebsJson webs_json_new (QBus qbus, QWebsRequest request)
{
  WebsJson self = CAPE_NEW (struct WebsJson_s);

  self->qbus = qbus;
  self->request = request;

  self->module = NULL;
  self->method = NULL;
  
  self->clist = NULL;
  self->cdata = NULL;
  self->files = NULL;
  
  self->files_to_delete = cape_list_new (webs__files__on_del);
  self->extra_token_t = NULL;
  self->extra_token_p = NULL;

  self->mime = NULL;
  self->auth = NULL;
  
  return self;
}

//---------------------------------------------------------------------------

void webs_json_del (WebsJson* p_self)
{
  if (*p_self)
  {
    WebsJson self = *p_self;
    
    cape_str_del (&(self->module));
    cape_str_del (&(self->method));

    cape_udc_del (&(self->cdata));
    cape_udc_del (&(self->clist));
    cape_udc_del (&(self->files));
    
    cape_list_del (&(self->files_to_delete));
    
    cape_str_del (&(self->extra_token_t));
    cape_str_del (&(self->extra_token_p));

    if (self->request)
    {
      CapeErr err = cape_err_new ();
      
      qwebs_request_send_json (&(self->request), NULL, err);
      
      cape_err_del (&err);
    }
    
    CAPE_DEL (p_self, struct WebsJson_s);
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__file__on_vsec (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  WebsJson self = ptr;

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
  webs_json_del (&self);

  return CAPE_ERR_NONE;

exit_and_cleanup:

  qwebs_request_send_json (&(self->request), qin->cdata, err);
  webs_json_del (&self);

  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__auth__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  WebsJson self = ptr;
  
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
      //cape_log_msg (CAPE_LL_DEBUG, "WEBS", "on call", "handle FILE type");

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

  webs_json_del (&self);
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
  webs_json_del (&self);
  
  return CAPE_UDC_NODE;
}

//-----------------------------------------------------------------------------

int webs_add_file (WebsJson self, const char* bufdat, number_t buflen, CapeErr err)
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

void webs_add_param (WebsJson self, const CapeString key, CapeString* p_val)
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
  WebsJson self = ptr;
  
  CapeMapNode n = cape_map_find (part_values, "CONTENT-DISPOSITION");
  if (n)
  {
    const CapeString disposition = cape_map_node_value (n);

    cape_log_fmt (CAPE_LL_TRACE, "WEBS", "multipart", "found disposition: %s", disposition);

    CapeString name = qwebs_parse_line (disposition, "name");
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

void webs__check_body (WebsJson self)
{
  // get the body as stream from the incoming request
  CapeStream body = qwebs_request_body (self->request);

  if (self->mime)
  {
    //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "handling content type = %s", self->mime);

    // check mime type
    if (cape_str_begins (self->mime, "multipart"))
    {
      CapeString boundary = qwebs_parse_line (self->mime, "boundary");
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

int webs_auth_call (WebsJson* p_self, QBusM qin, CapeUdc* p_cdata, CapeErr err)
{
  WebsJson self = *p_self;

  //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "got info, continue with module = %s, method = %s", self->module, self->method);

  webs__check_body (self);
  
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);

  // transfer the cdata
  cape_udc_replace_mv (&(qin->cdata), &(self->cdata));

  if (qin->rinfo == NULL)
  {
    qin->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  
  // remote
  {
    CapeString remote = qwebs_request_remote (self->request);
    if (remote)
    {
      cape_udc_add_s_mv (qin->rinfo, "remote", &remote);
    }
  }
  
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
  if (self->clist)
  {
    cape_udc_replace_mv (&(qin->clist), &(self->clist));
  }

  // transfer the files
  cape_udc_replace_mv (&(qin->files), &(self->files));

  return qbus_continue (self->qbus, self->module, self->method, qin, (void**)&self, qbus_webs__auth__on_call, err);
}

//-----------------------------------------------------------------------------

static int __STDCALL qbus_webs__auth__on_ui (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  WebsJson self = ptr;

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
  webs_json_del (&self);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int webs__check_header (WebsJson self, CapeErr err)
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

int webs_json_run_gen (WebsJson* p_self, CapeErr err)
{
  int res;
  WebsJson self = *p_self;
  
  res = webs__check_header (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "rest request with module = %s, method = %s", self->module, self->method);

  if (self->auth)
  {
    CapeString auth_type = NULL;
    CapeString auth_cont = NULL;
    
    if (cape_tokenizer_split (self->auth, ' ', &auth_type, &auth_cont))
    {
      if (cape_str_equal (auth_type, "Bearer"))
      {
        int res;
        
        QBusM msg = qbus_message_new (NULL, NULL);
        
        qbus_message_clr (msg, CAPE_UDC_NODE);

        // remote
        {
          CapeString remote = qwebs_request_remote (self->request);
          if (remote)
          {
            cape_udc_add_s_mv (msg->cdata, "remote", &remote);
          }
        }

        msg->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);
        
        cape_udc_add_s_cp (msg->pdata, "token", auth_cont);
        
        res = qbus_send (self->qbus, "AUTH", "session_get", msg, self, qbus_webs__auth__on_ui, err);
        if (res)
        {
          
        }
        
        qbus_message_del (&msg);
        
        *p_self = NULL;
        return CAPE_ERR_CONTINUE;
      }
      else
      {
        int res;
        
        QBusM msg = qbus_message_new (NULL, NULL);
        
        //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "ask AUTH for info");
        
        qbus_message_clr (msg, CAPE_UDC_NODE);
        
        cape_udc_add_s_cp (msg->cdata, "type", auth_type);
        
        // content needs to be base64 encrypted
        {
          CapeString h = qcrypt__encode_base64_o (auth_cont, cape_str_size (auth_cont));
          cape_udc_add_s_mv (msg->cdata, "content", &h);
        }

        // remote
        {
          CapeString remote = qwebs_request_remote (self->request);
          if (remote)
          {
            cape_udc_add_s_mv (msg->cdata, "remote", &remote);
          }
        }

        res = qbus_send (self->qbus, "AUTH", "getUI", msg, self, qbus_webs__auth__on_ui, err);
        
        qbus_message_del (&msg);
        
        *p_self = NULL;
        return CAPE_ERR_CONTINUE;
      }
    }
  }
  else if (self->extra_token_t)
  {
    int res;
    
    QBusM msg = qbus_message_new (NULL, NULL);
    
    //cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "ask AUTH for info");

    qbus_message_clr (msg, CAPE_UDC_NODE);
    
    cape_udc_add_s_cp (msg->cdata, "type", "Token");

    {
      CapeUdc extras = cape_udc_new (CAPE_UDC_NODE, "extras");
      
      cape_udc_add_s_mv (extras, "__T", &(self->extra_token_t));
      
      cape_udc_add (msg->cdata, &extras);
    }
    
    res = qbus_send (self->qbus, "AUTH", "getUI", msg, self, qbus_webs__auth__on_ui, err);
    
    qbus_message_del (&msg);
    
    *p_self = NULL;
    
    return CAPE_ERR_CONTINUE;
  }
  else if (self->extra_token_p)
  {
    int res;
    
    QBusM msg = qbus_message_new (NULL, NULL);
    
    cape_log_fmt (CAPE_LL_TRACE, "WEBS", "auth run", "perm token: ask for rinfo");
    
    qbus_message_clr (msg, CAPE_UDC_UNDEFINED);

    msg->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_mv (msg->pdata, "token", &(self->extra_token_p));
    
    {
      // get the body as stream from the incoming request
      CapeStream body = qwebs_request_body (self->request);

      if (cape_stream_size (body) > 0)
      {
        // try to convert into a UDC container
        msg->cdata = cape_json_from_buf (cape_stream_data (body), cape_stream_size (body));
      }
    }
    
    res = qbus_send (self->qbus, "AUTH", "token_perm_get", msg, self, qbus_webs__auth__on_ui, err);
    
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

  webs_json_del (p_self);
  
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------

int webs_json_run (WebsJson* p_self, CapeErr err)
{
  int res;
  WebsJson self = *p_self;

  CapeList clist = qwebs_request_clist (self->request);
  
  if (cape_list_size (clist) >= 2)
  {
    const CapeString module = NULL;
    const CapeString method = NULL;
    
    CapeString token_t = NULL;
    CapeString token_p = NULL;

    CapeListCursor* cursor = cape_list_cursor_create (clist, CAPE_DIRECTION_FORW);
    
    if (cape_list_cursor_next (cursor))
    {
      module = cape_list_node_data (cursor->node);
    }

    if (cape_list_cursor_next (cursor))
    {
      method = cape_list_node_data (cursor->node);
    }

    if (cape_list_cursor_next (cursor))
    {
      const CapeString special = cape_list_node_data (cursor->node);
      if (cape_str_equal (special, "__T"))
      {
        if (cape_list_cursor_next (cursor))
        {
          token_t = cape_str_cp (cape_list_node_data (cursor->node));
        }
      }
      else if (cape_str_equal (special, "__P"))
      {
        if (cape_list_cursor_next (cursor))
        {
          token_p = cape_str_cp (cape_list_node_data (cursor->node));
        }
      }
      else
      {
        self->clist = cape_udc_new (CAPE_UDC_LIST, NULL);

        cape_udc_add_s_cp (self->clist, NULL, cape_list_node_data (cursor->node));

        // check for clist entries
        while (cape_list_cursor_next (cursor))
        {
          cape_udc_add_s_cp (self->clist, NULL, cape_list_node_data (cursor->node));
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
    
    self->module = cape_str_cp (module);
    cape_str_to_upper (self->module);
    
    self->method = cape_str_cp (method);
    
    cape_str_replace_mv (&(self->extra_token_t), &token_t);
    cape_str_replace_mv (&(self->extra_token_p), &token_p);

    // check for authentication
    return webs_json_run_gen (p_self, err);
  }

  webs_json_del (p_self);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void webs_json_set (WebsJson self, const CapeString module, const CapeString method, CapeUdc* p_clist, CapeString* p_token)
{
  cape_str_replace_cp (&(self->module), module);
  cape_str_to_upper (self->module);

  cape_str_replace_cp (&(self->method), method);
  cape_udc_replace_mv (&(self->clist), p_clist);
  
  cape_str_replace_mv (&(self->extra_token_t), p_token);
}

//-----------------------------------------------------------------------------
