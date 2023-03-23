#include "qbus.h" 
#include "qbus_config.h"
#include "qbus_route.h"
#include "qbus_obsvbl.h"
#include "qbus_logger.h"
#include "qbus_engines.h"
#include "qbus_queue.h"
#include "qbus_chain.h"
#include "qbus_methods.h"

// c includes
#include <stdlib.h>
#include <stdio.h>

#ifdef __WINDOWS_OS

#include <windows.h>

#else

#include <termios.h>
#include <unistd.h>

#endif

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_types.h"
#include "sys/cape_file.h"
#include "stc/cape_str.h"
#include "aio/cape_aio_sock.h"
#include "fmt/cape_args.h"
#include "fmt/cape_json.h"
#include "fmt/cape_tokenizer.h"
#include "sys/cape_btrace.h"
#include "sys/cape_thread.h"
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

struct QBus_s
{
  CapeAioContext aio;
  
  QBusRoute route;

  QBusObsvbl obsvbl;
  
  QBusEngines engines;
  
  QBusLogger logger;
  
  QBusConfig config;
  
  QBusQueue queue;
  
  QBusChain chain;
  
  QBusMethods methods;
};

//-----------------------------------------------------------------------------

int __STDCALL qbus__load__on_timer (void* ptr)
{
  QBus self = ptr;
  
  CapeUdc value = cape_udc_new (CAPE_UDC_STRING, NULL);
  
  cape_udc_set_s_cp (value, "load: 12%");
  
  qbus_obsvbl_emit (self->obsvbl, qbus_route_uuid_get (self->route), "load", &value);
  
  return TRUE;
}

//-----------------------------------------------------------------------------

