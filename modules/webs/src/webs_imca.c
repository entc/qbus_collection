#include "webs_imca.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>
#include <sys/cape_mutex.h>

// qwebs includes
#include "qwebs_connection.h"

//-----------------------------------------------------------------------------

struct WebsStreamEntity_s
{
  CapeString image;
  
  number_t cnt;
  number_t len;
  
}; typedef struct WebsStreamEntity_s* WebsStreamEntity;

//-----------------------------------------------------------------------------

WebsStreamEntity webs_stream_entity_new ()
{
  WebsStreamEntity self = CAPE_NEW (struct WebsStreamEntity_s);

  self->image = NULL;
  self->cnt = 0;
  self->len = 0;

  return self;
}

//-----------------------------------------------------------------------------

void webs_stream_entity_set (WebsStreamEntity self, CapeString* p_image)
{
  self->cnt++;
  self->len = self->len + cape_str_size (*p_image);
  
  cape_str_replace_mv (&(self->image), p_image);
}

//-----------------------------------------------------------------------------

const CapeString webs_stream_entity_get (WebsStreamEntity self)
{
  return self->image;
}

//-----------------------------------------------------------------------------

struct WebsStream_s
{
  CapeMutex mutex;
  CapeMap streams;
};

//-----------------------------------------------------------------------------

void __STDCALL webs_stream__streams__on_del (void* key, void* val)
{
  
}

//-----------------------------------------------------------------------------

WebsStream webs_stream_new ()
{
  WebsStream self = CAPE_NEW (struct WebsStream_s);
  
  self->mutex = cape_mutex_new ();
  self->streams = cape_map_new (cape_map__compare__s, webs_stream__streams__on_del, NULL);

  return self;
}

//-----------------------------------------------------------------------------

void webs_stream_del (WebsStream* p_self)
{
  if (*p_self)
  {
    WebsStream self = *p_self;
   
    cape_map_del (&(self->streams));
    cape_mutex_del (&(self->mutex));
    
    CAPE_DEL (p_self, struct WebsStream_s);
  }
}

//-----------------------------------------------------------------------------

void webs_stream_reset (WebsStream self)
{
  cape_mutex_lock (self->mutex);
  
  /*
  cape_log_fmt (CAPE_LL_DEBUG, "WSRV", "stream stats", "stream: %li images/s, %f kb/s", self->cnt, (double)self->len / 1024);
  
  self->cnt = 0;
  self->len = 0;
  */
   
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

int webs_stream_add (WebsStream self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  // local objects
  CapeString token = cape_str_uuid ();

  cape_mutex_lock (self->mutex);

  // create a new entry for a stream
  cape_map_insert (self->streams, (void*)token, webs_stream_entity_new ());
  
  cape_mutex_unlock (self->mutex);

  // add output
  qout->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_s_cp (qout->pdata, "token", token);

  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int webs_stream_set (WebsStream self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
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
  
  {
    // try to find the stream
    CapeMapNode n = cape_map_find (self->streams, (void*)token);
    if (n)
    {
      // set a new image
      webs_stream_entity_set (cape_map_node_value (n), &image);
    }
  }
  
  cape_mutex_unlock (self->mutex);
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&image);
  return res;
}

//-----------------------------------------------------------------------------

int webs_stream_get (WebsStream self, QWebsRequest request, CapeErr err)
{
  // get the url parts
  CapeList url_parts = qwebs_request_clist (request);
  
  CapeListCursor* cursor = cape_list_cursor_create (url_parts, CAPE_DIRECTION_FORW);
  
  if (cape_list_cursor_next (cursor))
  {
    const CapeString token = cape_list_node_data (cursor->node);
    
    cape_mutex_lock (self->mutex);

    // monitor
    {
      // try to find the stream
      CapeMapNode n = cape_map_find (self->streams, (void*)token);
      if (n)
      {
        QWebsRequest h = request;
        
        // get the image
        const CapeString image = webs_stream_entity_get (cape_map_node_value (n));
        
        if (image)
        {
          qwebs_request_send_image (&h, image, err);
        }
        else
        {
          qwebs_request_send_json (&h, NULL, 0, err);
        }
      }
    }
    
    cape_mutex_unlock (self->mutex);
  }
  
  cape_list_cursor_destroy (&cursor);

  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------
