#include "qbus.h"

#include <stdlib.h>

// cape includes
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

static int __STDCALL test_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;

  res = CAPE_ERR_NONE;

  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL app__on_fct1 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL app__on_timer (void* user_ptr)
{
  int res;
  QBus qbus = user_ptr;
  
  // local objects
  CapeErr err = cape_err_new ();
  QBusM msg = qbus_message_new (NULL, NULL);

  res = qbus_send (qbus, "test2", "function1", msg, qbus, app__on_fct1, err);

  qbus_message_del (&msg);
  cape_err_del (&err);
  return TRUE;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  
  // local objects
  CapeAioTimer timer = cape_aio_timer_new ();

  res = cape_aio_timer_set (timer, 1000, qbus, app__on_timer, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_aio_timer_add (&timer, qbus_aio (qbus));
  if (res)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.TIMER_ADD");
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_done (QBus qbus, void* ptr, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("TEST", NULL, app_on_init, app_on_done, argc, argv);

  return 0;
}

//-----------------------------------------------------------------------------