QBus qbus_new (const char* module_origin)
{
  QBus self = CAPE_NEW(struct QBus_s);

  //
  self->config = qbus_config_new (module_origin);

  self->engines = qbus_engines_new ();

  self->logger = qbus_logger_new ();

  self->route = qbus_route_new (qbus_config_get_name (self->config), self->engines);

  self->obsvbl = qbus_obsvbl_new (self->engines, self->route);
  
  qbus_route_set (self->route, self->obsvbl);
  
  self->aio = cape_aio_context_new ();
  
  self->queue = qbus_queue_new ();
  
  self->chain = qbus_chain_new ();
  
  self->methods = qbus_methods_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_del (QBus* p_self)
{
  QBus self = *p_self;

  qbus_chain_del (&(self->chain));
  
  qbus_methods_del (&(self->methods));
  
  qbus_route_del (&(self->route));
  
  qbus_obsvbl_del (&(self->obsvbl));

  qbus_engines_del (&(self->engines));

  qbus_logger_del (&(self->logger));
  
  qbus_config_del (&(self->config));
  
  cape_aio_context_del (&(self->aio));
  
  qbus_queue_del (&(self->queue));
  
  CAPE_DEL (p_self, struct QBus_s);
}

//-----------------------------------------------------------------------------

int qbus_init (QBus self, CapeUdc pvds, number_t workers, CapeErr err)
{
  int res;
  
  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    return res;
  }

  // start all worker threads
  res = qbus_queue_init (self->queue, qbus_config_get_threads (self->config), err);
  if (res)
  {
    return res;
  }

  // load all engines and initialize the connection contexts
  res = qbus_engines__init_pvds (self->engines, pvds, self->aio, self->route, self->obsvbl, err);
  if (res)
  {
    return res;
  }
  
  {
    CapeAioTimer timer = cape_aio_timer_new ();
    
    res = cape_aio_timer_set (timer, 5000, self, qbus__load__on_timer, err);
    if (res)
    {
      goto cleanup_and_exit;
    }
    
    res = cape_aio_timer_add (&timer, self->aio);
    if (res)
    {
      goto cleanup_and_exit;
    }
  }

  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  return res;
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

int qbus_wait (QBus self, CapeUdc pvds, number_t workers, CapeErr err)
{
  int res;
  
  res = qbus_init (self, pvds, workers, err);
  if (res)
  {
    return res;
  }
  
  return qbus_wait__intern (self, err);
}

//-----------------------------------------------------------------------------

int qbus_register (QBus self, const char* method, void* user_ptr, fct_qbus_onMessage on_event, fct_qbus_onRemoved on_rm, CapeErr err)
{
  return qbus_methods_add (self->methods, method, user_ptr, on_event, err);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_send__process_local (void* qbus_ptr, QBusPvdConnection conn, QBusQueueItem qitem)
{
  int res;
  
  QBusMethodItem mitem;
  
  QBus self = qbus_ptr;
  
  // local objects
  CapeErr err = cape_err_new ();
  QBusM qout = qbus_message_new (NULL, NULL);

  CapeString last_chain_key = cape_str_mv (&(qitem->msg->chain_key));
  CapeString last_sender = cape_str_mv (&(qitem->msg->sender));
  
  CapeString next_chain_key = cape_str_uuid ();

  // set next chain key
  qitem->msg->chain_key = cape_str_cp (next_chain_key);
  qitem->msg->sender = cape_str_cp (qbus_route_name_get (self->route));

  // set default message type
  qout->mtype = QBUS_MTYPE_JSON;
  
  // try to find the method stored in route
  mitem = qbus_methods_get (self->methods, qitem->method);
  
  if (mitem == NULL)
  {
    goto exit_and_cleanup;
  }

  {
    CapeString last_chain_key_copy = cape_str_cp (last_chain_key);
    CapeString last_sender_copy = cape_str_cp (last_sender);
    
    CapeUdc rinfo_copy = cape_udc_cp (qitem->msg->rinfo);
    
    qbus_chain_add (self->chain, qitem->user_ptr, qitem->user_fct, &last_chain_key_copy, &next_chain_key, &last_sender_copy, &rinfo_copy);
    
    cape_udc_del (&rinfo_copy);
    
    cape_str_del (&last_sender_copy);
    cape_str_del (&last_chain_key_copy);
  }
  
  res = qbus_methods_item__call_request (mitem, self, qitem->msg, qout, err);

  switch (res)
  {
    case CAPE_ERR_CONTINUE:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call returned a continued state");
      
      // this request shall not continue and another request was created instead
      // we need to add the callback and ptr to the chains list, for response
      // -> don't consider qout
      
      break;
    }
    default:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call returned a terminated state");
      
      if (res)
      {
        // tranfer ownership
        qout->err = err;
        
        // create an empty error object
        err = cape_err_new ();
      }
      
      // correct chain parameters
      cape_str_replace_cp (&(qout->chain_key), last_chain_key);
      cape_str_replace_cp (&(qout->sender), last_sender);
      
      // add rinfo
      cape_udc_replace_cp (&(qout->rinfo), qitem->msg->rinfo);
      
      {
        // create a method object to re-use existing functionality
        QBusMethodItem qmeth_next = qbus_methods_item_new (QBUS_METHOD_TYPE__RESPONSE, NULL, qitem->user_ptr, qitem->user_fct);
        
        // provide the last chain key to handle the return chain traversal path
        qbus_methods_item_continue (qmeth_next, &last_chain_key, &last_sender, &(qitem->msg->rinfo));
        
        // this requests ends here, now send the results back
        qbus_methods_item__call_response (qmeth_next, self, qout, NULL, err);
        
        qbus_methods_item_del (&qmeth_next);
      }
      
      break;
    }
  }
  
exit_and_cleanup:

  cape_str_del (&next_chain_key);

  cape_str_replace_mv (&(qitem->msg->chain_key), &last_chain_key);
  cape_str_replace_mv (&(qitem->msg->sender), &last_sender);

  qbus_message_del (&qout);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_send__process_request (void* qbus_ptr, QBusPvdConnection conn, QBusQueueItem qitem)
{
  QBus self = qbus_ptr;

  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  // local objects
  CapeString next_chainkey = cape_str_uuid();
  
  // add default content
  qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, next_chainkey, qitem->module, qitem->method, qbus_route_name_get (self->route));
  
  // add message content
  qitem->msg->rinfo = qbus_frame_set_qmsg (frame, qitem->msg, NULL);
  
  // register this request as response in the chain storage
  qbus_chain_add (self->chain, qitem->user_ptr, qitem->user_fct, &(qitem->msg->chain_key), &next_chainkey, &(qitem->msg->sender), &(qitem->msg->rinfo));
  
  // finally send the frame
  qbus_engines__send (self->engines, frame, conn);

  qbus_frame_del (&frame);
}

