#include "qbus.h"
#include "qbus_con.h"
#include "qbus_engines.h"
#include "qbus_router.h"

// cape includes
#include <stc/cape_str.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

struct QBus_s
{
  CapeString qid;             // main ID of the module
  CapeAioContext aio;         // asyncronous IO context

  QBusRouter router;          // owned
  QBusEngines engines;        // owned
  QBusConfig config;          // owned
  
  QBusCon con;                // owned
  QBusMethods methods;
};

//-----------------------------------------------------------------------------

QBus qbus_new (const CapeString module)
{
  QBus self = CAPE_NEW (struct QBus_s);
  
  self->qid = cape_str_uuid ();
  self->aio = cape_aio_context_new ();

  self->router = qbus_router_new ();
  self->engines = qbus_engines_new ("qbus");   // always use this path
  self->config = qbus_config_new (module);
  self->methods = qbus_methods_new ();

  self->con = NULL;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_del (QBus* p_self)
{
  if (*p_self)
  {
    QBus self = *p_self;
   
    qbus_con_del (&(self->con));

    qbus_methods_del (&(self->methods));
    qbus_config_del (&(self->config));
    qbus_engines_del (&(self->engines));
    qbus_router_del (&(self->router));
    
    cape_aio_context_del (&(self->aio));
    cape_str_del (&(self->qid));
    
    CAPE_DEL (p_self, struct QBus_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_init (QBus self, int argc, char *argv[], CapeErr err)
{
  int res;
  
  // parse arguments and load config
  res = qbus_config_init (self->config, argc, argv, err);
  if (res)
  {
    return res;
  }
  
  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    return res;
  }
  
  // create
  self->con = qbus_con_new (self->router, self->methods, qbus_config_name (self->config));

  res = qbus_con_init (self->con, self->engines, self->aio, err);
  if (res)
  {
    return res;
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_wait__intern (QBus self, CapeErr err)
{
  int res;
  
  // activate signal handling strategy
  res = cape_aio_context_set_interupts (self->aio, TRUE, TRUE, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // wait infinite and let the AIO subsystem handle all events
  res = cape_aio_context_wait (self->aio, err);
  
exit_and_cleanup:
  
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "wait", "runtime error: %s", cape_err_text (err));
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus__intern__no_route (QBus self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_on_msg on_msg)
{
  // log
  cape_log_fmt (CAPE_LL_WARN, "QBUS", "msg forward", "no route to module %s", module);
  
  if (on_msg)
  {
    if (msg->err)
    {
      cape_err_del (&(msg->err));
    }
    
    // create a new error object
    msg->err = cape_err_new ();
    
    // set the error
    cape_err_set (msg->err, CAPE_ERR_NOT_FOUND, "no route to module");
    
    {
      // create a temporary error object
      CapeErr err = cape_err_new ();
      
      int res = on_msg (self, ptr, msg, NULL, err);
      if (res)
      {
        // TODO: handle error
        
      }
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_register (QBus self, const CapeString method, void* user_ptr, fct_qbus_on_msg on_msg, fct_qbus_on_rm on_rm, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------

int qbus_send (QBus self, const CapeString module, const CapeString method, QBusM msg, void* user_ptr, fct_qbus_on_msg on_msg, CapeErr err)
{
  // correct chain key
  // -> the key for the start must be NULL
  cape_str_del (&(msg->chain_key));

  if (cape_str_compare (module, qbus_config_name (self->config)))
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "execute local request on '%s'", module);
    
    /*
    QbusRouteWorkerCtx ctx = CAPE_NEW (struct QbusRouteWorkerCtx_s);
    
    ctx->route = self;
    ctx->qin = qbus_message_data_mv (msg);
    
    ctx->module = NULL;
    ctx->method = cape_str_cp (method);
    
    ctx->ptr = ptr;
    ctx->onMsg = onMsg;
    
    ctx->conn = NULL;
    ctx->cont = FALSE;
    
    qbus_route_request__local_request__worker (ctx, 0, 0);
    
    //cape_queue_add (self->queue, NULL, qbus_route_request__local_request__worker, NULL, ctx, 0);
    */
    return CAPE_ERR_CONTINUE;
  }
  else
  {
    const CapeString cid = qbus_router_get (self->router, module);
    
    if (cid)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "send", "run RPC on %s", cid);

      qbus_con_snd (self->con, cid, method, msg);
      
      return CAPE_ERR_CONTINUE;
    }
    else
    {
      qbus__intern__no_route (self, module, method, msg, user_ptr, on_msg);
      
      return cape_err_set (err, CAPE_ERR_NOT_FOUND, "no route to module");
    }
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_continue (QBus self, const CapeString module, const CapeString method, QBusM qin, void** p_user_ptr, fct_qbus_on_msg on_msg, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------

CapeAioContext qbus_aio (QBus self)
{
  return self->aio;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_modules (QBus self)
{
  return qbus_router_list (self->router);
}

//-----------------------------------------------------------------------------

QBusConfig qbus_config (QBus self)
{
  return self->config;
}

//-----------------------------------------------------------------------------

void qbus_log_msg (QBus self, const CapeString remote, const CapeString message)
{
  // TODO
  //qbus_logger_msg (self->logger, remote, message);
}

//-----------------------------------------------------------------------------

void qbus_log_fmt (QBus self, const CapeString remote, const char* format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  
  {
    CapeString h = cape_str_flp (format, ptr);
    
    // TODO
    //qbus_logger_msg (self->logger, remote, h);
    
    cape_str_del (&h);
  }

  va_end(ptr);
}

//-----------------------------------------------------------------------------

void qbus_instance (const char* name, void* ptr, fct_qbus_on_init on_init, fct_qbus_on_done on_done, int argc, char *argv[])
{
  int res;
  
  // local objects
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new (argv[1]);
  
  res = qbus_init (qbus, argc, argv, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  if (on_init)
  {
    on_init (qbus, ptr, &ptr, err);
  }
    
  res = qbus_wait__intern (qbus, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  qbus_del (&qbus);
  cape_err_del (&err);
  
  return res;
}
