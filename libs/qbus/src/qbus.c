#include "qbus.h"
#include "qbus_con.h"
#include "qbus_engines.h"
#include "qbus_router.h"
#include "qbus_methods.h"
#include "qbus_agent.h"

// cape includes
#include <stc/cape_str.h>
#include <sys/cape_log.h>
#include <fmt/cape_args.h>

// c includes
#include <stdlib.h>
#include <stdio.h>

#ifdef __WINDOWS_OS

#include <windows.h>

#else

#include <termios.h>
#include <unistd.h>

#endif

//-----------------------------------------------------------------------------

struct QBus_s
{
  CapeAioContext aio;         // asyncronous IO context

  QBusRouter router;          // owned
  QBusEngines engines;        // owned
  QBusConfig config;          // owned
  QBusAgent agent;            // owned

  QBusCon con;                // owned
  QBusMethods methods;
};

//-----------------------------------------------------------------------------

QBus qbus_new (const CapeString module)
{
  QBus self = CAPE_NEW (struct QBus_s);

  self->aio = cape_aio_context_new ();

  self->router = qbus_router_new ();
  self->engines = qbus_engines_new ("qbus");   // always use this path
  self->config = qbus_config_new (module);
  self->agent = qbus_agent_new (self->router);
  self->methods = qbus_methods_new (self);

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
    qbus_agent_del (&(self->agent));
    qbus_config_del (&(self->config));
    qbus_engines_del (&(self->engines));
    qbus_router_del (&(self->router));

    cape_aio_context_del (&(self->aio));

    CAPE_DEL (p_self, struct QBus_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_on_res (void* user_ptr, QBusMethodItem mitem, QBusM* p_msg)
{
  QBus self = user_ptr;

  const CapeString cid = qbus_method_item_cid (mitem);  
  if (cid)
  {
    qbus_con_snd (self->con, cid, NULL, qbus_method_item_skey (mitem), QBUS_FRAME_TYPE_MSG_RES, *p_msg);

    qbus_message_del (p_msg);
  }
  else
  {
    // the mitem skey has the original the request context
    const CapeString saves_key = qbus_method_item_skey (mitem);

    /*
    {
      QBusM msg = *p_msg;
      
      // correct the qin skey environment
      cape_str_replace_cp (&(msg->chain_key), saves_key);
      cape_str_del (&(msg->sender));
    }
     */

    qbus_methods__rpc_queue (self->methods, mitem, p_msg, saves_key);
  }
}

//-----------------------------------------------------------------------------

int qbus_init__preconditions (QBus self, CapeUdc* p_args, CapeErr err)
{
  int res;
  
  // parse arguments and load config
  qbus_config_init (self->config, p_args);
  
  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    return res;
  }
  
  // activate signal handling strategy
  // avoid that other threads got terminated by sigint
  res = cape_aio_context_set_interupts (self->aio, TRUE, TRUE, err);

  return res;
}

//-----------------------------------------------------------------------------

int qbus_init__submodules (QBus self, CapeErr err)
{
  int res;

  res = qbus_methods_init (self->methods, qbus_config_n (self->config, "threads", 4), self, qbus_on_res, err);
  if (res)
  {
    return res;
  }
  
  // create
  self->con = qbus_con_new (self->router, self->methods, qbus_config_name (self->config));
  
  res = qbus_con_init (self->con, self->engines, self->aio, qbus_config_s (self->config, "h", "127.0.0.1"), err);
  if (res)
  {
    return res;
  }
  
  // optional start the agent
  if (qbus_config_b (self->config, "run_agent", FALSE))
  {
    res = qbus_agent_init (self->agent, self->aio, qbus_config_n (self->config, "agent_port", 10161), err);
    if (res)
    {
      return res;
    }
  }

  return res;
}

//-----------------------------------------------------------------------------

int qbus_init (QBus self, CapeUdc* p_args, CapeErr err)
{
  int res;
  
  res = qbus_init__preconditions (self, p_args, err);
  if (res)
  {
    return res;
  }
  
  res = qbus_init__submodules (self, err);
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_wait__intern (QBus self, CapeErr err)
{
  int res;

  // wait infinite and let the AIO subsystem handle all events
  res = cape_aio_context_wait (self->aio, err);

  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "wait", "runtime error: %s", cape_err_text (err));
  }

  return res;
}

//-----------------------------------------------------------------------------

