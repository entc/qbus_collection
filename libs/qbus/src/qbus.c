#include "qbus.h" 
#include "qbus_config.h"
#include "qbus_route.h"
#include "qbus_obsvbl.h"
#include "qbus_logger.h"
#include "qbus_engines.h"

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

//-----------------------------------------------------------------------------

struct QBus_s
{
  CapeAioContext aio;
  
  QBusRoute route;

  QBusObsvbl obsvbl;
  
  QBusEngines engines;
  
  QBusLogger logger;
  
  QBusConfig config;
};

//-----------------------------------------------------------------------------

QBus qbus_new (const char* module_origin)
{
  QBus self = CAPE_NEW(struct QBus_s);

  //
  self->config = qbus_config_new (module_origin);

  self->engines = qbus_engines_new ();

  self->logger = qbus_logger_new ();

  self->route = qbus_route_new (qbus_config_get_name (self->config), self->engines);

  self->obsvbl = qbus_obsvbl_new (self->engines);
  
  self->aio = cape_aio_context_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_del (QBus* p_self)
{
  QBus self = *p_self;

  qbus_route_del (&(self->route));
  
  qbus_obsvbl_del (&(self->obsvbl));

  qbus_engines_del (&(self->engines));

  qbus_logger_del (&(self->logger));
  
  qbus_config_del (&(self->config));
  
  cape_aio_context_del (&(self->aio));
  
  //cape_str_del (&(self->name));
  
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

  // load all engines and initialize the connection contexts
  res = qbus_engines__init_pvds (self->engines, pvds, self->aio, self->route, err);
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

int qbus_register (QBus self, const char* method, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_send (QBus self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_continue (QBus self, const char* module, const char* method, QBusM qin, void** p_ptr, fct_qbus_onMessage on_msg, CapeErr err)
{
  int res;
  
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_response (QBus self, const char* module, QBusM msg, CapeErr err)
{
  
  return CAPE_ERR_NONE;
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
  
}

//-----------------------------------------------------------------------------

QBusSubscriber qbus_subscribe (QBus self, int type, const CapeString module, const CapeString name, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------

QBusEmitter qbus_emitter_add (QBus self, const CapeString name, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------

int qbus_emitter_rm (QBus self, QBusEmitter emitter, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------

void qbus_emitter_next (QBus self, QBusEmitter emitter, CapeUdc data)
{
  
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
