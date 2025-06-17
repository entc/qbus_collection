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
  QBusMethods methods;         // reference
  
  CapeString module;
};

//-----------------------------------------------------------------------------

QBusCon qbus_con_new (QBusRouter router, QBusMethods methods, const CapeString module_name)
{
  QBusCon self = CAPE_NEW (struct QBusCon_s);
  
  self->engine_context = NULL;
  self->con = NULL;

  self->engine = NULL;
  self->router = router;
  self->methods = methods;
  
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

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "request info: module = %s, sender = %s, method = '%s'", frame->module, frame->sender, frame->method);

  if (cape_str_equal (qbus_engine_con_cid (self->engine, self->con), frame->module))
  {
    CapeErr err = cape_err_new ();
    
    int res = qbus_methods_run (self->methods, frame->method, err);
    if (res)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "routing", "%s", cape_err_text (err));
    }
    
    cape_err_del (&err);
  }
  else
  {
    
    
    
  }
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

void qbus_con_snd (QBusCon self, const CapeString cid, const CapeString method, QBusM msg)
{
  const CapeString own_cid = qbus_engine_con_cid (self->engine, self->con);
  
  if (own_cid)
  {
    QBusFrame frame = qbus_frame_new ();
    
    qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, msg->chain_key, cid, method, own_cid);

    qbus_engine_con_snd (self->engine, self->con, cid, frame);

    qbus_frame_del (&frame);
  }

}

//-----------------------------------------------------------------------------
