#include "webs_enjs.h"

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

struct WebsEnjs_s
{
  QBus qbus;
  
  QWebsRequest request;

  CapeString module;
  CapeString method;
  
  CapeString encrypted_content;
  
  // the data for QBUS
  CapeUdc files;
  
  // shortcuts from the header values
  const CapeString mime;
  const CapeString auth;

  CapeList files_to_delete;
  CapeString vsec;
  
  number_t ttl;
};

//-----------------------------------------------------------------------------

static void __STDCALL webs__files__on_del (void* ptr)
{
  CapeString file = ptr;
  
  cape_fs_file_rm (file, NULL);
  
  cape_str_del (&file);
}

//-----------------------------------------------------------------------------

WebsEnjs webs_enjs_new (QBus qbus, QWebsRequest request)
{
  WebsEnjs self = CAPE_NEW (struct WebsEnjs_s);

  self->qbus = qbus;
  self->request = request;

  self->module = NULL;
  self->method = NULL;
  
  self->encrypted_content = NULL;
  
  self->files = NULL;
  
  self->mime = NULL;
  self->auth = NULL;
  
  self->files_to_delete = cape_list_new (webs__files__on_del);
  self->vsec = NULL;
  
  self->ttl = 0;
  
  return self;
}

//---------------------------------------------------------------------------

void webs_enjs_del (WebsEnjs* p_self)
{
  if (*p_self)
  {
    WebsEnjs self = *p_self;
    
    cape_list_del (&(self->files_to_delete));
    cape_str_del (&(self->vsec));

    cape_str_del (&(self->module));
    cape_str_del (&(self->method));

    cape_str_del (&(self->encrypted_content));
    
    cape_udc_del (&(self->files));
    
    if (self->request)
    {
      CapeErr err = cape_err_new ();
      
      qwebs_request_send_json (&(self->request), NULL, self->ttl, err);
      
      cape_err_del (&err);
    }
    
    CAPE_DEL (p_self, struct WebsEnjs_s);
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL webs_enjs_run__on_vsec (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  WebsEnjs self = ptr;
  
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
  
  cape_udc_add_s_mv (self->files, "vsec", &vsec);
  
  qwebs_request_send_file (&(self->request), self->files, err);
  webs_enjs_del (&self);
  
  return CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  qwebs_request_send_json (&(self->request), qin->cdata, self->ttl, err);
  webs_enjs_del (&self);
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL webs_enjs_run__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  WebsEnjs self = ptr;
  
  // local objects
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  
  if (qin->err)
  {
    res = cape_err_set (err, cape_err_code (qin->err), cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  switch (qin->mtype)
  {
    case QBUS_MTYPE_JSON:
    {
      if (qin->cdata)
      {
        h1 = cape_json_to_s__strict (qin->cdata);
        if (h1 == NULL)
        {
          res = cape_err_set (err, CAPE_ERR_PARSER, "can't serialize");
          goto exit_and_cleanup;
        }
        
        // encrypt response
        h2 = qcrypt__encrypt (self->vsec, h1, err);
        if (h2 == NULL)
        {
          res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "can't encrypt");
          goto exit_and_cleanup;
        }

        qwebs_request_send_buf (&(self->request), h2, "text/plain", self->ttl, err);
      }
      else
      {
        qwebs_request_send_json (&(self->request), NULL, self->ttl, err);
      }
      
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
          // temporary store qin->cdata
          cape_udc_replace_mv (&(self->files), &(qin->cdata));
          
          // clean up
          qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
          
          return qbus_continue (self->qbus, "AUTH", "getVaultSecret", qin, (void**)&self, webs_enjs_run__on_vsec, err);
        }
      }
      
      qwebs_request_send_file (&(self->request), qin->cdata, err);
      break;
    }
    default:
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "not supported mtype");
      goto exit_and_cleanup;
    }
  }

  res = CAPE_UDC_NODE;
  
exit_and_cleanup:

  cape_str_del (&h1);
  cape_str_del (&h2);
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "WEBS", "on call", cape_err_text (err));
  }
  
  qwebs_request_send_json (&(self->request), qin->cdata, self->ttl, err);
  webs_enjs_del (&self);
  
  return res;
}

//-----------------------------------------------------------------------------

int webs_enjs_run__add_file (WebsEnjs self, const char* bufdat, number_t buflen, CapeErr err)
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

void __STDCALL webs_enjs_run__mp__on_part (void* ptr, const char* bufdat, number_t buflen, CapeMap part_values)
{
  WebsEnjs self = ptr;
  
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
        if (webs_enjs_run__add_file (self, bufdat, buflen, err))
        {
          cape_log_fmt (CAPE_LL_ERROR, "WEBS", "multipart", "can't create temporary file: %s", cape_err_text (err));
        }
        
        cape_err_del (&err);
      }
      else // a parameter
      {
        CapeString val = cape_str_sub (bufdat, buflen);
       // webs_add_param (self, name, &val);
      }
      
      cape_str_del (&name);
    }
  }
}

//-----------------------------------------------------------------------------