/*
void qbus__intern__no_route (QBus self, const char* skey)
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
    cape_err_set_fmt (msg->err, CAPE_ERR_NOT_FOUND, "no route to module [%s]", module);

    {
      // create a temporary error object
      CapeErr err = cape_err_new ();

      // TODO: add this to queue
      int res = on_msg (self, ptr, msg, NULL, err);
      if (res)
      {
        // TODO: handle error

      }

      cape_err_del (&err);
    }
  }
}
*/

//-----------------------------------------------------------------------------

int qbus_register (QBus self, const CapeString method, void* user_ptr, fct_qbus_on_msg on_msg, fct_qbus_on_rm on_rm, CapeErr err)
{
  return qbus_methods__rpc_add (self->methods, method, user_ptr, on_msg, on_rm, err);
}

//-----------------------------------------------------------------------------

int qbus_request (QBus self, const CapeString module, const CapeString method, QBusM msg, void* user_ptr, fct_qbus_on_msg on_msg, CapeErr err)
{
  int res;
  
  // local objects
  CapeString module_upper_case = cape_str_cp (module);
  cape_str_to_upper (module_upper_case);

  if (cape_str_compare (module_upper_case, qbus_config_name (self->config)))
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "execute local request on '%s'", module_upper_case);

    const CapeString saves_key = qbus_methods_save (self->methods, user_ptr, on_msg, msg->chain_key, msg->sender, msg->rinfo, NULL);

    // need to clone the qin
    // TODO: check why we need this
    QBusM qin = qbus_message_mv (msg);

    // correct the qin skey environment
    cape_str_replace_cp (&(qin->chain_key), saves_key);
    cape_str_del (&(qin->sender));

    res = qbus_methods_run (self->methods, method, saves_key, &qin, err);
    if (res)
    {
      
    }
  }
  else
  {
    const CapeString cid_dst = qbus_router_get (self->router, module_upper_case);

    if (cid_dst)
    {
      const CapeString skey = qbus_methods_save (self->methods, user_ptr, on_msg, msg->chain_key, msg->sender, msg->rinfo, cid_dst);

      //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "send", "run RPC [%s:%s] on %s with skey = %s", module, method, cid_dst, skey);

      qbus_con_snd (self->con, cid_dst, method, skey, QBUS_FRAME_TYPE_MSG_REQ, msg);

      res = CAPE_ERR_NONE;
    }
    else
    {
      // log
      cape_log_fmt (CAPE_LL_WARN, "QBUS", "msg forward", "no route to module %s", module);

      {
        QBusMethodItem mitem = qbus_method_item_new (user_ptr, on_msg, msg->chain_key, msg->sender, NULL, msg->rinfo);

        // need to clone the qin
        // TODO: check why we need this
        QBusM qin = qbus_message_mv (msg);

        qin->err = cape_err_new ();
        cape_err_set_fmt__i (qin->err, 0, NULL, CAPE_ERR_NOT_FOUND, "no route to module [%s]", module_upper_case);
        
        qbus_methods__rpc_queue (self->methods, mitem, &qin, qin->chain_key);
        
        qbus_method_item_del (&mitem);
      }
      
      res = CAPE_ERR_NONE;
    }
  }
  
  cape_str_del (&module_upper_case);
  return res;
}

//-----------------------------------------------------------------------------

int qbus_send (QBus self, const CapeString module, const CapeString method, QBusM msg, void* user_ptr, fct_qbus_on_msg on_msg, CapeErr err)
{
  // correct save key and sender
  // -> the key for the start must be NULL
  cape_str_del (&(msg->chain_key));
  cape_str_del (&(msg->sender));

  return qbus_request (self, module, method, msg, user_ptr, on_msg, err);
}

//-----------------------------------------------------------------------------

int qbus_continue (QBus self, const CapeString module, const CapeString method, QBusM qin, void** p_user_ptr, fct_qbus_on_msg on_msg, CapeErr err)
{
  int res;

  if (p_user_ptr)
  {
    res = qbus_request (self, module, method, qin, *p_user_ptr, on_msg, err);
    *p_user_ptr = NULL;
  }
  else
  {
    res = qbus_request (self, module, method, qin, NULL, on_msg, err);
  }
  
  if (res)
  {
    return res;
  }
  else
  {
    // always return continue
    return CAPE_ERR_CONTINUE;
  }
}

//-----------------------------------------------------------------------------

int qbus_save (QBus self, QBusM msg, CapeString* p_skey, CapeErr err)
{
  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "save", "save message, skey = %s, sender = %s", msg->chain_key, msg->sender);
  
  // use this key directly
  cape_str_replace_cp (p_skey, msg->chain_key);
  
  /*
  
  // save this context and overrides the given pointer to the skey
  cape_str_replace_cp (p_skey, qbus_methods_save (self->methods, NULL, NULL, msg->chain_key, msg->sender, msg->rinfo, NULL));
  
  */
  
  // always return continue
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------

