#include "qbus_logger_udp.h"
#include "qbus_types.h"

// cape includes
#include "fmt/cape_json.h"
#include "sys/cape_log.h"
#include "sys/cape_dl.h"
#include "sys/cape_file.h"
#include <stc/cape_map.h>
#include <sys/cape_socket.h>

//-----------------------------------------------------------------------------

struct QbusLogCtx_s
{
  CapeAioContext aio;   // reference
  
  CapeAioSocketUdp aio_socket;
  
  CapeString host;
  number_t port;
  
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_udp__dst_on_sent (void* ptr, CapeAioSocketUdp aio_socket, void* userdata)
{
  if (userdata)
  {
    CapeStream s = userdata; cape_stream_del (&s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_udp__dst_on_recv (void* ptr, CapeAioSocketUdp aio_socket, const char* bufdat, number_t buflen, const char* host)
{
  printf ("RECV: %s\n", bufdat);
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
    QbusLogCtx self = CAPE_NEW (struct QbusLogCtx_s);
    
    self->aio = aio;
    self->aio_socket = cape_aio_socket__udp__new (handle);
    
    self->host = cape_str_cp (host);
    self->port = port;
    
    cape_aio_socket__udp__cb (self->aio_socket, self, qbus_logger_udp__dst_on_sent, qbus_logger_udp__dst_on_recv, NULL);
    
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
    QbusLogCtx self = *p_self;
    
    if (self->aio_socket)
    {
      
    }
    
    CAPE_DEL (p_self, struct QbusLogCtx_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_udp__dst_msg (QbusLogCtx ctx, const CapeString remote, const CapeString message)
{
  QbusLogCtx self = (QbusLogCtx)ctx;
  
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
