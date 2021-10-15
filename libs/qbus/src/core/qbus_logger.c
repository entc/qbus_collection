#include "qbus_logger.h"
#include "qbus_types.h"

// cape includes
#include "fmt/cape_json.h"
#include "sys/cape_log.h"
#include "sys/cape_dl.h"
#include "sys/cape_file.h"
#include <stc/cape_map.h>
#include <sys/cape_socket.h>

//-----------------------------------------------------------------------------

struct QbusLogCtx_Udp_s
{
  CapeAioContext aio;   // reference
  
  CapeAioSocketUdp aio_socket;
  
  CapeString host;
  number_t port;
  
}; typedef struct QbusLogCtx_Udp_s* QbusLogCtx_Udp;

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_udp__dst_on_sent (void* ptr, CapeAioSocketUdp aio_socket, void* userdata)
{
  if (userdata)
  {
    CapeStream s = userdata; cape_stream_del (&s);
  }
}

//-----------------------------------------------------------------------------

QbusLogCtx __STDCALL qbus_logger_udp__dst_new (CapeAioContext aio, CapeUdc config, CapeErr err)
{
  // local objects
  void* handle = NULL;
  
  const CapeString host = cape_udc_get_s (config, "host", NULL);
  number_t port = cape_udc_get_n (config, "port", 0);

  if (cape_str_empty (host) || port == 0)
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "host or port is missing");
    return NULL;
  }
  
  // for udp we need format, host, port
  const CapeString format = cape_udc_get_s (config, "format", NULL);
  
  if (cape_str_equal (format, "syslog"))
  {
    
  }
  else
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "format is not supported");
    return NULL;
  }

  handle = cape_sock__udp__clt_new (host, port, err);
  if (handle == NULL)
  {
    return NULL;
  }

  {
    QbusLogCtx_Udp self = CAPE_NEW (struct QbusLogCtx_Udp_s);
    
    self->aio = aio;
    self->aio_socket = cape_aio_socket__udp__new (handle);
    
    self->host = cape_str_cp (host);
    self->port = port;
    
    cape_aio_socket__udp__cb (self->aio_socket, self, qbus_logger_udp__dst_on_sent, NULL, NULL);
    
    // add the socket to the AIO subsystem
    {
      CapeAioSocketUdp socket = self->aio_socket;
      cape_aio_socket__udp__add (&socket, self->aio, CAPE_AIO_WRITE);
    }

    return (QbusLogCtx)self;
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_udp__dst_del (QbusLogCtx* p_self)
{
  if (*p_self)
  {
    QbusLogCtx_Udp self = (QbusLogCtx_Udp)*p_self;
    
    if (self->aio_socket)
    {
      
    }
    
    CAPE_DEL (p_self, struct QbusLogCtx_Udp_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_udp__dst_msg (QbusLogCtx ctx, const CapeString remote, const CapeString message)
{
  QbusLogCtx_Udp self = (QbusLogCtx_Udp)ctx;
  
  CapeDatetime dt;
  CapeStream s = cape_stream_new ();
  
  cape_datetime_utc (&dt);
  
//  cape_stream_append_str (s, "<30>");
  cape_stream_append_d (s, &dt);
  cape_stream_append_c (s, ' ');
  cape_stream_append_str (s, remote);
  cape_stream_append_str (s, ": qbus-5-0: ");
  cape_stream_append_str (s, message);

 // cape_stream_append_str (s, "<30>Oct 12 12:49:06 host app[12345]: syslog msg");
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "logger", "send udp log message -> %s", self->host);
  
  cape_aio_socket__udp__send (self->aio_socket, self->aio, cape_stream_data (s), cape_stream_size (s), s, self->host, self->port);
}

//-----------------------------------------------------------------------------

struct QBusLoggerEntity_s
{
  QbusLogDst dst;
  QbusLogCtx ctx;
  
}; typedef struct QBusLoggerEntity_s* QBusLoggerEntity;

//-----------------------------------------------------------------------------

void qbus_logger_entity_del (QBusLoggerEntity* p_self);

//-----------------------------------------------------------------------------

QBusLoggerEntity qbus_logger_entity__factory__udp (CapeAioContext aio, CapeUdc config, CapeErr err)
{
  QBusLoggerEntity self = CAPE_NEW (struct QBusLoggerEntity_s);
  
  self->dst.log_dst_new = qbus_logger_udp__dst_new;
  self->dst.log_dst_del = qbus_logger_udp__dst_del;
  self->dst.log_dst_msg = qbus_logger_udp__dst_msg;

  self->ctx = self->dst.log_dst_new (aio, config, err);

  // if there was an issue the ctx is NULL
  if (self->ctx == NULL)
  {
    // cleanup
    qbus_logger_entity_del (&self);
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_logger_entity_del (QBusLoggerEntity* p_self)
{
  if (*p_self)
  {
    QBusLoggerEntity self = *p_self;
    
    if (self->dst.log_dst_del)
    {
      self->dst.log_dst_del (&(self->ctx));
    }
    
    CAPE_DEL (p_self, struct QBusLoggerEntity_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_logger_entity_msg (QBusLoggerEntity self, const CapeString remote, const CapeString message)
{
  if (self->dst.log_dst_msg)
  {
    self->dst.log_dst_msg (self->ctx, remote, message);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger__entity__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusLoggerEntity h = val; qbus_logger_entity_del (&h);
  }
}

//-----------------------------------------------------------------------------

struct QBusLogger_s
{
  CapeMap dest;
};

//-----------------------------------------------------------------------------

QBusLogger qbus_logger_new (void)
{
  QBusLogger self = CAPE_NEW(struct QBusLogger_s);
  
  self->dest = cape_map_new (NULL, qbus_logger__entity__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_logger_del (QBusLogger* p_self)
{
  if (*p_self)
  {
    QBusLogger self = *p_self;
    
    
    CAPE_DEL (p_self, struct QBusLogger_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_logger_init__dest (QBusLogger self, CapeAioContext aio, CapeUdc dest, CapeErr err)
{
  int res;
  CapeUdcCursor* cursor = cape_udc_cursor_new (dest, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString type = cape_udc_get_s (cursor->item, "type", NULL);
    if (type)
    {
      if (cape_str_equal (type, "udp"))
      {
        QBusLoggerEntity le = qbus_logger_entity__factory__udp (aio, cursor->item, err);
        if (le)
        {
          cape_map_insert (self->dest, (void*)cape_str_cp (cape_udc_name (cursor->item)), (void*)le);
        }
        else
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
      }
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

int qbus_logger_init (QBusLogger self, CapeAioContext aio, CapeUdc config, CapeErr err)
{
  int res;
  
  if (config == NULL)
  {
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }

  // check for destination
  {
    CapeUdc dest = cape_udc_get (config, "dest");
    if (dest)
    {
      res = qbus_logger_init__dest (self, aio, dest, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_logger_msg (QBusLogger self, const CapeString remote, const CapeString message)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->dest, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusLoggerEntity le = cape_map_node_value (cursor->node);
    
    qbus_logger_entity_msg (le, remote, message);
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------
