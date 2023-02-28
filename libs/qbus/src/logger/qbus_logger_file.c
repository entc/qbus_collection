#include "qbus_logger_file.h"
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
  CapeFileHandle fh;
  CapeString path;
};

//-----------------------------------------------------------------------------

QbusLogCtx __STDCALL qbus_logger_file__dst_new (CapeAioContext aio, CapeUdc config, CapeErr err)
{
  const CapeString path = cape_udc_get_s (config, "path", NULL);
  const CapeString file = cape_udc_get_s (config, "file", NULL);

  // local objects
  CapeFileHandle fh = cape_fh_new (path, file);
  
  if (cape_fh_open (fh, O_WRONLY | O_CREAT | O_APPEND, err))
  {
    return NULL;
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "file logger", "sucessfully open log path = %s, file = %s", path, file);
  
  {
    QbusLogCtx self = CAPE_NEW (struct QbusLogCtx_s);

    self->fh = fh;
    fh = NULL;
    
    return self;
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_file__dst_del (QbusLogCtx* p_self)
{
  if (*p_self)
  {
    QbusLogCtx self = *p_self;

    cape_fh_del (&(self->fh));
    
    CAPE_DEL (p_self, struct QbusLogCtx_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_file__dst_msg (QbusLogCtx ctx, const CapeString remote, const CapeString message)
{
  QbusLogCtx self = (QbusLogCtx)ctx;
  
  CapeDatetime dt;
  CapeStream s = cape_stream_new ();
  
  cape_datetime_utc (&dt);
  
  cape_stream_append_d (s, &dt);
  cape_stream_append_c (s, ' ');
  cape_stream_append_str (s, remote);
  cape_stream_append_str (s, ": qbus-5-0: ");
  cape_stream_append_str (s, message);
  cape_stream_append_c (s, '\n');

  cape_fh_write_buf (self->fh, cape_stream_data (s), cape_stream_size (s));
  
  cape_stream_del (&s);
}

//-----------------------------------------------------------------------------