void webs_enjs_run__body (WebsEnjs self)
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
        QWebsMultipart mp = qwebs_multipart_new (boundary, self, webs_enjs_run__mp__on_part);
        
        qwebs_multipart_process (mp, cape_stream_data (body), cape_stream_size (body));
        
        qwebs_multipart_del (&mp);
        cape_str_del (&boundary);
      }
    }
    else
    {
      if (cape_stream_size (body) > 0)
      {
        self->encrypted_content = cape_stream_to_s (body);
      }
    }
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL webs_enjs_run__on_session_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  WebsEnjs self = ptr;

  // local objects
  CapeString content = NULL;
  CapeUdc cdata = NULL;

  if (qin->err)
  {
    res = cape_err_set (err, cape_err_code (qin->err), cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NOROLE");
    goto exit_and_cleanup;
  }
  
  self->vsec = cape_udc_ext_s (qin->pdata, "vsec");
  if (self->vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NOVSEC");
    goto exit_and_cleanup;
  }
  
  // optional to tell the client how long the session might be valid
  self->ttl = cape_udc_get_n (qin->pdata, "ttl", 0);

  // extract averything from the body
  webs_enjs_run__body (self);

  if (self->encrypted_content)
  {
    content = qcrypt__decrypt (self->vsec, self->encrypted_content, err);
    if (content == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_PARSER, "can't decrypt");
      goto exit_and_cleanup;
    }
  }

  if (content)
  {
    cdata = cape_json_from_s (content);
    if (cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_PARSER, "can't parse json");
      goto exit_and_cleanup;
    }
  }

  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);

  // remote
  {
    CapeString remote = qwebs_request_remote (self->request);
    if (remote)
    {
      cape_udc_add_s_mv (qin->rinfo, "remote", &remote);
    }
  }

  if (cdata)
  {
    cape_udc_replace_mv (&(qin->cdata), &cdata);
  }

  // transfer the files
  cape_udc_replace_mv (&(qin->files), &(self->files));
  
  // continue
  // -> use the direct return, otherwise need to check self
  res = qbus_continue (self->qbus, self->module, self->method, qin, (void**)&self, webs_enjs_run__on_call, err);
  
exit_and_cleanup:
  
  cape_str_del (&content);
  
  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "on ui", "auth UI failed = %s", cape_err_text (err));

    if (self)
    {
      // send the error back
      qwebs_request_send_json (&(self->request), qin->cdata, self->ttl, err);
    }
  }
  
  webs_enjs_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int webs_enjs_run__check_header (WebsEnjs self, CapeErr err)
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
    else
    {
      n = cape_map_find (header_values, "content-type");
      if (n)
      {
        self->mime = cape_map_node_value (n);
      }
    }
  }
  
  // check for auth
  {
    CapeMapNode n = cape_map_find (header_values, "Authorization");
    if (n)
    {
      self->auth = cape_map_node_value (n);
    }
    else
    {
      n = cape_map_find (header_values, "authorization");
      if (n)
      {
        self->auth = cape_map_node_value (n);
      }
    }
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int webs_enjs_run__auth (WebsEnjs* p_self, CapeErr err)
{
  int res;
  WebsEnjs self = *p_self;

  // local objects
  CapeString auth_type = NULL;
  CapeString auth_cont = NULL;
  QBusM msg = NULL;

  res = webs_enjs_run__check_header (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  if (NULL == self->auth)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NOAUTH");
    goto exit_and_cleanup;
  }
  
  if (!cape_tokenizer_split (self->auth, ' ', &auth_type, &auth_cont))
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NOAUTH");
    goto exit_and_cleanup;
  }

  if (!cape_str_equal (auth_type, "Bearer"))
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NOAUTH");
    goto exit_and_cleanup;
  }

  // allocate the message object
  msg = qbus_message_new (NULL, NULL);
  
  // reset the message object to a node udc
  qbus_message_clr (msg, CAPE_UDC_NODE);
  
  // decode base64
  {
    CapeStream content_stream = qcrypt__decode_base64_s (auth_cont);
    
    if (content_stream)
    {
      msg->pdata = cape_json_from_buf (cape_stream_data (content_stream), cape_stream_size (content_stream), NULL);
      cape_stream_del (&content_stream);
    }
  }

  // remote
  {
    CapeString remote = qwebs_request_remote (self->request);
    if (remote)
    {
      cape_udc_add_s_mv (msg->cdata, "remote", &remote);
    }
  }

  res = qbus_send (self->qbus, "AUTH", "session_get", msg, self, webs_enjs_run__on_session_get, err);
  if (res)
  {
    
  }
  
  // self was transfered to the qbus event chain
  *p_self = NULL;
  res = CAPE_ERR_CONTINUE;
  
exit_and_cleanup:

  qbus_message_del (&msg);

  cape_str_del (&auth_type);
  cape_str_del (&auth_cont);
  
  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "webs run", "got error: %s", cape_err_text(err));

    if (*p_self)
    {
      // send the error back
      qwebs_request_send_json (&(self->request), NULL, self->ttl, err);
    }
  }

  webs_enjs_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int webs_enjs_run (WebsEnjs* p_self, CapeErr err)
{
  int res;
  WebsEnjs self = *p_self;

  CapeList clist = qwebs_request_clist (self->request);
  
  if (cape_list_size (clist) >= 2)
  {
    const CapeString module = NULL;
    const CapeString method = NULL;
    
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
    
    self->module = cape_str_cp (module);
    cape_str_to_upper (self->module);
    
    self->method = cape_str_cp (method);
    
    // check for authentication
    return webs_enjs_run__auth (p_self, err);
  }

  webs_enjs_del (p_self);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
