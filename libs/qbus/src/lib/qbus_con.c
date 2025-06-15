#include "qbus_con.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_dl.h>

//-----------------------------------------------------------------------------

struct QBusCon_s
{
  QbusPvdCtx engine_context;
  QbusPvdConnection con;

  QBusEngine engine;           // reference
  QBusRouter router;           // reference
  
  CapeString module;
};

//-----------------------------------------------------------------------------

QBusCon qbus_con_new (QBusRouter router, const CapeString module_name)
{
  QBusCon self = CAPE_NEW (struct QBusCon_s);
  
  self->engine_context = NULL;
  self->con = NULL;

  self->engine = NULL;
  self->router = router;
  
  self->module = cape_str_cp (module_name);

  return self;
}

//-----------------------------------------------------------------------------

void qbus_con_del (QBusCon* p_self)
{
  if (*p_self)
  {
    QBusCon self = *p_self;
    
    cape_str_del (&(self->module));
    
    CAPE_DEL (p_self, struct QBusCon_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_con__on_snd (void* user_ptr, QBusFrame frame)
{
  QBusCon self = user_ptr;

  printf ("got frame: %s\n", frame->sender);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_con__on_con (void* user_ptr, const CapeString cid, const CapeString name, number_t type)
{
  QBusCon self = user_ptr;

  switch (type)
  {
    case 1:
    {
      qbus_router_add (self->router, cid, name);
      break;
    }
    case 2:
    {
      qbus_router_rm (self->router, cid, name);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_con_init (QBusCon self, QBusEngines engines, CapeAioContext aio, CapeErr err)
{
  int res;
  
  self->engine = qbus_engines_add (engines, "mqtt", err);
  if (self->engine == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->engine_context = qbus_engine_ctx_new (self->engine, aio, self->module, err);
  if (self->engine_context == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  qbus_engine_ctx_add (self->engine, self->engine_context, &(self->con), NULL, self, qbus_con__on_con, qbus_con__on_snd);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_con_snd (QBusCon self, const CapeString cid, QBusM msg)
{
  QBusFrame frame = qbus_frame_new ();
  
  qbus_engine_con_snd (self->engine, self->con, cid, frame);

  qbus_frame_del (&frame);
}

//-----------------------------------------------------------------------------