//-----------------------------------------------------------------------------

int qbus_send__request (QBus self, const char* module, const char* method, QBusM msg, void* user_ptr, fct_qbus_onMessage user_fct, int cont, CapeErr err)
{
  if (cape_str_compare (module, qbus_route_name_get (self->route)))
  {
    QBusQueueItem item = qbus_queue_item_new (msg, NULL, method, user_ptr, user_fct);
    
    // add to queue as local processing
    qbus_queue_add (self->queue, NULL, &item, self, qbus_send__process_local);
    
    return CAPE_ERR_CONTINUE;
  }
  else
  {
    QBusPvdConnection conn = qbus_route_get (self->route, module);
    
    if (conn)
    {
      QBusQueueItem item = qbus_queue_item_new (msg, module, method, user_ptr, user_fct);
      
      // add to queue as request
      qbus_queue_add (self->queue, conn, &item, self, qbus_send__process_request);
      
      return CAPE_ERR_CONTINUE;
    }
    else
    {
      qbus_message__no_route (msg, self, user_ptr, user_fct);
      
      return cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "QBUS", "no route to module [%s]", module);
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_send (QBus self, const char* module, const char* method, QBusM msg, void* user_ptr, fct_qbus_onMessage user_fct, CapeErr err)
{
  // correct chain key
  // -> the key for the start must be NULL
  cape_str_del (&(msg->chain_key));

  return qbus_send__request (self, module, method, msg, user_ptr, user_fct, FALSE, err);
}

//-----------------------------------------------------------------------------

int qbus_continue (QBus self, const char* module, const char* method, QBusM qin, void** p_ptr, fct_qbus_onMessage on_msg, CapeErr err)
{
  int res;
  
  if (p_ptr)
  {
    res = qbus_send__request (self, module, method, qin, *p_ptr, on_msg, TRUE, err);
    
    *p_ptr = NULL;
  }
  else
  {
    res = qbus_send__request (self, module, method, qin, NULL, on_msg, TRUE, err);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_response__local (QBus self, QBusM msg)
{
  if (msg->chain_key)
  {
    QBusMethodItem mitem = qbus_chain_ext (self->chain, msg->chain_key);
    
    if (mitem)
    {
      switch (qbus_methods_item_type (mitem))
      {
        case QBUS_METHOD_TYPE__RESPONSE:
        {
          CapeErr err = cape_err_new ();
          
          qbus_methods_item__call_response (mitem, self, msg, NULL, err);
          
          cape_err_del (&err);
          
          break;
        }
      }
      
      qbus_methods_item_del (&mitem);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "QBUS", "response", "chain key was not found in local response");
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_response (QBus self, const char* module, QBusM msg, CapeErr err)
{
  if (module)
  {
    if (cape_str_compare (module, qbus_route_name_get (self->route)))
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "response", "execute local response on '%s'", module);
      
      qbus_response__local (self, msg);
      
      return CAPE_ERR_NONE;
    }
    else
    {
      QBusPvdConnection conn = qbus_route_get (self->route, module);
      
      if (conn)
      {
        // create a new frame
        QBusFrame frame = qbus_frame_new ();
        
        // add default content
        qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_RES, msg->chain_key, module, NULL, qbus_route_name_get (self->route));
        
        // add message content
        qbus_frame_set_qmsg (frame, msg, err);
        
        // finally send the frame
        qbus_engines__send (self->engines, frame, conn);

        qbus_frame_del (&frame);
        
        return CAPE_ERR_NONE;
      }
    }
  }
  
  return cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "QBUS", "no route for response [%s]", module);
}

//-----------------------------------------------------------------------------

const CapeString qbus_name (QBus self)
{
  return qbus_config_get_name (self->config);
}

//-----------------------------------------------------------------------------

CapeAioContext qbus_aio (QBus self)
{
  return self->aio;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_modules (QBus self)
{
  return qbus_route_modules (self->route);
}

//-----------------------------------------------------------------------------

QBusSubscriber qbus_subscribe (QBus self, const CapeString module, const CapeString name, void* user_ptr, fct_qbus_on_emit user_fct)
{
  return qbus_obsvbl_subscribe (self->obsvbl, module, name, user_ptr, user_fct);
}

//-----------------------------------------------------------------------------

void qbus_emit (QBus self, const CapeString value_name, CapeUdc* p_value)
{
  CapeString prefix = cape_str_catenate_2 ("#", qbus_route_name_get (self->route));
  
  qbus_obsvbl_emit (self->obsvbl, prefix, value_name, p_value);
  
  cape_str_del (&prefix);
}

//-----------------------------------------------------------------------------

void qbus_log_msg (QBus self, const CapeString remote, const CapeString message)
{
  qbus_logger_msg (self->logger, remote, message);
}

//-----------------------------------------------------------------------------

void qbus_log_fmt (QBus self, const CapeString remote, const char* format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  
  {
    CapeString h = cape_str_flp (format, ptr);
    
    qbus_logger_msg (self->logger, remote, h);
    
    cape_str_del (&h);
  }

  va_end(ptr);
}

//-----------------------------------------------------------------------------

void qbus_instance (const char* name, void* ptr, fct_qbus_on_init on_init, fct_qbus_on_done on_done, int argc, char *argv[])
{
  int res = CAPE_ERR_NONE;

  const CapeString module_name;
  
  // local objects
  CapeErr err = cape_err_new ();
  CapeFileLog log = NULL;

  QBus self = NULL;
  CapeUdc args = NULL;
  
  void* user_ptr= NULL;
  
  printf ("          _                     \n");
  printf ("   __ _  | |__    _   _   ___   \n");
  printf ("  / _` | | '_ \\  | | | | / __|  \n");
  printf (" | (_| | | |_) | | |_| | \\__ \\  \n");
  printf ("  \\__, | |_.__/   \\__,_| |___/  \n");
  printf ("     |_|                        \n");
  printf ("\n");

  res = cape_btrace_activate (err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // convert program arguments into a node with parameters
  args = cape_args_from_args (argc, argv, NULL);

  // replace name from the parameters
  module_name = cape_udc_get_s (args, "n", name);

  // start a new qbus instance
  self = qbus_new (module_name);

  // set name and args to the config
  qbus_config_set_args (self->config, &args);
  
  // run the config
  res = qbus_config_init (self->config, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_log_msg (CAPE_LL_TRACE, module_name, "qbus_instance", "arguments parsed");
  
  res = qbus_init (self, qbus_config_get_pvds (self->config), qbus_config_get_threads (self->config), err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qbus_logger_init (self->logger, self->aio, qbus_config_get_logger (self->config), err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "instance", "error in initialization: %s", cape_err_text(err));
    goto exit_and_cleanup;
  }
  
  if (on_init)
  {
    res = on_init (self, ptr, &user_ptr, err);
  }

  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "instance", "error in initialization: %s", cape_err_text(err));    
    goto exit_and_cleanup;
  }
  
  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "start main loop");

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
    on_done (self, user_ptr, err);
  }
  
exit_and_cleanup:
  
  qbus_del (&self);
  
  cape_err_del (&err);
  cape_log_del (&log);
}

//-----------------------------------------------------------------------------

void* qbus_add_on_change (QBus self, void* ptr, fct_qbus_on_route_change on_change)
{
  
}

//-----------------------------------------------------------------------------

void qbus_rm_on_change (QBus self, void* obj)
{
  
}

//-----------------------------------------------------------------------------

void qbus_methods (QBus self, const char* module, void* ptr, fct_qbus_on_methods on_methods)
{
  
}

//-----------------------------------------------------------------------------