int qbus_response (QBus self, const CapeString skey, QBusM msg, CapeErr err)
{
  QBusMethodItem mitem = qbus_methods_load (self->methods, skey);

  if (mitem)
  {
    // need to clone the qin
    // TODO: check why we need this
    QBusM qin = qbus_message_mv (msg);

    qbus_methods_response (self->methods, mitem, &qin, err);

    qbus_method_item_del (&mitem);
    
    return CAPE_ERR_NONE;
  }
  else
  {
    return cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.MITEM_NOT_FOUND");
  }
}

//-----------------------------------------------------------------------------

int qbus_subscribe (QBus self, const CapeString module, const CapeString ident, void* user_ptr, fct_qbus_on_val on_val, CapeErr err)
{
  int res;
  
  // local objects
  CapeString topic = NULL;
  CapeString module_upper_case = cape_str_cp (module);
  cape_str_to_upper (module_upper_case);

  // create the topic
  topic = cape_str_catenate_c (module_upper_case, '_', ident);
  
  // this will activate the subscription on the connection
  qbus_con_sub_add (self->con, topic);
  
  // this will save the callback related to the topic
  res = qbus_methods__sub_add (self->methods, topic, user_ptr, on_val, err);
  
  cape_str_del (&module_upper_case);
  cape_str_del (&topic);

  return res;
}

//-----------------------------------------------------------------------------

int qbus_unsubscribe (QBus self, const CapeString module, const CapeString ident, CapeErr err)
{
  int res;

  // local objects
  CapeString topic = NULL;
  CapeString module_upper_case = cape_str_cp (module);
  cape_str_to_upper (module_upper_case);

  // create the topic
  topic = cape_str_catenate_c (module_upper_case, '_', ident);

  // this will deactive the subscription on the connection
  qbus_con_sub_rm (self->con, topic);

  // this will remove the callback related to the topic
  qbus_methods__sub_rm (self->methods, topic);
  
  res = CAPE_ERR_NONE;
  
  cape_str_del (&module_upper_case);
  cape_str_del (&topic);

  return res;
}

//-----------------------------------------------------------------------------

void qbus_next (QBus self, const CapeString ident, CapeUdc* p_val)
{
  // local objects
  CapeString topic = cape_str_catenate_c (qbus_config_name (self->config), '_', ident);

  // this will emit the new value to the engine
  qbus_con_sub_next (self->con, topic, p_val);

  cape_str_del (&topic);
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

int qbus_wait (QBus self, CapeUdc* p_args, CapeErr err)
{
  int res;

  res = qbus_init (self, p_args, err);
  if (res)
  {
    return res;
  }

  return qbus_wait__intern (self, err);
}

//-----------------------------------------------------------------------------

void qbus_instance (const char* name, void* ptr, fct_qbus_on_init on_init, fct_qbus_on_done on_done, number_t argc, char *argv[])
{
  int res;

  // local objects
  CapeErr err = cape_err_new ();
  QBus self = qbus_new (name);
  CapeUdc args = NULL;

#if defined __WINDOWS_OS
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "instance", "start qbus initialization");
  
#else

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "instance", "start qbus initialization [%i:%i]", getuid(), getgid());
  
#endif

  // convert program arguments into a node with parameters
  args = cape_args_from_args (argc, argv, NULL);

  res = qbus_init__preconditions (self, &args, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_log_msg (CAPE_LL_DEBUG, "QBUS", "instance", "---- user initialization --------------------------------------------------");

  if (on_init)
  {
    res = on_init (self, ptr, &ptr, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  cape_log_msg (CAPE_LL_DEBUG, "QBUS", "instance", "---- submodules -----------------------------------------------------------");
    
  res = qbus_init__submodules (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_msg (CAPE_LL_DEBUG, "QBUS", "instance", "---- main loop ------------------------------------------------------------");
  
#if defined __WINDOWS_OS

  // TODO: nice to have
  {


  }

#else

  // disable console echoing
  {
    struct termios saved;
    struct termios attributes;

    tcgetattr(STDIN_FILENO, &saved);
    tcgetattr(STDIN_FILENO, &attributes);

    attributes.c_lflag &= ~ ECHO;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attributes);

    // *** main loop ***
    qbus_wait__intern (self, err);

    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
  }

#endif

  if (on_done)
  {
    on_done (self, ptr, err);
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  qbus_del (&self);
  cape_err_del (&err);
}
