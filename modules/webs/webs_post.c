#include "webs_post.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>
#include <fmt/cape_tokenizer.h>

// qwebs includes
#include "qwebs_connection.h"
#include "qwebs_multipart.h"

//-----------------------------------------------------------------------------

struct WebsPost_s
{
  QBus qbus;
  
  QWebsRequest request;
  
  const CapeString mime;
  
  CapeString location;
  CapeUdc form_data;
};

//-----------------------------------------------------------------------------

WebsPost webs_post_new (QBus qbus, QWebsRequest request)
{
  WebsPost self = CAPE_NEW (struct WebsPost_s);

  self->qbus = qbus;
  self->request = request;
  
  self->mime = NULL;
  
  self->location = NULL;
  self->form_data = NULL;
  
  return self;
}

//---------------------------------------------------------------------------

void webs_post_del (WebsPost* p_self)
{
  if (*p_self)
  {
    WebsPost self = *p_self;
    
    cape_str_del (&(self->location));
    cape_udc_del (&(self->form_data));
    
    if (self->request)
    {
      CapeErr err = cape_err_new ();
      
      qwebs_request_send_json (&(self->request), NULL, err);
      
      cape_err_del (&err);
    }
    
    CAPE_DEL (p_self, struct WebsPost_s);
  }
}

//-----------------------------------------------------------------------------

int webs_post__header (WebsPost self, CapeErr err)
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
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL webs_post__parse__multipart__on_part (void* ptr, const char* bufdat, number_t buflen, CapeMap part_values)
{
  WebsPost self = ptr;
  
  CapeMapNode n = cape_map_find (part_values, "CONTENT-DISPOSITION");
  if (n)
  {
    const CapeString disposition = cape_map_node_value (n);

    cape_log_fmt (CAPE_LL_TRACE, "WEBS", "multipart", "found disposition: %s", disposition);

    CapeString name = qwebs_parse_line (disposition, "name");
    if (name)
    {
      CapeString h1 = cape_str_sub (bufdat, buflen);
      if (h1)
      {
        CapeString h2 = qwebs_decode_run (h1);
        cape_udc_add_s_mv (self->form_data, name, &h2);
      }
      
      cape_str_del (&h1);
      cape_str_del (&name);
    }
  }
}

//-----------------------------------------------------------------------------

int webs_post__parse__multipart (WebsPost self, CapeErr err)
{
  CapeStream body = qwebs_request_body (self->request);

  CapeString boundary = qwebs_parse_line (self->mime, "boundary");
  if (boundary)
  {
    QWebsMultipart mp = qwebs_multipart_new (boundary, self, webs_post__parse__multipart__on_part);
    
    qwebs_multipart_process (mp, cape_stream_data (body), cape_stream_size (body));
    
    qwebs_multipart_del (&mp);
    cape_str_del (&boundary);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int webs_post__parse__list (WebsPost self, CapeErr err)
{
  CapeStream body = qwebs_request_body (self->request);

  // name=sadsad&name=sadsad&name=sadasd&name=sadas&name=2312&name=sadsd&Street=sad&email=23%40asdasd&phone=213213
  CapeList values = cape_tokenizer_buf (cape_stream_data(body), cape_stream_size(body), '&');
  if (values)
  {
    CapeListCursor* cursor = cape_list_cursor_create (values, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (cursor))
    {
      CapeString key = NULL;
      CapeString val = NULL;
      
      if (cape_tokenizer_split (cape_list_node_data (cursor->node), '=', &key, &val))
      {
        CapeString h2 = qwebs_decode_run (val);
        cape_udc_add_s_mv (self->form_data, key, &h2);
      }
      
      cape_str_del (&key);
      cape_str_del (&val);
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_list_del (&values);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int webs_post__parse (WebsPost self, CapeErr err)
{
  int res;
  
  self->form_data = cape_udc_new (CAPE_UDC_NODE, NULL);

  // check mime type
  if (cape_str_begins (self->mime, "multipart"))
  {
    res = webs_post__parse__multipart (self, err);
  }
  else
  {
    res = webs_post__parse__list (self, err);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL webs_post_run__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_DEBUG, "WEBS", "post on call", "returned");

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "WEBS", "post on call", "got error: %s", cape_err_text (err));
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int webs_post_run (WebsPost* p_self, CapeErr err)
{
  int res;
  WebsPost self = *p_self;
  
  if (cape_str_equal (qwebs_request_method (self->request), "POST"))
  {
    res = webs_post__header (self, err);

    res = webs_post__parse (self, err);
    
    // try to extract the location
    self->location = cape_str_cp (cape_udc_get_s (self->form_data, "location", NULL));
    
    CapeList url_parts = qwebs_request_clist (self->request);
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
              
        msg->cdata = cape_udc_mv (&(self->form_data));
        
        res = qbus_send (self->qbus, module, method, msg, self, webs_post_run__on_call, err);
        
        qbus_message_del (&msg);
      }
    }

    {
      if (self->location)
      {
        cape_log_fmt (CAPE_LL_DEBUG, "WEBS", "post run", "submit redirect to = '%s'", self->location);
        
        qwebs_request_redirect (&(self->request), self->location);
      }
      else
      {
        qwebs_request_send_json (&(self->request), NULL, err);
      }
    }
  }

  webs_post_del (p_self);
  
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------
